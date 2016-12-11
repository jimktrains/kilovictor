#include <iostream>
#include "KVClient.h"

int main(int argc, char** argv) {

  auto pid = getpid();
	auto p = pid * 100;
  KVClient greeter(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

	Key k;
	k.set_value("This is a test");

	int i = 0;
	NumericKeyValue nkv;
	*(nkv.mutable_key()) = k;
	while(greeter.isConnected()) {
		NumericValue v;
		v.set_value(p + (i%100));
    v.set_version(0);

		*(nkv.mutable_value()) = v;

		NumericValue reply = greeter.setNumeric(nkv);
		std::cout << pid << '\t' << " Set: " << reply.value() << '/' << reply.version() << std::endl;

		reply = greeter.getNumeric(k);
		std::cout << pid << '\t' << " Got: " << reply.value() << '/' << reply.version() << std::endl;

		nkv.mutable_value()->set_version(reply.version() + 1);
	}

  return 0;
}
