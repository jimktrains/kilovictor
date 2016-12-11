#include "KVServer.h"

const int TABLE_SIZE = 1023;
std::array<NumericKeyValue, TABLE_SIZE> numeric_map;
std::array<std::atomic_llong, TABLE_SIZE> numeric_map_lock;

std::atomic_llong cmdid;

std::size_t hash(Key k) {
    return std::hash<std::string>{}(k.value());
}

Status KVServer :: getNumeric(
  ServerContext* context, 
  const Key* request, 
  NumericValue* response) {
  UNUSED(context);

  auto h = hash(*request) % TABLE_SIZE;
  auto ret = numeric_map[h];
  *response = ret.value();

  return Status::OK;
}

Status KVServer :: setNumeric(
    ServerContext* context, 
    const NumericKeyValue* request, 
    NumericValue* response) {
  UNUSED(context);


  auto h = hash(request->key()) % TABLE_SIZE;

  auto des_val = request->value();
  Status s = Status(grpc::StatusCode::OUT_OF_RANGE, "Newer version available");

  auto cur_cmd_id = cmdid++;

  while( (numeric_map_lock[h].load() - cur_cmd_id) != 0) {
    std::cout << "Waiting at: " << numeric_map_lock[h].load() << " for " << cur_cmd_id << std::endl;
    // thread wait
  }
  
  NumericValue *newval = new NumericValue(numeric_map[h].value());

  std::cout << newval->version() << '\t' << des_val.version() << '\t' << (newval->version() < des_val.version() || des_val.version() == 0  ) << std::endl;
  if (newval->version() < des_val.version() || des_val.version() == 0) {

    newval->set_value(request->value().value());
    newval->set_version(cur_cmd_id);

    numeric_map[h].set_allocated_value(newval);
    // set_allocated_value may free this before
    // we use it if we release the lock first
    *response = *newval;

    s = Status::OK;
  }
	else
	{
		*response = numeric_map[h].value();
	}

  // release the lock and return
  numeric_map_lock[h]++;
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

