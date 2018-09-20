//
// Created by yugan on 9/18/18.
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

#include "gen-cpp/HelloworldService.h"

using namespace std;

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

using namespace helloworld;

class HelloworldHandler : public HelloworldServiceIf {
public:
  HelloworldHandler() = default;

  ~HelloworldHandler() override = default;;

  void getHelloworld(string &return_str) override {
    return_str = "Helloworld";
  }
};

class HelloworldCloneFactory : virtual public HelloworldServiceIfFactory {
public:
  ~HelloworldCloneFactory() override = default;

  HelloworldServiceIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) override {
      stdcxx::shared_ptr<TSocket> sock = stdcxx::dynamic_pointer_cast<TSocket>(connInfo.transport);
      cout << "Incoming connection\n";
      cout << "\tSocketInfo: "  << sock->getSocketInfo() << "\n";
      cout << "\tPeerHost: "    << sock->getPeerHost() << "\n";
      cout << "\tPeerAddress: " << sock->getPeerAddress() << "\n";
      cout << "\tPeerPort: "    << sock->getPeerPort() << "\n";
      return new HelloworldHandler;
  }

  void releaseHandler(HelloworldServiceIf *handler) override {
      delete handler;
  }
};

void startServer(TServer &server) {
  cout << "Starting the server..." << endl;
  server.serve();
  cout << "Done." << endl;
}



int main(int argc, char *argv[]) {

  int flags, opt;
  flags = 0;
  size_t n_threads = 4;

  enum ServerType {simple, threaded, threadpool, nonblocking};
  ServerType serverType = simple;

  while ((opt = getopt(argc, argv, "s:t:")) != -1) {
    switch (opt) {
      case 's':
        cout<<optarg<<endl;
        if (!strcmp(optarg, "threaded")) {
          serverType = threaded;
        } else if (!strcmp(optarg, "threadpool")) {
          serverType = threadpool;
        } else if (!strcmp(optarg, "nonblocking")) {
          serverType = nonblocking;
        } else {                        // simple server by default
          serverType = simple;
        }
        break;
      case 't':
        n_threads = atoi(optarg);
        break;
      default:  /* '?' */
        fprintf(stderr, "Usage: %s [-t number of threads] [-s] server type\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (serverType == simple) {
    TSimpleServer server(
        stdcxx::make_shared<HelloworldServiceProcessor>(stdcxx::make_shared<HelloworldHandler>()),
        stdcxx::make_shared<TServerSocket>(9090),
        stdcxx::make_shared<TBufferedTransportFactory>(),
        stdcxx::make_shared<TBinaryProtocolFactory>());
    startServer(server);
  }
  
  else if (serverType == threaded) {
    TThreadedServer server (
      stdcxx::make_shared<HelloworldServiceProcessor>(stdcxx::make_shared<HelloworldHandler>()),
      stdcxx::make_shared<TServerSocket>(9090),
      stdcxx::make_shared<TBufferedTransportFactory>(),
      stdcxx::make_shared<TBinaryProtocolFactory>()
    );
    startServer(server);
  }
  else if (serverType == threadpool) {
    stdcxx::shared_ptr<ThreadManager> threadManager =
        ThreadManager::newSimpleThreadManager(n_threads);
    threadManager->threadFactory(stdcxx::make_shared<PlatformThreadFactory>());
    threadManager->start();

    // This server allows "workerCount" connection at a time, and reuses threads
    TThreadPoolServer server(
        stdcxx::make_shared<HelloworldServiceProcessorFactory>(stdcxx::make_shared<HelloworldCloneFactory>()),
        stdcxx::make_shared<TServerSocket>(9090),
        stdcxx::make_shared<TBufferedTransportFactory>(),
        stdcxx::make_shared<TBinaryProtocolFactory>(),
        threadManager
    );
    startServer(server);
  }
  else {
    stdcxx::shared_ptr<HelloworldHandler> handler(new HelloworldHandler());

    stdcxx:: shared_ptr<TProcessor> processor(new HelloworldServiceProcessor(handler));
    stdcxx::shared_ptr<TNonblockingServerSocket> serverTransport(new TNonblockingServerSocket(9090));
    stdcxx::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    stdcxx::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());


    stdcxx::shared_ptr<ThreadManager> threadManager =
        ThreadManager::newSimpleThreadManager(n_threads);
    threadManager->threadFactory(stdcxx::make_shared<PlatformThreadFactory>());
    threadManager->start();

    TNonblockingServer server (
        processor, protocolFactory,
        serverTransport,
        threadManager);
    cout<<"Starting non-blocking server"<<endl;
    startServer(server);
  }
  return 0;
}