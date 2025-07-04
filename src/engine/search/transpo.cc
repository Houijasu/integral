#include "transpo.h"

#include <thread>

#include "../evaluation/evaluation.h"

namespace search {

[[nodiscard]] TranspositionTableEntry *TranspositionTable::Probe(
    const U64 &key) {
  auto &cluster = (*this)[key];
  const U16 key16 = static_cast<U16>(key);
  
  // Pre-calculate quality scores for all entries
  int qualities[kTTClusterSize];
  int min_quality = INT_MAX;
  int min_quality_idx = 0;
  
  // First pass: find exact match or empty slot, calculate qualities
  for (int i = 0; i < kTTClusterSize; i++) {
    const auto entry = &cluster.entries[i];
    
    // Fast path: exact key match or empty slot
    if (entry->key == 0 || entry->key == key16) {
      return entry;
    }
    
    // Calculate quality score for replacement decision
    qualities[i] = entry->depth - 8 * GetAgeDelta(entry);
    if (qualities[i] < min_quality) {
      min_quality = qualities[i];
      min_quality_idx = i;
    }
  }
  
  // No exact match found, return lowest quality entry
  return &cluster.entries[min_quality_idx];
}

void TranspositionTable::Save(TranspositionTableEntry *old_entry,
                              TranspositionTableEntry new_entry,
                              const U64 &key,
                              I32 ply,
                              bool in_pv) {
  if (new_entry.move || !old_entry->CompareKey(key)) {
    old_entry->move = new_entry.move;
  }

  if (!old_entry->CompareKey(key) ||
      new_entry.flag == TranspositionTableEntry::kExact ||
      new_entry.depth + 3 + 2 * in_pv >= old_entry->depth ||
      old_entry->age != age_) {
    new_entry.age = age_;

    old_entry->key = static_cast<U16>(key);
    old_entry->score =
        TranspositionTableEntry::CorrectScore(new_entry.score, -ply);
    old_entry->depth = new_entry.depth;
    old_entry->age = new_entry.age;
    old_entry->flag = new_entry.flag;
    old_entry->was_in_pv = new_entry.was_in_pv;
    old_entry->static_eval = new_entry.static_eval;
  }
}

U32 TranspositionTable::GetAgeDelta(
    const TranspositionTableEntry *entry) const {
  return (kMaxTTAge + age_ - entry->age) % kMaxTTAge;
}

void TranspositionTable::Age() {
  age_ = (age_ + 1) % kMaxTTAge;
}

int TranspositionTable::HashFull() const {
  int count = 0;
  for (int i = 0; i < 1000; i++) {
    count +=
        std::ranges::count_if(table_[i].entries, [this](const auto &entry) {
          return entry.age == age_ && entry.key != 0 &&
                 entry.score != kScoreNone;
        });
  }
  return count / kTTClusterSize;
}

void TranspositionTable::Clear(int num_threads) {
  const std::size_t chunks = (table_size_ + num_threads - 1) / num_threads;

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([i, chunks, this]() {
      const std::size_t clear_index = chunks * i;
      const std::size_t clear_size =
          std::min(chunks, table_size_ - clear_index) *
          sizeof(TranspositionTableCluster);
      std::memset(table_ + clear_index, 0, clear_size);
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  age_ = 0;
}

}  // namespace search