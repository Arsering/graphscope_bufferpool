#include "../include/debug.h"
#include <bitset>
#include <iostream>
#include <memory>
#include <vector>

namespace gbp {
namespace debug {
std::vector<std::shared_ptr<std::atomic<size_t>>> counters_g;

static thread_local std::vector<debug::BitMap> bit_maps_g;

BitMap& get_bitset(uint32_t file_id) {
  assert(bit_maps_g.size() > file_id);
  return bit_maps_g[file_id];
}
std::vector<debug::BitMap>& get_bitmaps() { return bit_maps_g; }

std::atomic<size_t>& get_counter_read() {
  static std::atomic<size_t> counter(0);
  return counter;
}
std::atomic<size_t>& get_counter_fetch() {
  static std::atomic<size_t> counter(0);
  return counter;
}
std::atomic<size_t>& get_counter_fetch_unique() {
  static std::atomic<size_t> counter(0);
  return counter;
}
}  // namespace debug
}  // namespace gbp