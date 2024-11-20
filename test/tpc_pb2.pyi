from google.protobuf import empty_pb2 as _empty_pb2
from google.protobuf.internal import containers as _containers
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from typing import ClassVar as _ClassVar, Iterable as _Iterable, Mapping as _Mapping, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class Transaction(_message.Message):
    __slots__ = ("sender", "receiver", "amount")
    SENDER_FIELD_NUMBER: _ClassVar[int]
    RECEIVER_FIELD_NUMBER: _ClassVar[int]
    AMOUNT_FIELD_NUMBER: _ClassVar[int]
    sender: str
    receiver: str
    amount: int
    def __init__(self, sender: _Optional[str] = ..., receiver: _Optional[str] = ..., amount: _Optional[int] = ...) -> None: ...

class TransferReq(_message.Message):
    __slots__ = ("txn", "tid")
    TXN_FIELD_NUMBER: _ClassVar[int]
    TID_FIELD_NUMBER: _ClassVar[int]
    txn: Transaction
    tid: int
    def __init__(self, txn: _Optional[_Union[Transaction, _Mapping]] = ..., tid: _Optional[int] = ...) -> None: ...

class TransferRes(_message.Message):
    __slots__ = ("ack", "tid")
    ACK_FIELD_NUMBER: _ClassVar[int]
    TID_FIELD_NUMBER: _ClassVar[int]
    ack: bool
    tid: int
    def __init__(self, ack: bool = ..., tid: _Optional[int] = ...) -> None: ...

class BalanceReq(_message.Message):
    __slots__ = ("client",)
    CLIENT_FIELD_NUMBER: _ClassVar[int]
    client: str
    def __init__(self, client: _Optional[str] = ...) -> None: ...

class BalanceRes(_message.Message):
    __slots__ = ("amount",)
    AMOUNT_FIELD_NUMBER: _ClassVar[int]
    amount: int
    def __init__(self, amount: _Optional[int] = ...) -> None: ...

class LogRes(_message.Message):
    __slots__ = ("txns",)
    TXNS_FIELD_NUMBER: _ClassVar[int]
    txns: _containers.RepeatedCompositeFieldContainer[Transaction]
    def __init__(self, txns: _Optional[_Iterable[_Union[Transaction, _Mapping]]] = ...) -> None: ...

class Ballot(_message.Message):
    __slots__ = ("num", "server_id")
    NUM_FIELD_NUMBER: _ClassVar[int]
    SERVER_ID_FIELD_NUMBER: _ClassVar[int]
    num: int
    server_id: int
    def __init__(self, num: _Optional[int] = ..., server_id: _Optional[int] = ...) -> None: ...

class PrepareReq(_message.Message):
    __slots__ = ("ballot", "last_committed")
    BALLOT_FIELD_NUMBER: _ClassVar[int]
    LAST_COMMITTED_FIELD_NUMBER: _ClassVar[int]
    ballot: Ballot
    last_committed: int
    def __init__(self, ballot: _Optional[_Union[Ballot, _Mapping]] = ..., last_committed: _Optional[int] = ...) -> None: ...

class PrepareRes(_message.Message):
    __slots__ = ("ack", "ballot", "accept_num", "accept_val", "last_committed")
    ACK_FIELD_NUMBER: _ClassVar[int]
    BALLOT_FIELD_NUMBER: _ClassVar[int]
    ACCEPT_NUM_FIELD_NUMBER: _ClassVar[int]
    ACCEPT_VAL_FIELD_NUMBER: _ClassVar[int]
    LAST_COMMITTED_FIELD_NUMBER: _ClassVar[int]
    ack: bool
    ballot: Ballot
    accept_num: Ballot
    accept_val: Transaction
    last_committed: int
    def __init__(self, ack: bool = ..., ballot: _Optional[_Union[Ballot, _Mapping]] = ..., accept_num: _Optional[_Union[Ballot, _Mapping]] = ..., accept_val: _Optional[_Union[Transaction, _Mapping]] = ..., last_committed: _Optional[int] = ...) -> None: ...

class AcceptReq(_message.Message):
    __slots__ = ("ballot", "txn")
    BALLOT_FIELD_NUMBER: _ClassVar[int]
    TXN_FIELD_NUMBER: _ClassVar[int]
    ballot: Ballot
    txn: Transaction
    def __init__(self, ballot: _Optional[_Union[Ballot, _Mapping]] = ..., txn: _Optional[_Union[Transaction, _Mapping]] = ...) -> None: ...

class AcceptRes(_message.Message):
    __slots__ = ("ack", "ballot")
    ACK_FIELD_NUMBER: _ClassVar[int]
    BALLOT_FIELD_NUMBER: _ClassVar[int]
    ack: bool
    ballot: Ballot
    def __init__(self, ack: bool = ..., ballot: _Optional[_Union[Ballot, _Mapping]] = ...) -> None: ...

class CommitReq(_message.Message):
    __slots__ = ("ballot",)
    BALLOT_FIELD_NUMBER: _ClassVar[int]
    ballot: Ballot
    def __init__(self, ballot: _Optional[_Union[Ballot, _Mapping]] = ...) -> None: ...

class SyncReq(_message.Message):
    __slots__ = ("last_committed",)
    LAST_COMMITTED_FIELD_NUMBER: _ClassVar[int]
    last_committed: int
    def __init__(self, last_committed: _Optional[int] = ...) -> None: ...

class SyncRes(_message.Message):
    __slots__ = ("ack", "txns", "last_committed_ballot")
    ACK_FIELD_NUMBER: _ClassVar[int]
    TXNS_FIELD_NUMBER: _ClassVar[int]
    LAST_COMMITTED_BALLOT_FIELD_NUMBER: _ClassVar[int]
    ack: bool
    txns: _containers.RepeatedCompositeFieldContainer[Transaction]
    last_committed_ballot: Ballot
    def __init__(self, ack: bool = ..., txns: _Optional[_Iterable[_Union[Transaction, _Mapping]]] = ..., last_committed_ballot: _Optional[_Union[Ballot, _Mapping]] = ...) -> None: ...
