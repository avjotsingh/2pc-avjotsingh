#pragma once

namespace types {
    enum RequestTypes {
        TPC_PREPARE,
        TPC_COMMIT,
        TPC_ABORT,
        TRANSFER,
        PREPARE,
        ACCEPT,
        COMMIT,
        SYNC,
        BALANCE,
        LOGS
    };

    struct Transaction {
        int sender;
        int receiver;
        int amount;
    };

    enum TransactionType {
        INTRA,
        CROSS
    };

    enum TransactionStatus {
        NO_STATUS,
        PREPARED,
        COMMITTED,
        ABORTED
    };

    struct WALEntry {
        int tid;
        int ballot_num;
        int ballot_server_id;
        Transaction txn;
        TransactionType type;
        TransactionStatus status;
    };
}