import sys
sys.path.append('gen-py')

from helloworld import HelloworldService, TransferService

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

import time

def main():
  # Make socket
  transport = TSocket.TSocket('localhost', 9091)

  # Buffering is critical. Raw sockets are very slow
  transport = TTransport.TBufferedTransport(transport)

  # Wrap in a protocol
  protocol = TBinaryProtocol.TBinaryProtocol(transport)

  # Create a client to use the protocol encoder
  client = TransferService.Client(protocol)

  # Connect!
  transport.open()
  print("conn open")
  res = client.transfer()
  print(res)

  transport.close()

if __name__ == '__main__':
  try:
    main()
  except Thrift.TException as tx:
    print('%s' % tx.message)