#pragma once

#include "message.grpc.pb.h"
#include <atomic>
#include <string>
#include <array>

#include <iostream>
#include <grpc++/grpc++.h>

#define UNUSED(expr) do { (void)(expr); } while (0)

#include "DynHaT.h"

using namespace kilovictor;
using grpc::Status;
using grpc::ServerContext;
using grpc::Server;
using grpc::ServerBuilder;


class KVServer final : public KiloVictor::Service {
  private:
    DynHaT<std::string, NumericValue> table;
  public:
    Status getNumeric(ServerContext* context, const Key* request, NumericValue* response);
    Status setNumeric(ServerContext* context, const NumericKeyValue* request, NumericValue* response);
    void run();
};

