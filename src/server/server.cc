#include <grpcpp/grpcpp.h>
#include "tpc.grpc.pb.h"

#include <map>

#include "server.h"
#include "in_call.h"
#include "out_call.h"
#include "../constants.h"

using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::CompletionQueue;
using grpc::Server;

using tpc::Ballot;

ServerImpl::ServerImpl(int id, std::string name) {
    logger = spdlog::stdout_color_mt("console");
    logger->set_level(spdlog::level::debug);

    server_id = id % ServerImpl::CLUSTER_SIZE;
    server_name = name;
    cluster_id = server_id / 3;
    balances = std::map<std::string, int>();
    for (auto& pair: constants::client_clusters) {
        if (pair.second == cluster_id) balances[pair.first] = 100;
    }
    ballot_num = 0;
    promised = false;
    accepted = false;
    await_prepare_decision = false;
    await_accept_decision = false;
    last_committed = -1;
    in_sync = false;
}

ServerImpl::~ServerImpl() {
    server->Shutdown();
    request_cq->Shutdown();
    response_cq->Shutdown();
}

void ServerImpl::run(std::string address) {
    ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    request_cq = builder.AddCompletionQueue();
    response_cq = std::make_unique<CompletionQueue>();
    server = builder.BuildAndStart();

    for (auto& pair: constants::server_addresses) {
        std::string server = pair.first;
        std::string address = pair.second;
        if (constants::cluster_ids[server_name] == cluster_id) {
            stubs.push_back(TpcServer::NewStub(grpc::CreateChannel(address, grpc::InsecureChannelCredentials())));    
        }
    }

    logger->info("Server running on {}", address);
    HandleRPCs();
}

void ServerImpl::HandleRPCs() {
    new InCall(&service, this, request_cq.get(), types::TRANSFER, ServerImpl::RETRY_TIMEOUT_MS);
    new InCall(&service, this, request_cq.get(), types::BALANCE, ServerImpl::RETRY_TIMEOUT_MS);
    new InCall(&service, this, request_cq.get(), types::LOGS, ServerImpl::RETRY_TIMEOUT_MS);
    new InCall(&service, this, request_cq.get(), types::PREPARE, ServerImpl::RETRY_TIMEOUT_MS);
    new InCall(&service, this, request_cq.get(), types::ACCEPT, ServerImpl::RETRY_TIMEOUT_MS);
    new InCall(&service, this, request_cq.get(), types::COMMIT, ServerImpl::RETRY_TIMEOUT_MS);
    new InCall(&service, this, request_cq.get(), types::SYNC, ServerImpl::RETRY_TIMEOUT_MS);

    void* request_tag;
    bool request_ok;
    void* response_tag;
    bool response_ok;

    while (true) {
        // Poll the request queue
        grpc::CompletionQueue::NextStatus request_status = request_cq->AsyncNext(&request_tag, &request_ok, gpr_time_0(GPR_CLOCK_REALTIME));

        // Poll the response queue
        grpc::CompletionQueue::NextStatus response_status = response_cq->AsyncNext(&response_tag, &response_ok, gpr_time_0(GPR_CLOCK_REALTIME));

        // Handle request events
        if (request_status == grpc::CompletionQueue::NextStatus::GOT_EVENT && request_ok) {
            static_cast<InCall*>(request_tag)->Proceed();  // Process request
        }

        // Handle response events
        if (response_status == grpc::CompletionQueue::NextStatus::GOT_EVENT && response_ok) {
            static_cast<OutCall*>(response_tag)->HandleRPCResponse();  // Process response
        }
    }
}

