import asyncio
import logging

import grpc
import tpc_pb2
import tpc_pb2_grpc
from google.protobuf.empty_pb2 import Empty

async def run() -> None:
    channel1 = grpc.aio.insecure_channel("localhost:50051")
    channel2 = grpc.aio.insecure_channel("localhost:50052")
    channel3 = grpc.aio.insecure_channel("localhost:50053")
    
    stub1 = tpc_pb2_grpc.TpcServerStub(channel1)
    stub2 = tpc_pb2_grpc.TpcServerStub(channel2)
    stub3 = tpc_pb2_grpc.TpcServerStub(channel3)

    # await stub1.Transfer(
    #     tpc_pb2.TransferReq(
    #         txn = tpc_pb2.Transaction(sender="A", receiver="B", amount=10),
    #         tid = 1
    #     )
    # )

    # await stub2.Transfer(
    #     tpc_pb2.TransferReq(
    #         txn = tpc_pb2.Transaction(sender="B", receiver="C", amount=20),
    #         tid = 2
    #     )
    # )

    await stub3.Transfer(
        tpc_pb2.TransferReq(
            txn = tpc_pb2.Transaction(sender="C", receiver="A", amount=5),
            tid = 3
        )
    )


    response = await stub1.Logs(Empty())
    print("Logs S1: " + str(response))

    # response = await stub2.Logs(Empty())
    # print("Logs S2: " + str(response))

    response = await stub3.Logs(Empty())
    print("Logs S3: " + str(response))

if __name__ == "__main__":
    logging.basicConfig()
    asyncio.run(run())