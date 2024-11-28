#include "client.h"
#include "../constants.h"
#include "../utils/utils.h"

using grpc::ClientContext;
using grpc::Status;
using google::protobuf::Empty;

using tpc::TpcServer;
using tpc::BalanceReq;
using tpc::BalanceRes;
using tpc::LogEntry;
using tpc::LogRes;
using tpc::DisconnectReq;
using tpc::TransferReq;
using tpc::Transaction;

Client::Client() {
    logger = spdlog::get("console");
    for (auto& s: constants::server_ids) {
        std::string name = s.first;
        stubs[name] = TpcServer::NewStub(grpc::CreateChannel(constants::server_addresses[name], grpc::InsecureChannelCredentials()));    
    }
}

void Client::updateDisconnected(std::vector<std::string> disconnected_servers) {
    DisconnectReq request;
    for (auto& s: disconnected_servers) request.add_servers(s);
    for (auto& s: stubs) {
        ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(Client::RPC_TIMEOUT_MS));
        
        Empty reply;
        s.second->Disconnect(&context, request, &reply);
    }
}

void Client::processTransactions(std::vector<types::Transaction> transactions, std::vector<std::string> leaders) {
    int issued = 0;

    for (auto& t: transactions) {
        bool cross_shard = false;
        int sender_cluster = utils::getClusterIdFromClientId(t.sender);
        int receiver_cluster = utils::getClusterIdFromClientId(t.receiver);
        
        std::string sender_leader = leaders[sender_cluster - 1];
        std::string receiver_leader = leaders[receiver_cluster - 1];

        if (sender_cluster != receiver_cluster) cross_shard = true;

        TransferReq request;
        auto epoch = std::chrono::system_clock::now().time_since_epoch();
        long tid = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch).count();
        request.set_tid(tid);
        
        Transaction* txn = request.mutable_txn();
        txn->set_sender(t.sender);
        txn->set_receiver(t.receiver);
        txn->set_amount(t.amount);

        if (!cross_shard) {
            logger->debug("Sending transfer to {}", sender_leader);
            sendTransfer(request, sender_leader);
        } else {
            processing[tid] = { { t.sender, t.receiver, t.amount }, 0, 0 };
            logger->debug("Sending 2PC prepare to {} and {}", sender_leader, receiver_leader);
            tpcPrepare(request, sender_leader);
            tpcPrepare(request, receiver_leader);
        }
    }
}

void Client::printBalance(int client_id) {
    int cluster_id = utils::getClusterIdFromClientId(client_id);
    std::vector<std::string> servers = utils::getServersInCluster(cluster_id);
    std::vector<std::string> balances;

    BalanceReq request;
    request.set_client(client_id);
    for (auto& s: servers) {
        ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(Client::RPC_TIMEOUT_MS));
        
        BalanceRes reply;
        Status status = stubs[s]->Balance(&context, request, &reply);
        balances.push_back(status.ok() ? std::to_string(reply.amount()) : "-");
    }

    std::cout << std::setw(10) << "Server|" << std::setw(10) << "Balance|" << std::endl;
    for (int i = 0; i < servers.size(); i++) {
        std::cout << std::setw(10) << servers[i] + "|" << std::setw(10) << balances[i] + "|" << std::endl;
    }
}

void Client::printDatastore() {
    std::map<std::string, std::vector<types::WALEntry>> datastore;
    Empty request;

    for (auto& s: constants::server_ids) {
        datastore[s.first] = std::vector<types::WALEntry>();
        ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(Client::RPC_TIMEOUT_MS));
        LogRes reply;
        Status status = stubs[s.first]->Logs(&context, request, &reply);
        if (status.ok()) {
            for (int i = 0; i < reply.logs_size(); i++) {
                LogEntry e = reply.logs(i);
                types::WALEntry entry = {
                    e.tid(),
                    e.ballot_num(),
                    e.ballot_server_id(),
                    { e.txn().sender(), e.txn().receiver(), e.txn().amount() },
                    static_cast<types::TransactionType>(e.type()),
                    static_cast<types::TransactionStatus>(e.status())
                };
                datastore[s.first].push_back(entry);
            }
        }
    }

    std::cout << std::setw(10) << "Server|" <<  "Datastore|" << std::endl;
    for (auto& pair: datastore) {
        std::cout << std::setw(10) << pair.first + "|";
        
        for (int i = 0; i < pair.second.size(); i++) {
            types::WALEntry e = pair.second[i];
            std::stringstream entry;
            if (i > 0) std::cout << " -> ";
        
            entry << "[<" << e.ballot_num << "," << e.ballot_server_id << ">, ";
        
            if (e.type == types::TransactionType::CROSS) {
                std::string status;
                switch (e.status) {
                    case types::TransactionStatus::PREPARED:
                        status = "P"; break;
                    case types::TransactionStatus::COMMITTED:
                        status = "C"; break;
                    case types::TransactionStatus::ABORTED:
                        status = "A"; break;
                    default:
                        status = "NS";
                }
                entry << status << ", ";
            }
            
            entry << "(" << e.txn.sender << ", " << e.txn.receiver << ", " << e.txn.amount << ")]";
            std::cout << entry.str();
        }
        std::cout << std::endl;
    }
}

