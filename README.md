## Abstract
The objective of this project is to implement a fault-tolerant distributed transaction processing system that supports a simple banking application. To this end, we partition servers into multiple clusters where each cluster maintains a data shard. Each data shard is replicated on all servers of a cluster to provide fault tolerance. The system supports two types of transactions: intra-shard and cross-shard. An intra-shard transaction accesses the data items of the same shard while a cross-shard transaction accesses the data items on multiple shards. To process intra-shard transactions the modified Paxos protocol implemented in the first project (or the Multi-Paxos protocol) should be used while the two-phase commit protocol is used to process cross-shard transactions.


## Architecture
In this project, similar to the first and second projects, you are supposed to deploy a simple banking application where clients send their transfer transactions in the form of (x, y, amt) to the servers where x is the sender, y is the receiver, and amt is the amount of money to transfer. This time, we shift our focus towards real-world applications by examining a large-scale key-value store that is partitioned across multiple clusters, with each cluster managing a distinct shard of the application’s data. The system architecture is illustrated in _Figure 1_.

<p align="center">
  <img src="https://github.com/user-attachments/assets/6ec30b18-769a-4951-84c7-435213b7218c">
  <br>
  <em>Figure 1: System Architecture</em>
</p>

As shown, the data is divided into three shards: D1, D2, and D3. The system comprises a total of nine servers, labeled S1 through S9, organized into three clusters: C1, C2, and C3. Each data shard Di is replicated across all servers within its respective cluster Ci to ensure fault tolerance, operating under the assumption that servers adhere to a fail-stop failure model, where at most one server in each cluster may be faulty at any given time.

We assume that clients have access to the shard mapping, which informs them about the data items stored in each cluster of servers. Clients can initiate both intra-shard and cross-shard transactions.

For an intra-shard transaction, a client sends its request to a randomly selected server within the relevant cluster (the cluster that holds the requested data items). The servers in that cluster then execute the modified (or Multi-) Paxos protocol to reach consensus on the order of the request. It’s important to note that, unlike in the first project, consensus is required for each individual transaction, as there is no local log maintained.

In the case of a cross-shard transaction, the client sends its request to a randomly chosen server in each of the relevant clusters. In this scenario, the client acts as the coordinator for the two-phase commit protocol executed among the involved clusters.

_Figure 2_ shows an example of the system where a dataset consisting of 6 data items is partitioned into 3 different data shards. Each data shard is then maintained by (replicated on) a cluster of servers.

<p align="center">
  <img src="https://github.com/user-attachments/assets/043e8b56-61d4-42cb-b208-5fd045ed809e">
  <br>
  <em>Figure 2: System Architecture</em>
</p>

## Intra-shard transactions: Modified Paxos
To process intra-shard transactions, we utilize a modified version of the Paxos protocol from the first project, with some adjustments. The key distinction in this project is that we require consensus for each individual transaction, and servers do not maintain any local transaction logs. This design enhances fault tolerance, as all transactions are replicated across all servers within a cluster. If you implemented Multi-Paxos in the initial project, you may choose to use it in place of the modified Paxos protocol to achieve better performance.

Here is a brief overview of the modified Paxos protocol. A client sends an intra-shard request (x, y, amt) to a randomly selected server in the cluster that holds both data items x and y. The server first checks two conditions: (1) there are no locks on data items x and y, and (2) the balance of x is at least equal to amt.
If both conditions are met, the server initiates the consensus protocol by sending a prepare message with ballot number n to all other servers in the cluster. As a reminder, servers must synchronize when sending the prepare message by sharing their states, which consist of either the number of committed transaction blocks or the ballot number of the latest committed block along with its associated transaction. It is important to note that in our project, every block corresponds to a single transaction.

