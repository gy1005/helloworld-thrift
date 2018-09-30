//
// Created by yugan on 9/28/18.
//

#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PlatformThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TNonblockingServerSocket.h>
#include <thrift/TToString.h>
#include <thrift/stdcxx.h>

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <getopt.h>
#include <thread>

#include <execinfo.h>

#include "gen-cpp/HelloworldService.h"
#include "gen-cpp/TransferService.h"
#include "TThreadedClientPool.h"

using namespace std;

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

using namespace helloworld;

#define CHILD_PORT 9090
#define CLIENT_POOL_SIZE 4

class TransferServiceHandler: public TransferServiceIf {
public:
  explicit TransferServiceHandler(
      shared_ptr<TThreadedClientPool<HelloworldServiceClient>> &client_pool
  ) : client_pool_(client_pool) {
  };

  ~TransferServiceHandler() override = default;

  void transfer(string &return_str) override {
    auto client_tuple_ptr = client_pool_->getClient();
    client_tuple_ptr->getClient()->getHelloworld(return_str);
    client_pool_->returnClient(client_tuple_ptr->getId());

  }

private:
  shared_ptr<TThreadedClientPool<HelloworldServiceClient>> client_pool_;
};

void startServer(TServer &server) {
  cout << "Starting the server..." << endl;
  server.serve();
  cout << "Done." << endl;
}


int testConnect() noexcept(false) {
  stdcxx::shared_ptr<TTransport> socket(new TSocket("localhost", CHILD_PORT));
  stdcxx::shared_ptr<TTransport> transport;

  transport = stdcxx::shared_ptr<TTransport>(new TBufferedTransport(socket));
  stdcxx::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  HelloworldServiceClient client(protocol);
  while (true) {
    try {
      transport->open();
      transport->close();
      break;
    } catch (TException &tx) {
      cout << "TestConnect ERROR: " << tx.what() << endl;
      this_thread::sleep_for(chrono::seconds(1));
    }
  }

}

int main(int argc, char *argv[]) {
  // Test if the child server is ready
  testConnect();

  auto client_pool = new TThreadedClientPool<HelloworldServiceClient> (
      CLIENT_POOL_SIZE,
      "localhost",
      CHILD_PORT
  );

  auto client_pool_ptr = shared_ptr<TThreadedClientPool<
      HelloworldServiceClient>>(client_pool);

  TThreadedServer server (
      stdcxx::make_shared<TransferServiceProcessor>(
          stdcxx::make_shared<TransferServiceHandler>(client_pool_ptr)),
      stdcxx::make_shared<TServerSocket>(9091),
      stdcxx::make_shared<TBufferedTransportFactory>(),
      stdcxx::make_shared<TBinaryProtocolFactory>()
  );
  startServer(server);


}

