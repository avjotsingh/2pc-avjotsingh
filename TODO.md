- Client maintains state about which clusters are currently executing a transaction (not needed since the client is not closed loop)
- Client checks if a transaction involves a cluster which is already executing something. If yes, then it waits otherwise it issues the transaction (not needed)
- abort/commit cross shard transaction after receiving a decision from client (done)
- in promise messages, select the highest accept num and accept val (can be ignored since only a single paxos instance at a time)
- implement coordinator on client side (done)
- implement the application interface (done)
- Set state from WAL on server startup (not needed since servers don't shut down)
- update account balances while synchronizing (done)
- move server startup inside driver code (done)
- print which transaction was aborted on the client side (won't do)
- test with configurable clusters sizes and clusters (done)
- servers are not terminating correctly on exit