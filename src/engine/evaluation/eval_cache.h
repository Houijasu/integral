#ifndef INTEGRAL_EVAL_CACHE_H_
#define INTEGRAL_EVAL_CACHE_H_

#include "../../utils/types.h"
#include <cstring>

namespace eval {

// Simple evaluation cache to avoid recomputing NNUE evaluations
class EvalCache {
 public:
  struct Entry {
    U64 key;
    Score score;
  };
  
  static constexpr size_t kCacheSize = 1 << 16;  // 64K entries
  static constexpr size_t kCacheMask = kCacheSize - 1;
  
  EvalCache() {
    Clear();
  }
  
  void Clear() {
    std::memset(entries_, 0, sizeof(entries_));
  }
  
  [[nodiscard]] inline bool Probe(U64 key, Score& score) const {
    const size_t index = key & kCacheMask;
    const Entry& entry = entries_[index];
    
    if (entry.key == key) {
      score = entry.score;
      return true;
    }
    return false;
  }
  
  inline void Store(U64 key, Score score) {
    const size_t index = key & kCacheMask;
    Entry& entry = entries_[index];
    
    entry.key = key;
    entry.score = score;
  }
  
 private:
  alignas(64) Entry entries_[kCacheSize];
};

}  // namespace eval

#endif  // INTEGRAL_EVAL_CACHE_H_