bool ServerImpl::processTransferCall(TransferReq& request, TransferRes& response) {
    logger->debug("Transfer received {}", request.DebugString());
    if (in_sync) {
        logger->debug("Syncing...");
        return false;
    }

    if (!await_prepare_decision && !await_accept_decision) {
        Ballot ballot;
        ballot.set_num(++ballot_num);
        ballot.set_server_id(server_id);

        PrepareReq prepare;
        prepare.mutable_ballot()->CopyFrom(ballot);
        prepare.set_last_committed(last_committed);

        // Send prepare request to all
        promised = true;
        promised_num = ballot;
        await_prepare_decision = true;
        prepare_successes = 1;
        prepare_failures = 0;
        logger->debug("Sending prepare to replicas {}", prepare.DebugString());
        for (int i = 0; i < stubs.size(); i++) {
            if (i != server_id) {
                OutCall* call = new OutCall(this, response_cq.get(), types::PREPARE, ServerImpl::RPC_TIMEOUT_MS);
                call->sendPrepare(prepare, stubs[i]);
            }
        }
        return false;

    } else if (await_prepare_decision) {
        if (prepare_successes >= ServerImpl::MAJORITY) {
            await_prepare_decision = false;
            Ballot ballot;
            ballot.set_num(ballot_num);
            ballot.set_server_id(server_id);

            Transaction t = request.txn();

            AcceptReq accept;
            accept.mutable_ballot()->CopyFrom(ballot);
            accept.mutable_txn()->CopyFrom(t);

            // Send accept request to all
            accepted = true;
            accept_num = ballot;
            accept_val = t;
            await_accept_decision = true;
            accept_successes = 1;
            accept_failures = 0;
            logger->debug("Sending accept to replicas {}", accept.DebugString());
            for (int i = 0; i < stubs.size(); i++) {
                if (i != server_id) {
                    OutCall* call = new OutCall(this, response_cq.get(), types::ACCEPT, ServerImpl::RPC_TIMEOUT_MS);
                    call->sendAccept(accept, stubs[i]);
                }
            }

        } else if (prepare_failures >= ServerImpl::MAJORITY) {
            await_prepare_decision = false;
        }

        return false;

    } else if (await_accept_decision) {
        if (accept_successes >= ServerImpl::MAJORITY) {
            await_accept_decision = false;

            CommitReq commit;
            commit.mutable_ballot()->CopyFrom(accept_num);

            logger->debug("Sending commit to replicas {}", commit.DebugString());
            // Send commit request to all
            for (int i = 0; i < stubs.size(); i++) {
                if (i != server_id) {
                    OutCall* call = new OutCall(this, response_cq.get(), types::COMMIT, ServerImpl::RPC_TIMEOUT_MS);
                    call->sendCommit(commit, stubs[i]);
                }
            }

            // Update server state
            log.push_back(accept_val);
            last_committed = log.size() - 1;
            last_committed_ballot = accept_num;
            promised = false;
            accepted = false;
            await_prepare_decision = false;
            await_accept_decision = false;
            
            return true;
            
        } else if (prepare_failures >= ServerImpl::MAJORITY) {
            await_accept_decision = false;
            return false;
        }

        return false;
    }
    return true;
}

void ServerImpl::processPrepareCall(PrepareReq& request, PrepareRes& response) {
    bool ack = false;
    logger->debug("Received prepare {}", request.DebugString());
    if (request.ballot().num() > ballot_num) {
        if (request.last_committed() == last_committed) {
            ack = true;
            promised = true;
            promised_num = request.ballot();
            ballot_num = request.ballot().num();

        } else if (request.last_committed() > last_committed) {
            logger->debug("Requesting sync due to prepare");
            // Synchronize
            in_sync = true;
            SyncReq sync;
            sync.set_last_committed(last_committed);

            OutCall* call = new OutCall(this, response_cq.get(), types::SYNC, ServerImpl::RPC_TIMEOUT_MS);
            call->sendSync(sync, stubs[request.ballot().server_id()]);
        }
    }

    response.set_ack(ack);
    response.mutable_ballot()->CopyFrom(request.ballot());
    if (ack && accepted) {
        response.mutable_accept_num()->CopyFrom(accept_num);
        response.mutable_accept_val()->CopyFrom(accept_val);
    } else if (!ack) {
        response.set_last_committed(last_committed);
        response.set_server_id(server_id);
    }
    logger->debug("Prepare response {}", response.DebugString());
}

