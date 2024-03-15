// Copyright 2022 Guanyu Feng, Tsinghua University
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include <sys/mman.h>
#include <cassert>

#include "config.h"
#include "page_table.h"

namespace gbp {
class MemoryPool {
 public:
  MemoryPool(mpage_id_type num_pages) : num_pages_(num_pages) {
    pool_ =
        (char*) ::aligned_alloc(PAGE_SIZE_MEMORY, PAGE_SIZE_FILE * num_pages_);
    madvise(pool_, num_pages_ * PAGE_SIZE_FILE, MADV_RANDOM);
  }

  MemoryPool(const MemoryPool&) = delete;
  MemoryPool(MemoryPool&&) = delete;

  ~MemoryPool() { ::free(pool_); }

  FORCE_INLINE void* FromPageId(const mpage_id_type& mpage_id) const {
    assert(mpage_id < num_pages_);
    return pool_ + mpage_id * PAGE_SIZE_MEMORY;
  }

  FORCE_INLINE mpage_id_type ToPageId(void* ptr) const {
    assert((uintptr_t) ptr % num_pages_ == 0 && ptr > pool_ &&
           ptr < pool_ + num_pages_ * PAGE_SIZE_MEMORY);
    return ((char*) ptr - pool_) / PAGE_SIZE_MEMORY;
  }
  mpage_id_type GetSize() const { return num_pages_; }

 private:
  mpage_id_type num_pages_;
  char* pool_ = nullptr;
};
}  // namespace gbp