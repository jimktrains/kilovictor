#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "message.grpc.pb.h"

#include <sys/types.h>
#include <unistd.h>

#define UNUSED(expr) do { (void)(expr); } while (0)

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using namespace kilovictor;

class KVClient{
 public:
  KVClient(std::shared_ptr<Channel> channel)
      : stub_(KiloVictor::NewStub(channel)), channel_(channel) {}

  // Assambles the client's payload, sends it and presents the response back
  // from the server.
  NumericValue getNumeric(const Key& k) {
    // Data we are sending to the server.

    // Container for the data we expect from the server.
    NumericValue reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->getNumeric(&context, k, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return NumericValue();
    }
  }

  NumericValue setNumeric(const NumericKeyValue& k) {
    // Data we are sending to the server.

    // Container for the data we expect from the server.
    NumericValue reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->setNumeric(&context, k, &reply);

    // Act upon its status.
    if (status.ok()) {
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
		return reply;
  }

	bool isConnected()
	{
    auto x = channel_->GetState(false);

    std::cout << "Connection: " << x << std::endl;
    return true;
	}

 private:
  std::unique_ptr<KiloVictor::Stub> stub_;
  std::shared_ptr<Channel> channel_;
};
