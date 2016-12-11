#include "KVServer.h"

int main(int argc, const char* argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  KVServer kvs;

  kvs.run();

  return 0;
}
