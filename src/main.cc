#include "message.pb.h"
#include "message.grpc.pb.h"
#include <unordered_map>
#include <atomic>
#include <string>
#include <array>

#include <iostream>
#include <grpc++/grpc++.h>

#define UNUSED(expr) do { (void)(expr); } while (0)

using namespace kilovictor;
using grpc::Status;
using grpc::ServerContext;
using grpc::Server;
using grpc::ServerBuilder;

const int TABLE_SIZE = 1023;
std::array<NumericKeyValue, TABLE_SIZE> numeric_map;
std::array<std::atomic_llong, TABLE_SIZE> numeric_map_lock;

std::atomic_llong cmdid{1};

std::size_t hash(Key k) {
    return std::hash<std::string>{}(k.value());
}

class KVImpl final : public KiloVictor::Service {
  public:
    Status getNumeric(ServerContext* context, const Key* request, NumericValue* response);
    Status setNumeric(ServerContext* context, const NumericKeyValue* request, NumericValue* response);
};

Status KVImpl :: getNumeric(
  ServerContext* context, 
  const Key* request, 
  NumericValue* response) {
  UNUSED(context);

  auto h = hash(*request) % TABLE_SIZE;
  auto ret = numeric_map[h];
  *response = ret.value();

  return Status::OK;
}

Status KVImpl :: setNumeric(
    ServerContext* context, 
    const NumericKeyValue* request, 
    NumericValue* response) {
  UNUSED(context);


  auto h = hash(request->key()) % TABLE_SIZE;

  auto des_val = request->value();
  Status s = Status(grpc::StatusCode::OUT_OF_RANGE, "Newer version available");

  auto cur_cmd_id = cmdid++;

  while(numeric_map_lock[h].fetch_sub(cur_cmd_id) != 0) {
    // thread wait
  }
  
  NumericValue *newval = new NumericValue(numeric_map[h].value());

  if (newval->version() < des_val.version()) {

    newval->set_value(request->value().value());
    newval->set_version(cur_cmd_id);

    numeric_map[h].set_allocated_value(newval);
    // set_allocated_value may free this before
    // we use it if we release the lock first
    *response = *newval;

    s = Status::OK;
 }

  // release the lock and return
  numeric_map_lock[h]++;
  return s;
}

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  KVImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}


int main(int argc, const char* argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  KVImpl srv;

  Key k;
  k.set_value("Test Key");

  NumericValue v1;
  v1.set_value(1234);
  v1.set_version(1);

  NumericValue v2;

  NumericKeyValue kv;
  *kv.mutable_key() = k;
  *kv.mutable_value() = v1;

  NumericKeyValue ret;
  auto r1 = srv.setNumeric(nullptr, &kv, &v2);
  std::cout << r1.error_code() << '\t' << v2.version() << std::endl;
  auto r2 = srv.setNumeric(nullptr, &kv, &v2);
  std::cout << r2.error_code() << '\t' << v2.version() << std::endl;

  auto r3 = srv.getNumeric(nullptr, &k, &v2);
  std::cout << r3.error_code() << '\t' << v2.value() << '\t' << v2.version() << std::endl;


  return 0;
}
