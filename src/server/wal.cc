#include "wal.h"

WAL::WAL(const std::string& fileName) : walFile(fileName) {
    logger = spdlog::get("console");
}

types::WALEntry WAL::getTransactionFromFile(std::streampos position) {
    std::ifstream inFile(walFile);
    if (!inFile) {
        logger->debug("Error: Unable to open WAL file for reading.");
        return { -1, -1, -1, { -1, -1, -1 }, types::TransactionType::CROSS, types::TransactionStatus::NO_STATUS };
    }

    inFile.seekg(position);
    std::string transaction;
    std::getline(inFile, transaction);
    inFile.close();

    std::istringstream iss(transaction);
    std::string command;
    long tid;
    int ballot_num;
    int ballot_server_id;
    int sender;
    int receiver;
    int amount;
    iss >> command >> tid >> ballot_num >> ballot_server_id >> sender >> receiver >> amount;

    return {
        tid,
        ballot_num,
        ballot_server_id,
        { sender, receiver, amount },
        types::TransactionType::CROSS,
        types::TransactionStatus::PREPARED
    };
}


types::WALEntry WAL::prepareTransaction(TransferReq& request, Ballot& ballot) {
    long tid = request.tid();
    int sender = request.txn().sender();
    int receiver = request.txn().receiver();
    int amount = request.txn().amount();
    int ballot_num = ballot.num();
    int ballot_server_id = ballot.server_id();

    std::ofstream outFile(walFile, std::ios::app);
    if (!outFile) {
        logger->debug("Error: Unable to open WAL file for writing.");
    }

    // Record file position and write transaction
    std::streampos position = outFile.tellp();
    transferIndex[tid] = position;

    outFile << "CS_PREPARE " << tid << " " << ballot_num << " " << ballot_server_id << " " << sender << " " << receiver << " " << amount << "\n";
    outFile.close();

    return {
        tid, 
        ballot_num,
        ballot_server_id,
        { sender, receiver, amount },
        types::TransactionType::CROSS,
        types::TransactionStatus::PREPARED
    };
}

types::WALEntry WAL::commitTransaction(TransferReq& request, Ballot& ballot) {
    long tid = request.tid();
    int sender = request.txn().sender();
    int receiver = request.txn().receiver();
    int amount = request.txn().amount();
    int ballot_num = ballot.num();
    int ballot_server_id = ballot.server_id();

    std::ofstream outFile(walFile, std::ios::app);
    if (!outFile) {
        logger->debug("Error: Unable to open WAL file for writing.");
        return { -1, -1, -1, { -1, -1, -1 }, types::TransactionType::CROSS, types::TransactionStatus::NO_STATUS };
    }

    outFile << "IN_COMMIT " << tid << " " << ballot_num << " " << ballot_server_id << " " << sender << " " << receiver << " " << amount << "\n";
    outFile.close();

    return {
        tid,
        ballot_num,
        ballot_server_id,
        { sender, receiver, amount },
        types::TransactionType::INTRA,
        types::TransactionStatus::COMMITTED
    };
}

types::WALEntry WAL::commitTransaction(TpcTid& request) {
    long tid = request.tid();
    auto it = transferIndex.find(tid);
    if (it == transferIndex.end()) {
        logger->debug("Error: Transaction ID " + std::to_string(tid) + " not found.");
    }

    // Retrieve the corresponding transaction
    types::WALEntry txn = getTransactionFromFile(it->second);

    std::ofstream outFile(walFile, std::ios::app);
    if (!outFile) {
        logger->debug("Error: Unable to open WAL file for writing.");
        return { -1, -1, -1, { -1, -1, -1 }, types::TransactionType::CROSS, types::TransactionStatus::NO_STATUS };
    }

    outFile << "CS_COMMIT " << tid << " " << txn.ballot_num << " " << txn.ballot_server_id << " " << txn.txn.sender << " " << txn.txn.receiver << " " << txn.txn.amount << "\n";
    outFile.close();

    // Remove the transaction from the transferIndex
    transferIndex.erase(it);

    txn.status = types::TransactionStatus::COMMITTED;
    return txn;
}

types::WALEntry WAL::abortTransaction(TpcTid& request) {
    long tid = request.tid();
    auto it = transferIndex.find(tid);
    if (it == transferIndex.end()) {
        logger->debug("Error: Transaction ID " + std::to_string(tid) + " not found.");
        return { -1, -1, -1, { -1, -1, -1 }, types::TransactionType::CROSS, types::TransactionStatus::NO_STATUS };
    }

    // Retrieve the corresponding transaction
    types::WALEntry txn = getTransactionFromFile(it->second);

    // Append ABORT message to WAL
    std::ofstream outFile(walFile, std::ios::app);
    if (!outFile) {
        logger->debug("Error: Unable to open WAL file for writing.");
        return {-1, -1};
    }

    outFile << "CS_ABORT " << tid << " " << txn.ballot_num << " " << txn.ballot_server_id << " " << txn.txn.sender << " " << txn.txn.receiver << " " << txn.txn.amount << "\n";
    outFile.close();

    // Remove the transaction from the transferIndex
    transferIndex.erase(it);

    txn.status = types::TransactionStatus::ABORTED;
    return txn;
}