When a server receives a prepare message from a proposer server, it first compares its current state (e.g., datastore) with the state provided by the proposer. If the states do not match, the server takes the necessary steps to synchronize with the proposer or vice versa. If the states are consistent, the server responds by sending a promise message ⟨ACK, n, AcceptN um, AcceptV al⟩ back to the proposer, where AcceptNum and AcceptVal indicate the latest accepted (but not yet committed) transaction and the ballot number in which it was accepted. Unlike in Project 1, there is no need to send local logs, as this component is not applicable in the current project.
Once the proposer receives a sufficient number of promise messages, it enters the accept phase and finally commits the request if all conditions are satisfied. The leader needs to send a message back to the client letting the client know that the transaction has been committed. The client waits for a reply from the leader to accept the result. For the sake of simplicity, you do not need to use timers on the client side or resend the requests.

## Cross-shard transactions: Two-phase commit protocol
To process cross-shard transactions, we need to implement the two-phase commit (2PC) protocol across the clusters involved in each transaction. Since our focus is on transfer transactions, each cross-shard transaction involves two distinct shards. In this configuration, the client acts as the coordinator for the 2PC protocol.

The client initiates a cross-shard request (x, y, amt) by contacting: (1) a randomly selected server from the cluster that holds data item x, and (2) a randomly selected server from the cluster that holds data item y.
Each of these servers then starts the modified Paxos protocol to achieve consensus on the transaction within their respective clusters, treating the transaction as an intra-shard transaction. While reaching consensus, the cluster responsible for data item x must verify that the balance of x is at least equal to amt.

To prevent concurrent updates to the data items, all servers in both clusters acquire locks (using two-phase locking) on accessed items (item x in one cluster and item y in the other). If another transaction has already secured a lock on either item, the cluster will decide to simply abort the cross-shard transaction.

While the servers within the clusters reach an agreement on the transaction, they need to also update their write-ahead logs (WAL), allowing them to revert changes in the case of an abort.
The leader of each cluster sends either a prepared or abort message back to the transaction coordinator. Upon receiving prepared messages from both involved clusters, the client (coordinator) sends a commit message to all servers in both clusters. Conversely, if any cluster aborts the transaction or if the transaction coordinator times out, the coordinator will issue an abort message to all servers in both clusters.

If the outcome is a commit, each server releases its locks. If the outcome is an abort or if a timeout occurs, the server will use the WAL to redo the executed operations before releasing the locks. In either scenario, the server sends an Ack message back to the coordinating client.

_Figure 3_ demonstrates the flow of a cross-shard transaction between clusters C1 and C2.

<p align="center">
  <img src="https://github.com/user-attachments/assets/3b997256-ee5a-4d2f-bd87-85d0740ccb6e">
  <br>
  <em>Figure 3: Cross-shard Transactions</em>
</p>



## Setup instructions

### Install gRPC

Instructions to install gRPC on MacOS are given below:

For installation instructions on Linux or Windows, please follow the steps here: https://grpc.io/docs/languages/cpp/quickstart/

1. Choose a directory to hold locally installed packages
```
$ export MY_INSTALL_DIR=$HOME/.local
```

2. Ensure that the directory exists
```
$ mkdir -p $MY_INSTALL_DIR
```

3. Add the local `bin` folder to path variable
```
$ export PATH="$MY_INSTALL_DIR/bin:$PATH"
```

4. Install cmake
```
$ brew install cmake
```

5. Install other required tools
```
$ brew install autoconf automake libtool pkg-config
```

6. Clone the gRPC repo
```
$ git clone --recurse-submodules -b v1.66.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
```

7. Build and install gRPC and Protocol Buffers
```
$ cd grpc
$ mkdir -p cmake/build
$ pushd cmake/build
$ cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
      ../..
$ make -j 4
$ make install
$ popd
```

### Clone the project repository

1. Clone the project repository which has the source code for running paxos.
```
$ git clone git@github.com:avjotsingh/2pc-avjotsingh.git
```


### Build the project

1. Build the project. This should not take too long
```
$ cd pbft-avjotsingh
$ mkdir build
$ cd ./build
$ cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ..
$ cmake --build .
```

2. Run the project
The driver program needs a CSV with the following format.

<p align="center">
  <img src="https://github.com/user-attachments/assets/4dafcffe-2565-41bf-9332-ba885e373e87">
</p>

An example CSV file is available under the test directory.
```
$ cd build
$ ./driver <num_cluster> <cluster_size> <csv_filepath>
```
