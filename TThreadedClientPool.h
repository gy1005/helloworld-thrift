//
// Created by yugan on 9/28/18.
//

#ifndef HELLOWORLD_TTHREADEDCLIENTPOOL_H
#define HELLOWORLD_TTHREADEDCLIENTPOOL_H


#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/stdcxx.h>

#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <mutex>


using namespace std;
using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;


// TODO: Max number of connections and wait for free connection.
// TODO: Reconnect while failure.

template <class TClient>
class ClientTuple {
public:
  ClientTuple(const string &, int, int);
  ClientTuple(const ClientTuple<TClient> &);
  ~ClientTuple();

  const shared_ptr<TClient> &getClient() const;

  int getId() const;

private:
  stdcxx::shared_ptr<TTransport> socket_;
  stdcxx::shared_ptr<TProtocol> protocol_;
  stdcxx::shared_ptr<TTransport> transport_;
  stdcxx::shared_ptr<TClient> client_;
  int id_;

};

template <class TClient>
ClientTuple<TClient>::ClientTuple(
    const string & addr,
    int port,
    int id
) : id_(id){

  socket_ = stdcxx::shared_ptr<TTransport>(new TSocket(addr, port));
  transport_ = stdcxx::shared_ptr<TTransport>(
      new TBufferedTransport(socket_));
  protocol_ = stdcxx::shared_ptr<TProtocol>(new TBinaryProtocol(transport_));
  client_ = stdcxx::shared_ptr<TClient>(new TClient(protocol_));
  try {
    transport_->open();
  } catch (TException &tx) {
    // TODO: Reconnect if fails.
    cout << "ERROR: " << tx.what() << endl;
  }
}

template <class TClient>
ClientTuple<TClient>::ClientTuple(const ClientTuple<TClient> &other) :
    socket_(other.socket_),
    protocol_(other.protocol_),
    client_(other.client_),
    transport_(other.transport_),
    id_(other.id_) {}

template <class TClient>
ClientTuple<TClient>::~ClientTuple() {
  try {
    transport_->close();
  } catch (TException &tx) {
    // TODO: Resolve closing error
    cout << "ERROR: " << tx.what() << endl;
  }
  client_.reset();
  protocol_.reset();
  transport_.reset();
  socket_.reset();
}

template<class TClient>
const shared_ptr<TClient> &ClientTuple<TClient>::getClient() const {
  return client_;
}

template<class TClient>
int ClientTuple<TClient>::getId() const {
  return id_;
}

template <class TClient>
class TThreadedClientPool {
public:
  TThreadedClientPool(int pool_size, const string &addr, int port);
  TThreadedClientPool(TThreadedClientPool<TClient> &other);

  ~TThreadedClientPool();
  shared_ptr<ClientTuple<TClient>> getClient();
  void returnClient(int client_id);




private:
  int pool_size_{};
  string addr_;
  int port_;
  vector<shared_ptr<ClientTuple<TClient>>> clients_;
  queue<int> free_list_;
  mutex mtx_;

  shared_ptr<ClientTuple<TClient>> createClientAndUse_();

};

template <class TClient>
TThreadedClientPool<TClient>::TThreadedClientPool(
    int pool_size,
    const string &addr,
    int port): mtx_(), pool_size_(pool_size), addr_(addr), port_(port) {
  for (int i = 0; i < pool_size_; ++i) {
    auto client_ptr = shared_ptr<ClientTuple<TClient>>(
        new ClientTuple<TClient>(addr, port, i));

    // Ensure the thread safety when manipulating the queues.
    mtx_.lock();
    clients_.emplace_back(client_ptr);
    free_list_.push(i);
    mtx_.unlock();
  }
}


template <class TClient>
TThreadedClientPool<TClient>::TThreadedClientPool(
    TThreadedClientPool<TClient>  &other) :
    pool_size_(other.pool_size_),
    addr_(other.addr_),
    port_(other.port_),
    clients_(other.clients_),
    free_list_(other.free_list_),
    mtx_() {}



template <class TClient>
TThreadedClientPool<TClient>::~TThreadedClientPool() {
  assert(free_list_.size() == pool_size_);
  assert(clients_.size() == pool_size_);

  while (clients_.size() > 0)
    clients_.pop_back();

}

template <class TClient>
shared_ptr<ClientTuple<TClient>> TThreadedClientPool<TClient>::
getClient() {
  int client_id;
  mtx_.lock();
  mtx_.unlock();

  shared_ptr<ClientTuple<TClient>> return_ptr;

  mtx_.lock();
  if (free_list_.size() == 0) {
    mtx_.unlock();
    return createClientAndUse_();
  }
  else {
    client_id = free_list_.front();
    free_list_.pop();
    return_ptr = clients_[client_id];
    mtx_.unlock();

    return return_ptr;
  }
}

template <class TClient>
void TThreadedClientPool<TClient>::returnClient(int client_id) {
  mtx_.lock();
  free_list_.push(client_id);
  mtx_.unlock();
}

template <class TClient>
shared_ptr<ClientTuple<TClient>> TThreadedClientPool<TClient>::
createClientAndUse_() {
  shared_ptr<ClientTuple<TClient>> client_ptr;
  mtx_.lock();
  client_ptr = shared_ptr<ClientTuple<TClient>>(
      new ClientTuple<TClient>(addr_, port_, pool_size_));
  clients_.emplace_back(client_ptr);
  pool_size_++;
  mtx_.unlock();
  return client_ptr;
}


#endif //HELLOWORLD_TTHREADEDCLIENTPOOL_H
