#include "KVServer.h"

Status KVServer :: getNumeric(
  ServerContext* context, 
  const Key* request, 
  NumericValue* response) {
  UNUSED(context);

  auto ret = table[request->value()];
  auto s = Status(grpc::StatusCode::NOT_FOUND, "Not Found");
  if (ret) {
    *response = ret.value_or(NumericValue());
    s = Status::OK;
  }

  return s;
}

Status KVServer :: setNumeric(
    ServerContext* context, 
    const NumericKeyValue* request, 
    NumericValue* response) {
  UNUSED(context);


  auto s = Status(grpc::StatusCode::OUT_OF_RANGE, "out of range");
  if (table.insert(request->key().value(), request->value()))
  {
    *response = request->value();
    s = Status::OK;
  }


  
  return s;
}

void KVServer :: run() {
  std::string server_address("0.0.0.0:50051");
  KVServer service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

