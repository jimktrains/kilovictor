#pragma once

#include "message.grpc.pb.h"
#include <atomic>
#include <string>
#include <array>
//#include <optional>
#include <experimental/optional>

#include <iostream>
#include <grpc++/grpc++.h>

#define UNUSED(expr) do { (void)(expr); } while (0)

#define LOCK_SIZE 256
#define PARTITION_COUNT 8
#define TABLE_SIZE 1023

struct HashParts {
  size_t hash;
  size_t partition;
  size_t rhash;
  size_t lock_idx;
};

template<typename KEY>
std::size_t hash(KEY k) {
    return std::hash<KEY>{}(k);
}

using std::experimental::optional;

/**
 * Goal: Concurrent access to a hash table using a consistent  hashing
 * scheme.
 */
// Eventually, this should mimic std::map's interface
template<typename KEY, typename VALUE>
class DynHaT {
  private:

    const static int partition_lower_bound[PARTITION_COUNT];
    const static int partition_upper_bound[PARTITION_COUNT];

    std::atomic_uint subtable_count;
    std::array<std::unique_ptr<std::array<optional<std::pair<KEY, VALUE>>, TABLE_SIZE>>, PARTITION_COUNT> table = {};
    std::array<std::atomic_ullong, LOCK_SIZE> lock = {};
    std::array<std::atomic_ullong, LOCK_SIZE> counter = {};

    HashParts hashparts(KEY key);
  public:
    DynHaT();
    const optional<VALUE> operator[](KEY key);
    bool insert(KEY key, VALUE value);
};

template<typename KEY, typename VALUE>
DynHaT<KEY, VALUE> :: DynHaT() {
  table[0] = std::make_unique<std::array<optional<std::pair<KEY, VALUE>>, TABLE_SIZE>>(std::array<optional<std::pair<KEY, VALUE>>, TABLE_SIZE>());
}

/*
index numerator denominator eigth upper_bound
  0       0         1        0        8
  1       1         2        4        8
  2       1         4        2        4
  3       3         4        6        8
  4       1         8        1        2
  5       3         8        3        4
  6       5         8        5        6
  7       7         8        7        8
*/
template<typename KEY, typename VALUE>
constexpr int DynHaT<KEY, VALUE> :: partition_lower_bound[PARTITION_COUNT] = {0,4,2,6,1,3,5,7};
template<typename KEY, typename VALUE>
constexpr int DynHaT<KEY, VALUE> :: partition_upper_bound[PARTITION_COUNT] = {8,8,4,8,2,4,6,8};

template<typename KEY, typename VALUE>
HashParts DynHaT<KEY, VALUE> :: hashparts(KEY key) {
  auto h = hash(key);

  // So this is a bit hacky/hard coded
  //
  // Pulls the top 3 bits all the way to the bottom
  // 10101010 -> 00000101
  auto partition = h >> ((sizeof(h) * 8) - 3);
  // Drops the top 3 bits
  // 10101010 -> 00001010
  auto rhash = (h << 3) >> 3;

  auto lidx = h % LOCK_SIZE;


  return HashParts{h, partition, rhash, lidx};
}

template<typename KEY, typename VALUE>
const optional<VALUE> DynHaT<KEY, VALUE> :: operator[](KEY key) {
  auto hashval = hashparts(key);

  int partition = hashval.partition;
  for(long long i = subtable_count.load(); 0 <= i; i--)
  {
    if (   partition <  partition_upper_bound[i] 
        && partition >= partition_lower_bound[i]) {

      if (table[i]) {
        auto ctable = *table[i];
        auto idx = hashval.rhash % ctable.size();
        auto val = ctable[idx];
        if (val) {
          if (val.value_or(std::pair<KEY, VALUE>()).first == key) {
            return optional<VALUE>(val.value_or(std::pair<KEY, VALUE>()).second);
          }
        }
      }
      else {
        std::cerr << "Table " << i << " does not exist!!!!" << std::endl;
      }
    }
  }
  return optional<VALUE>();
}

// So, I should actually figure out how to store a value when its
// space is occupied
template<typename KEY, typename VALUE>
bool DynHaT<KEY, VALUE> :: insert(KEY key, VALUE value) {
  auto hashval = hashparts(key);

  int partition = hashval.partition;

  //@TODO make into RAII style lock
  auto cur_cmd_id = counter[hashval.lock_idx]++;
  while( (cur_cmd_id - lock[hashval.lock_idx].load()) != 0) {
    // thread wait
  }

  bool inserted = false;
  for(auto i = subtable_count.load(); 0 <= i; i--)
  {
    if (   partition <  partition_upper_bound[i] 
        && partition >= partition_lower_bound[i]) {

      auto ctable = *table[i];
      auto idx = hashval.rhash % ctable.size();

      auto val = ctable[idx];
      if (!val) {
        ctable[idx] = std::pair<KEY, VALUE>{key, value};
        inserted = true;
      } 
      break;
    }
  }
  // @TODO see L111
  lock[hashval.lock_idx]++;
  return inserted;
}
