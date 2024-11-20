#pragma once
#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "tpc.grpc.pb.h"

#include <string.h>

using grpc::Server;
using grpc::CompletionQueue;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::StatusCode;

using tpc::TpcServer;
using tpc::Ballot;
using tpc::Transaction;
using tpc::TransferReq;
using tpc::TransferRes;
using tpc::BalanceReq;
using tpc::BalanceRes;
using tpc::LogRes;
using tpc::PrepareReq;
using tpc::PrepareRes;
using tpc::AcceptReq;
using tpc::AcceptRes;
using tpc::CommitReq;
using tpc::SyncReq;
using tpc::SyncRes;

// Server Implementation
class ServerImpl final {
public:
    ServerImpl(int, std::string);
    void run(std::string);
    ~ServerImpl();
    void HandleRPCs();

    void handlePrepareReply(Status&, PrepareReq&, PrepareRes&);
    void handleAcceptReply(Status&, AcceptReq&, AcceptRes&);
    void handleSyncReply(Status&, SyncRes&);
    
    bool processTransferCall(TransferReq&, TransferRes&);
    void processPrepareCall(PrepareReq&, PrepareRes&);
    void processAcceptCall(AcceptReq&, AcceptRes&);
    void processCommitCall(CommitReq&);
    void processSyncCall(SyncReq&, SyncRes&);
    void processGetBalanceCall(BalanceReq&, BalanceRes&);
    void processGetLogsCall(LogRes&);
    
private:
    std::shared_ptr<spdlog::logger> logger;
    int server_id;
    std::string server_name;
    int cluster_id;

    std::unique_ptr<ServerCompletionQueue> request_cq;
    std::unique_ptr<CompletionQueue> response_cq;
    TpcServer::AsyncService service;
    std::unique_ptr<Server> server;
    std::vector<std::unique_ptr<TpcServer::Stub>> stubs;

    const static int CLUSTER_SIZE = 3;
    const static int MAJORITY = 2;
    const static int RETRY_TIMEOUT_MS = 1000;
    const static int RPC_TIMEOUT_MS = 10;

    std::map<std::string, int> balances;
    std::vector<Transaction> log;

    int ballot_num;
    bool promised;
    Ballot promised_num;

    bool accepted;
    Ballot accept_num;
    Transaction accept_val;

    int last_committed;
    Ballot last_committed_ballot;

    bool in_sync;
    
    int prepare_successes;
    int prepare_failures;
    int accept_successes;
    int accept_failures;
    int commit_successes;
    int commit_failures;
    bool await_prepare_decision;
    bool await_accept_decision;
};