void Client::printPerformance() {
    double performance = 0.0;
    if (transactions_processed > 0) {
        performance = (transactions_processed * 1e9) / (total_time);
    }

    std::cout << "Transactions Processed: " << transactions_processed << std::endl;
    std::cout << "Performance: " << performance << " transactions/second" << std::endl;
}

void Client::sendTransfer(TransferReq& request, std::string leader) {
    ClientCall* call = new ClientCall;
    call->type = types::RequestTypes::TRANSFER;
    call->transferReader = stubs[leader]->PrepareAsyncTransfer(&call->context, request, &cq);
    call->transferReader->StartCall();
    call->transferReader->Finish(&call->reply, &call->status, (void*)call);
}

void Client::tpcPrepare(TransferReq& request, std::string leader) {
    ClientCall* call = new ClientCall;
    call->type = types::RequestTypes::TPC_PREPARE;
    call->transferReader = stubs[leader]->PrepareAsyncTpcPrepare(&call->context, request, &cq);
    call->transferReader->StartCall();
    call->transferReader->Finish(&call->reply, &call->status, (void*)call);
}

void Client::tpcCommit(TpcTid& request, int sender_cluster, int receiver_cluster) {
    logger->debug("Sending TPC commit to clusters {} and {}", sender_cluster, receiver_cluster);
    for (auto& s: constants::server_ids) {
        int cluster_id = utils::getClusterIdFromServerId(s.second);
        if (cluster_id == sender_cluster || cluster_id == receiver_cluster) {
            ClientCall* call = new ClientCall;
            call->type = types::RequestTypes::TPC_COMMIT;
            call->emptyReader = stubs[s.first]->PrepareAsyncTpcCommit(&call->context, request, &cq);
            call->emptyReader->StartCall();
            call->emptyReader->Finish(&call->empty, &call->status, (void*)call);
        }
    }
}

void Client::tpcAbort(TpcTid& request, int sender_cluster, int receiver_cluster) {
    logger->debug("Sending TPC abort to clusters {} and {}", sender_cluster, receiver_cluster);
    for (auto& s: constants::server_ids) {
        int cluster_id = utils::getClusterIdFromServerId(s.second);
        if (cluster_id == sender_cluster || cluster_id == receiver_cluster) {
            ClientCall* call = new ClientCall;
            call->type = types::RequestTypes::TPC_ABORT;
            call->emptyReader = stubs[s.first]->PrepareAsyncTpcAbort(&call->context, request, &cq);
            call->emptyReader->StartCall();
            call->emptyReader->Finish(&call->empty, &call->status, (void*)call);
        }
    }
}

void Client::consumeReplies() {
    void* tag;
    bool ok;

    while(cq.Next(&tag, &ok)) {
        ClientCall* call = static_cast<ClientCall*>(tag);
        CHECK(ok);
        
        if (call->type == types::TRANSFER && call->status.ok() && call->reply.ack()) {
            auto epoch = std::chrono::system_clock::now().time_since_epoch();
            int end_time = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch).count();
            total_time += (end_time - call->reply.tid());
            ++transactions_processed;
        } else if (call->type == types::RequestTypes::TPC_PREPARE) {
            long tid = call->reply.tid();
            if (!call->status.ok() || !call->reply.ack()) ++processing[tid].failures;
            else ++processing[tid].successes;

            bool commit = processing[tid].successes == 2;
            bool abort = processing[tid].failures > 0 && (processing[tid].successes + processing[tid].failures == 2);
            if (commit || abort) {
                int sender_cluster = utils::getClusterIdFromClientId(processing[tid].txn.sender);
                int receiver_cluster = utils::getClusterIdFromClientId(processing[tid].txn.receiver);

                TpcTid request;
                request.set_tid(tid);
                commit ? 
                    tpcCommit(request, sender_cluster, receiver_cluster) :
                    tpcAbort(request, sender_cluster, receiver_cluster);
                
                processing.erase(tid);
            }
        }
        
        delete call;
    }
}
