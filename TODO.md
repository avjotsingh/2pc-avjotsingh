- Get the basic consensus within a cluster working
- Client maintains state about which clusters are currently executing a transaction
- Client checks if a transaction involves a cluster which is already executing something. If yes, then it waits otherwise it issues the transaction
- 

- WAL should be replicated within a cluster
- WAL should record all transactions (intrashard + cross-shard)
- abort/commit cross shard transaction after receiving a decision from client
- updatE WAL on replicas during paxos
- in promise messages, select the highest accept num and accept val