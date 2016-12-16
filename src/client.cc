#include <iostream>
#include "KVClient.h"
#include <random>
#include <chrono>
#include <thread>

std::atomic<long long> timer_sum;
std::atomic<long long> timer_count;

std::random_device rd;     // only used once to initialise (seed) engine
std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
std::uniform_int_distribution<int> uni(1000,1100); // guaranteed unbiased

std::mutex g_i_mutex;
int test_runs = 10000;

void runTest() {
  auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id()) % 10;
	Key k;
	k.set_value("This is a test" + std::to_string(thread_id));

	int i = 0;
	NumericKeyValue nkv;
	*(nkv.mutable_key()) = k;

  std::chrono::milliseconds start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

  KVClient greeter(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
  for(int t = 0; t < test_runs; t++) {
		NumericValue v;
		v.set_value(thread_id + (i%100));
    v.set_version(0);

		*(nkv.mutable_value()) = v;

		NumericValue reply = greeter.setNumeric(nkv);
		reply = greeter.getNumeric(k);
		nkv.mutable_value()->set_version(reply.version() + 1);

    i++;
	}
  std::chrono::milliseconds end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

  timer_sum += (end_ms - start_ms).count();
  timer_count++;
}

int main(int argc, char** argv) {
  UNUSED(argc);
  UNUSED(argv);

  int thread_count = 5;
  if (1 < argc) {
    thread_count = std::atoi(argv[1]);
  }
  if (2 < argc) {
    test_runs = std::atoi(argv[2]);
  }

  std::cout << "Running " << thread_count << " threads; " << test_runs << " iterations each" << std::endl;

  std::vector<std::thread> threads;

  std::chrono::milliseconds start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
  for(int i = 0; i < thread_count; i++) {
    threads.push_back(std::thread(runTest));
  }

  for(auto&& t : threads) {
    t.join();
  }
  std::chrono::milliseconds end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());


  auto avg_call = (static_cast<double>(timer_sum.load())/timer_count.load()/test_runs);
  std::cout << "Ran " << threads.size() << " tests in " 
            << (end_ms - start_ms).count() << "ms" << std::endl;
  std::cout << timer_sum.load() << "/" << timer_count.load() 
            << "/" << test_runs << " = " 
            << avg_call << "ms" << std::endl;
  std::cout << (static_cast<double>(timer_sum.load()) / (end_ms - start_ms).count()) << std::endl;

  return 0;
}