void ServerImpl::processAcceptCall(AcceptReq& request, AcceptRes& response) {
    bool ack = false;
    logger->debug("Received accept {}", request.DebugString());
    if (promised && request.ballot().num() == promised_num.num()) {
        ack = true;
        accepted = true;
        accept_num = request.ballot();
        accept_val = request.txn();
    }

    response.set_ack(ack);
    response.mutable_ballot()->CopyFrom(request.ballot());
    logger->debug("Accept response {}", response.DebugString());
}

void ServerImpl::processCommitCall(CommitReq& request) {
    logger->debug("Received commit {}", request.DebugString());
    int commit_ballot_num = request.ballot().num();
    if (promised_num.num() == commit_ballot_num && accept_num.num() == commit_ballot_num) {
        log.push_back(accept_val);
        promised = false;
        accepted = false;
        last_committed = log.size() - 1;
        last_committed_ballot = accept_num;
    }
}

void ServerImpl::processSyncCall(SyncReq& request, SyncRes& response) {
    logger->debug("Received sync {}", request.DebugString());
    int commit_idx = request.last_committed();
    if (last_committed > commit_idx) {
        for (int i = commit_idx + 1; i < log.size(); i++) {
            response.add_txns()->CopyFrom(log[i]);
            response.mutable_last_committed_ballot()->CopyFrom(last_committed_ballot);
        }
        response.set_ack(true);
        response.mutable_last_committed_ballot()->CopyFrom(last_committed_ballot);
    } else {
        response.set_ack(false);
    }
    logger->debug("Sync response {}", response.DebugString());
}

void ServerImpl::processGetBalanceCall(BalanceReq& request, BalanceRes& response) {
    std::string client = request.client();
    response.set_amount(balances[client]);
}

void ServerImpl::processGetLogsCall(LogRes& response) {
    for (auto& t: log) {
        response.add_txns()->CopyFrom(t);
    }
}

void ServerImpl::handlePrepareReply(Status& status, PrepareReq& request, PrepareRes& response) {
    if (!await_prepare_decision) return;
    if (!status.ok() && request.ballot().num() == promised_num.num() && request.ballot().server_id() == server_id) {
        ++prepare_failures;
    } else if (status.ok() && response.ballot().num() == promised_num.num() && response.ballot().server_id() == server_id) {
        response.ack() ? ++prepare_successes : ++prepare_failures;
        if (response.has_last_committed() && response.last_committed() > last_committed) {
            // Synchronize
            logger->debug("Requesting sync due to prepare reject");
            in_sync = true;
            await_prepare_decision = false;
            
            SyncReq sync;
            sync.set_last_committed(last_committed);

            OutCall* call = new OutCall(this, response_cq.get(), types::SYNC, ServerImpl::RPC_TIMEOUT_MS);
            call->sendSync(sync, stubs[response.server_id()]);
        }
    }
}

void ServerImpl::handleAcceptReply(Status& status, AcceptReq& request, AcceptRes& response) {
    if (!await_accept_decision) return;
    if (!status.ok() && request.ballot().num() == accept_num.num() && request.ballot().server_id() == server_id) {
        ++accept_failures;
    }
    else if (status.ok() && response.ballot().num() == accept_num.num() && response.ballot().server_id() == server_id) {
        response.ack() ? ++accept_successes : ++accept_failures;
    }
}

void ServerImpl::handleSyncReply(Status& status, SyncRes& response) {
    if (status.ok()) {
        logger->debug("Received sync response {}", response.DebugString());
        if (response.ack()) {
            for (int i = 0; i < response.txns_size(); i++) {
                log.push_back(response.txns(i));
            }

            ballot_num = response.last_committed_ballot().num();
            last_committed += response.txns_size();
            last_committed_ballot = response.last_committed_ballot();
        }
    }

    in_sync = false;
}

void RunServer(std::string server_name) {
    int id = constants::server_ids[server_name];
    std::string address = constants::server_addresses[server_name];
    ServerImpl server(id, server_name);
    server.run(address);
}


int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: server <name>\n");
        exit(1);
    }

    try {
        RunServer(std::string(argv[1]));
    } catch (std::exception& e) {
        printf("Exception: %s", e.what());
    }
}