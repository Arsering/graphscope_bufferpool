/** Copyright 2020 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <memory>
#include <mutex>
#include <string_view>
#include <type_traits>
#include <vector>

namespace gs {
template <typename INDEX_T>
class index_key_item {
 public:
  INDEX_T index;
  int64_t key;
};

namespace id_indexer_impl {

static constexpr int8_t min_lookups = 4;
static constexpr double max_load_factor = 0.5f;

inline int8_t log2(size_t value) {
  static constexpr int8_t table[64] = {
      63, 0,  58, 1,  59, 47, 53, 2,  60, 39, 48, 27, 54, 33, 42, 3,
      61, 51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4,
      62, 57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21,
      56, 45, 25, 31, 35, 16, 9,  12, 44, 24, 15, 8,  23, 7,  6,  5};
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  value |= value >> 32;
  return table[((value - (value >> 1)) * 0x07EDD5E59A4E28C2) >> 58];
}

template <typename T>
struct KeyBuffer {
  using type = std::vector<T>;

  template <typename IOADAPTOR_T>
  static void serialize(std::unique_ptr<IOADAPTOR_T>& writer,
                        const type& buffer) {
    size_t size = buffer.size();
    CHECK(writer->Write(&size, sizeof(size_t)));
    if (size > 0) {
      CHECK(writer->Write(buffer.data(), size * sizeof(T)));
    }
  }

  template <typename IOADAPTOR_T>
  static void deserialize(std::unique_ptr<IOADAPTOR_T>& reader, type& buffer) {
    size_t size;
    CHECK(reader->Read(&size, sizeof(size_t)));
    if (size > 0) {
      buffer.resize(size);
      CHECK(reader->Read(buffer.data(), size * sizeof(T)));
    }
  }
};

template <>
struct KeyBuffer<std::string> {
  using type = std::vector<std::string>;

  template <typename IOADAPTOR_T>
  static void serialize(std::unique_ptr<IOADAPTOR_T>& writer,
                        const type& buffer) {
    grape::InArchive arc;
    arc << buffer;
    CHECK(writer->WriteArchive(arc));
  }

  template <typename IOADAPTOR_T>
  static void deserialize(std::unique_ptr<IOADAPTOR_T>& reader, type& buffer) {
    grape::OutArchive arc;
    CHECK(reader->ReadArchive(arc));
    arc >> buffer;
  }
};

template <>
struct KeyBuffer<std::string_view> {
  using type = StringViewVector;

  template <typename IOADAPTOR_T>
  static void serialize(std::unique_ptr<IOADAPTOR_T>& writer,
                        const type& buffer) {
    size_t content_buffer_size = buffer.content_buffer().size();
    CHECK(writer->Write(&content_buffer_size, sizeof(size_t)));
    if (content_buffer_size > 0) {
      CHECK(writer->Write(buffer.content_buffer().data(),
                          content_buffer_size * sizeof(char)));
    }
    size_t offset_buffer_size = buffer.offset_buffer().size();
    CHECK(writer->Write(&offset_buffer_size, sizeof(size_t)));
    if (offset_buffer_size > 0) {
      CHECK(writer->Write(buffer.offset_buffer().data(),
                          offset_buffer_size * sizeof(size_t)));
    }
  }

  template <typename IOADAPTOR_T>
  static void deserialize(std::unique_ptr<IOADAPTOR_T>& reader, type& buffer) {
    size_t content_buffer_size;
    CHECK(reader->Read(&content_buffer_size, sizeof(size_t)));
    if (content_buffer_size > 0) {
      buffer.content_buffer().resize(content_buffer_size);
      CHECK(reader->Read(buffer.content_buffer().data(),
                         content_buffer_size * sizeof(char)));
    }
    size_t offset_buffer_size;
    CHECK(reader->Read(&offset_buffer_size, sizeof(size_t)));
    if (offset_buffer_size > 0) {
      buffer.offset_buffer().resize(offset_buffer_size);
      CHECK(reader->Read(buffer.offset_buffer().data(),
                         offset_buffer_size * sizeof(size_t)));
    }
  }
};

}  // namespace id_indexer_impl

template <typename T>
struct GHash {
  size_t operator()(const T& val) const { return std::hash<T>()(val); }
};

template <>
struct GHash<int64_t> {
  size_t operator()(const int64_t& val) const {
    uint64_t x = static_cast<uint64_t>(val);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
  }
};

template <typename KEY_T, typename INDEX_T>
class IdIndexer;

template <typename INDEX_T>
class LFIndexer;

template <class INDEX_T>
void build_lf_indexer(const IdIndexer<int64_t, INDEX_T>& input,
                      const std::string& filename, LFIndexer<INDEX_T>& output,
                      double rate = 0.8);

template <typename INDEX_T>
class LFIndexer {
 public:
  LFIndexer() : num_elements_(0), hasher_() {}
  LFIndexer(LFIndexer&& rhs)
      : keys_(std::move(rhs.keys_)),
        indices_(std::move(rhs.indices_)),
        num_elements_(rhs.num_elements_.load()),
        num_slots_minus_one_(rhs.num_slots_minus_one_),
        hasher_(rhs.hasher_) {
    hash_policy_.set_mod_function_by_index(
        rhs.hash_policy_.get_mod_function_index());
  }

  size_t size() const { return num_elements_.load(); }

  INDEX_T insert(int64_t oid) {
    INDEX_T ind = static_cast<INDEX_T>(num_elements_.fetch_add(1));
#if OV
    keys_.set(ind, oid);
    size_t index =
        hash_policy_.index_for_hash(hasher_(oid), num_slots_minus_one_);
    static constexpr INDEX_T sentinel = std::numeric_limits<INDEX_T>::max();

    while (true) {
      if (__sync_bool_compare_and_swap(&indices_.data()[index], sentinel,
                                       ind)) {
        break;
      }
      index = (index + 1) % num_slots_minus_one_;
    }
#else
    {
      auto item1 = keys_.get(ind);
      gbp::BufferBlock::UpdateContent<int64_t>(
          [&](int64_t& item) { item = oid; }, item1);
    }
    size_t index =
        hash_policy_.index_for_hash(hasher_(oid), num_slots_minus_one_);
    static constexpr INDEX_T sentinel = std::numeric_limits<INDEX_T>::max();

    int mark = 0;
    // TODO: 此处实现未被测试正确性
    uint32_t num_get =
        indices_.OBJ_NUM_PERPAGE - index % indices_.OBJ_NUM_PERPAGE;
    uint32_t start_index = index, end_index = index + num_get;
    auto items = indices_.get(index, num_get);
    while (true) {
      if (unlikely(index < start_index || index >= end_index)) {
        num_get = indices_.OBJ_NUM_PERPAGE - index % indices_.OBJ_NUM_PERPAGE;
        items = indices_.get(index, num_get);
        start_index = index, end_index = index + num_get;
      }

      gbp::BufferBlock::UpdateContent<index_key_item<INDEX_T>>(
          [&](index_key_item<INDEX_T>& item) {
            mark = __sync_bool_compare_and_swap(&(item.index), sentinel, ind);
            if (mark) {
              item.key = oid;
            }
          },
          items, index - start_index);
      if (mark) {
        break;
      }
      index = (index + 1) % num_slots_minus_one_;
    }
#endif
    return ind;
  }

  INDEX_T get_index(int64_t oid) const {
    size_t index =
        hash_policy_.index_for_hash(hasher_(oid), num_slots_minus_one_);
    static constexpr INDEX_T sentinel = std::numeric_limits<INDEX_T>::max();
#if OV
    while (true) {
      INDEX_T ind = indices_.get(index);
      if (ind == sentinel) {
        LOG(FATAL) << "cannot find " << oid << " in id_indexer";
      } else if (keys_.get(ind) == oid) {
        return ind;
      } else {
        index = (index + 1) % num_slots_minus_one_;
      }
    }
#else
    auto num_get = indices_.OBJ_NUM_PERPAGE - index % indices_.OBJ_NUM_PERPAGE;
    num_get = std::min(num_get, indices_.size() - index);
    uint32_t start_index = index, end_index = index + num_get;
    auto items = indices_.get(index, num_get);
    while (true) {
      if (unlikely(index < start_index || index >= end_index)) {
        num_get = indices_.OBJ_NUM_PERPAGE - index % indices_.OBJ_NUM_PERPAGE;
        num_get = std::min(num_get, indices_.size() - index);
        items = indices_.get(index, num_get);
        start_index = index, end_index = index + num_get;
      }

      auto& ind = gbp::BufferBlock::Ref<index_key_item<INDEX_T>>(
          items, index - start_index);
      if (ind.index == sentinel) {
        LOG(FATAL) << "cannot find " << oid << " in id_indexer";
      } else {
        // auto item = keys_.get(ind.index);
        // assert(gbp::BufferBlock::Ref<int64_t>(item) == ind.key);
        if (ind.key == oid) {
          return ind.index;
        }
      }
      index = (index + 1) % num_slots_minus_one_;
    }
#endif
  }

#if OV
  bool get_index(int64_t oid, INDEX_T& ret) const {
    size_t index =
        hash_policy_.index_for_hash(hasher_(oid), num_slots_minus_one_);

    static constexpr INDEX_T sentinel = std::numeric_limits<INDEX_T>::max();
    while (true) {
      INDEX_T ind = indices_.get(index);

      if (ind == sentinel) {
        return false;
      } else if (keys_.get(ind) == oid) {
        ret = ind;
        return true;
      } else {
        index = (index + 1) % num_slots_minus_one_;
      }
    }

    return false;
  }
  int64_t get_key(const INDEX_T& index) const { return keys_.get(index); }
#else
  bool get_index(int64_t oid, INDEX_T& ret) const {
    size_t index =
        hash_policy_.index_for_hash(hasher_(oid), num_slots_minus_one_);
    static constexpr INDEX_T sentinel = std::numeric_limits<INDEX_T>::max();

    uint32_t num_get =
        indices_.OBJ_NUM_PERPAGE - index % indices_.OBJ_NUM_PERPAGE;
    num_get =
        num_get > indices_.size() - index ? indices_.size() - index : num_get;
    uint32_t start_index = index, end_index = index + num_get;
    auto items = indices_.get(index, num_get);
    while (true) {
      if (unlikely(index < start_index || index >= end_index)) {
        num_get = indices_.OBJ_NUM_PERPAGE - index % indices_.OBJ_NUM_PERPAGE;
        num_get = num_get > indices_.size() - index ? indices_.size() - index
                                                    : num_get;
        items = indices_.get(index, num_get);
        start_index = index, end_index = index + num_get;
      }

      auto& ind = gbp::BufferBlock::Ref<index_key_item<INDEX_T>>(
          items, index - start_index);
      if (ind.index == sentinel) {
        return false;
      } else {
        // auto item = keys_.get(ind.index);
        // assert(gbp::BufferBlock::Ref<int64_t>(item) == ind.key);
        if (ind.key == oid) {
          ret = ind.index;
          return true;
        }
      }
      index = (index + 1) % num_slots_minus_one_;
    }

    return false;
  }

  int64_t get_key(const INDEX_T& index) const {
    auto item = keys_.get(index);
    return gbp::BufferBlock::Ref<int64_t>(item);
  }
#endif

  void open(const std::string& name, const std::string& snapshot_dir,
            const std::string& work_dir) {
    keys_.open(snapshot_dir + "/" + name + ".keys", true);
    keys_.touch(work_dir + "/" + name + ".keys");
    indices_.open(snapshot_dir + "/" + name + ".indices", true);
    indices_.touch(work_dir + "/" + name + ".indices");

    indices_size_ = indices_.size();
    for (size_t k = keys_.size() - 1; k >= 0; --k) {
#if OV
      if (keys_.get(k) != std::numeric_limits<int64_t>::max()) {
        num_elements_.store(k + 1);
        break;
      }
#else
      auto item = keys_.get(k);
      if (gbp::BufferBlock::Ref<int64_t>(item) !=
          std::numeric_limits<int64_t>::max()) {
        num_elements_.store(k + 1);
        break;
      }
#endif
    }

    load_meta(snapshot_dir + "/" + name + ".meta");
  }

  size_t get_size_in_byte() {
    return keys_.get_size_in_byte() + indices_.get_size_in_byte();
  }

  void dump(const std::string& name, const std::string& snapshot_dir) {
    keys_.dump(snapshot_dir + "/" + name + ".keys");
    indices_.dump(snapshot_dir + "/" + name + ".indices");
    dump_meta(snapshot_dir + "/" + name + ".meta");
  }

  void dump_meta(const std::string& filename) const {
    grape::InArchive arc;
    arc << num_slots_minus_one_ << hash_policy_.get_mod_function_index();
    FILE* fout = fopen(filename.c_str(), "wb");
    fwrite(arc.GetBuffer(), sizeof(char), arc.GetSize(), fout);
    fflush(fout);
    fclose(fout);
  }

  void load_meta(const std::string& filename) {
    grape::OutArchive arc;
    FILE* fin = fopen(filename.c_str(), "r");

    size_t meta_file_size = std::filesystem::file_size(filename);

    std::vector<char> buf(meta_file_size);
    CHECK_EQ(fread(buf.data(), sizeof(char), meta_file_size, fin),
             meta_file_size);
    arc.SetSlice(buf.data(), meta_file_size);
    size_t mod_function_index;
    arc >> num_slots_minus_one_ >> mod_function_index;
    hash_policy_.set_mod_function_by_index(mod_function_index);
  }

  // get keys
  const mmap_array<int64_t>& get_keys() const { return keys_; }

 private:
  mmap_array<int64_t> keys_;
#if OV
  mmap_array<INDEX_T>
      indices_;  // size() == indices_size_ == num_slots_minus_one_
                 // +log(num_slots_minus_one_)
#else
  mmap_array<index_key_item<INDEX_T>>
      indices_;  // size() == indices_size_ == num_slots_minus_one_
                 // +log(num_slots_minus_one_)
#endif
  std::atomic<size_t> num_elements_;
  size_t num_slots_minus_one_;
  size_t indices_size_;
  ska::ska::prime_number_hash_policy hash_policy_;
  GHash<int64_t> hasher_;

  template <typename _INDEX_T>
  friend void build_lf_indexer(const IdIndexer<int64_t, _INDEX_T>& input,
                               const std::string& filename,
                               LFIndexer<_INDEX_T>& output, double rate);
};

template <typename KEY_T, typename INDEX_T>
class IdIndexer {
 public:
  using key_buffer_t = typename id_indexer_impl::KeyBuffer<KEY_T>::type;
  using ind_buffer_t = std::vector<INDEX_T>;
  using dist_buffer_t = std::vector<int8_t>;

  IdIndexer() : hasher_() { reset_to_empty_state(); }
  ~IdIndexer() {}

  size_t entry_num() const { return distances_.size(); }

  bool add(const KEY_T& oid, INDEX_T& lid) {
    size_t index =
        hash_policy_.index_for_hash(hasher_(oid), num_slots_minus_one_);

    int8_t distance_from_desired = 0;
    for (; distances_[index] >= distance_from_desired;
         ++index, ++distance_from_desired) {
      INDEX_T cur_lid = indices_[index];
      if (keys_[cur_lid] == oid) {
        lid = cur_lid;
        return false;
      }
    }

    lid = static_cast<INDEX_T>(keys_.size());
    keys_.push_back(oid);
#if OV
    assert(keys_.size() == num_elements_ + 1);
    emplace_new_value(distance_from_desired, index, lid);
    assert(keys_.size() == num_elements_);
#else

#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_ + 1);
#endif
    emplace_new_value(distance_from_desired, index, lid);
#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_);
#endif

#endif
    return true;
  }

  bool add(KEY_T&& oid, INDEX_T& lid) {
    size_t index =
        hash_policy_.index_for_hash(hasher_(oid), num_slots_minus_one_);

    int8_t distance_from_desired = 0;
    for (; distances_[index] >= distance_from_desired;
         ++index, ++distance_from_desired) {
      INDEX_T cur_lid = indices_[index];
      if (keys_[cur_lid] == oid) {
        lid = cur_lid;
        return false;
      }
    }

    lid = static_cast<INDEX_T>(keys_.size());
    keys_.push_back(std::move(oid));
#if OV
    assert(keys_.size() == num_elements_ + 1);
    emplace_new_value(distance_from_desired, index, lid);
    assert(keys_.size() == num_elements_);
#else
#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_ + 1);
#endif
    emplace_new_value(distance_from_desired, index, lid);
#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_);
#endif
#endif
    return true;
  }

  bool _add(const KEY_T& oid, size_t hash_value, INDEX_T& lid) {
    size_t index =
        hash_policy_.index_for_hash(hash_value, num_slots_minus_one_);

    int8_t distance_from_desired = 0;
    for (; distances_[index] >= distance_from_desired;
         ++index, ++distance_from_desired) {
      INDEX_T cur_lid = indices_[index];
      if (keys_[cur_lid] == oid) {
        lid = cur_lid;
        return false;
      }
    }

    lid = static_cast<INDEX_T>(keys_.size());
    keys_.push_back(oid);
#if OV
    assert(keys_.size() == num_elements_ + 1);
    emplace_new_value(distance_from_desired, index, lid);
    assert(keys_.size() == num_elements_);
#else

#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_ + 1);
#endif
    emplace_new_value(distance_from_desired, index, lid);
#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_);
#endif

#endif
    return true;
  }

  bool _add(KEY_T&& oid, size_t hash_value, INDEX_T& lid) {
    size_t index =
        hash_policy_.index_for_hash(hash_value, num_slots_minus_one_);

    int8_t distance_from_desired = 0;
    for (; distances_[index] >= distance_from_desired;
         ++index, ++distance_from_desired) {
      INDEX_T cur_lid = indices_[index];
      if (keys_[cur_lid] == oid) {
        lid = cur_lid;
        return false;
      }
    }

    lid = static_cast<INDEX_T>(keys_.size());
    keys_.push_back(std::move(oid));
#if OV
    assert(keys_.size() == num_elements_ + 1);
    emplace_new_value(distance_from_desired, index, lid);
    assert(keys_.size() == num_elements_);
#else
#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_ + 1);
#endif
    emplace_new_value(distance_from_desired, index, lid);
#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_);
#endif
#endif
    return true;
  }

  void _add(const KEY_T& oid) {
    size_t index =
        hash_policy_.index_for_hash(hasher_(oid), num_slots_minus_one_);

    int8_t distance_from_desired = 0;
    for (; distances_[index] >= distance_from_desired;
         ++index, ++distance_from_desired) {
      if (keys_[indices_[index]] == oid) {
        return;
      }
    }

    INDEX_T lid = static_cast<INDEX_T>(keys_.size());
    keys_.push_back(oid);
#if OV
    assert(keys_.size() == num_elements_ + 1);
    emplace_new_value(distance_from_desired, index, lid);
    assert(keys_.size() == num_elements_);
#else
#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_ + 1);
#endif
    emplace_new_value(distance_from_desired, index, lid);
#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_);
#endif
#endif
  }

  void _add(KEY_T&& oid) {
    size_t index =
        hash_policy_.index_for_hash(hasher_(oid), num_slots_minus_one_);

    int8_t distance_from_desired = 0;
    for (; distances_[index] >= distance_from_desired;
         ++index, ++distance_from_desired) {
      if (keys_[indices_[index]] == oid) {
        return;
      }
    }

    INDEX_T lid = static_cast<INDEX_T>(keys_.size());
    keys_.push_back(std::move(oid));
#if OV
    assert(keys_.size() == num_elements_ + 1);
    emplace_new_value(distance_from_desired, index, lid);
    assert(keys_.size() == num_elements_);
#else
#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_ + 1);
#endif
    emplace_new_value(distance_from_desired, index, lid);
#if ASSERT_ENABLE
    assert(keys_.size() == num_elements_);
#endif
#endif
  }

  size_t bucket_count() const {
    return num_slots_minus_one_ ? num_slots_minus_one_ + 1 : 0;
  }

  bool empty() const { return (num_elements_ == 0); }

  size_t size() const { return num_elements_; }

  bool get_key(INDEX_T lid, KEY_T& oid) const {
    if (static_cast<size_t>(lid) >= num_elements_) {
      return false;
    }
    oid = keys_[lid];
    return true;
  }

  bool get_index(const KEY_T& oid, INDEX_T& lid) const {
    size_t index =
        hash_policy_.index_for_hash(hasher_(oid), num_slots_minus_one_);
    for (int8_t distance = 0; distances_[index] >= distance;
         ++distance, ++index) {
      INDEX_T ret = indices_[index];
      if (keys_[ret] == oid) {
        lid = ret;
        return true;
      }
    }
    return false;
  }

  bool _get_index(const KEY_T& oid, size_t hash, INDEX_T& lid) const {
    size_t index = hash_policy_.index_for_hash(hash, num_slots_minus_one_);
    for (int8_t distance = 0; distances_[index] >= distance;
         ++distance, ++index) {
      INDEX_T ret = indices_[index];
      if (keys_[ret] == oid) {
        lid = ret;
        return true;
      }
    }
    return false;
  }

  void swap(IdIndexer<KEY_T, INDEX_T>& rhs) {
    keys_.swap(rhs.keys_);
    indices_.swap(rhs.indices_);
    distances_.swap(rhs.distances_);

    hash_policy_.swap(rhs.hash_policy_);
    std::swap(max_lookups_, rhs.max_lookups_);
    std::swap(num_elements_, rhs.num_elements_);
    std::swap(num_slots_minus_one_, rhs.num_slots_minus_one_);

    std::swap(hasher_, rhs.hasher_);
  }

  const key_buffer_t& keys() const { return keys_; }

  key_buffer_t& keys() { return keys_; }

  void Serialize(std::unique_ptr<grape::LocalIOAdaptor>& writer) const {
    id_indexer_impl::KeyBuffer<KEY_T>::serialize(writer, keys_);
    grape::InArchive arc;
    arc << hash_policy_.get_mod_function_index() << max_lookups_
        << num_elements_ << num_slots_minus_one_ << indices_.size()
        << distances_.size();
    CHECK(writer->WriteArchive(arc));
    arc.Clear();

    if (indices_.size() > 0) {
      CHECK(writer->Write(const_cast<uint8_t*>(indices_.data()),
                          indices_.size() * sizeof(INDEX_T)));
    }
    if (distances_.size() > 0) {
      CHECK(writer->Write(const_cast<int8_t*>(distances_.data()),
                          distances_.size() * sizeof(int8_t)));
    }
  }

  void Deserialize(std::unique_ptr<grape::LocalIOAdaptor>& reader) {
    id_indexer_impl::KeyBuffer<KEY_T>::deserialize(reader, keys_);
    grape::OutArchive arc;
    CHECK(reader->ReadArchive(arc));
    size_t mod_function_index;
    size_t indices_size, distances_size;
    arc >> mod_function_index >> max_lookups_ >> num_elements_ >>
        num_slots_minus_one_ >> indices_size >> distances_size;
    arc.Clear();

    hash_policy_.set_mod_function_by_index(mod_function_index);
    indices_.resize(indices_size);
    distances_.resize(distances_size);
    if (indices_size > 0) {
      CHECK(reader->Read(indices_.data(), indices_.size() * sizeof(INDEX_T)));
    }
    if (distances_size > 0) {
      CHECK(
          reader->Read(distances_.data(), distances_.size() * sizeof(int8_t)));
    }
  }

  void _rehash(int num) { rehash(num); }

 private:
  void emplace(INDEX_T lid) {
    KEY_T key = keys_[lid];
    size_t index =
        hash_policy_.index_for_hash(hasher_(key), num_slots_minus_one_);
    int8_t distance_from_desired = 0;
    for (; distances_[index] >= distance_from_desired;
         ++index, ++distance_from_desired) {
      if (indices_[index] == lid) {
        return;
      }
    }

    emplace_new_value(distance_from_desired, index, lid);
  }

  void emplace_new_value(int8_t distance_from_desired, size_t index,
                         INDEX_T lid) {
    if (num_slots_minus_one_ == 0 || distance_from_desired == max_lookups_ ||
        num_elements_ + 1 >
            (num_slots_minus_one_ + 1) * id_indexer_impl::max_load_factor) {
      grow();
      return;
    } else if (distances_[index] < 0) {
      indices_[index] = lid;
      distances_[index] = distance_from_desired;
      ++num_elements_;
      return;
    }
    INDEX_T to_insert = lid;
    std::swap(distance_from_desired, distances_[index]);
    std::swap(to_insert, indices_[index]);
    for (++distance_from_desired, ++index;; ++index) {
      if (distances_[index] < 0) {
        indices_[index] = to_insert;
        distances_[index] = distance_from_desired;
        ++num_elements_;
        return;
      } else if (distances_[index] < distance_from_desired) {
        std::swap(distance_from_desired, distances_[index]);
        std::swap(to_insert, indices_[index]);
        ++distance_from_desired;
      } else {
        ++distance_from_desired;
        if (distance_from_desired == max_lookups_) {
          grow();
          return;
        }
      }
    }
  }

  void grow() { rehash(std::max(size_t(4), 2 * bucket_count())); }

  void rehash(size_t num_buckets) {
    num_buckets = std::max(
        num_buckets, static_cast<size_t>(std::ceil(
                         num_elements_ / id_indexer_impl::max_load_factor)));

    if (num_buckets == 0) {
      reset_to_empty_state();
      return;
    }

    auto new_prime_index = hash_policy_.next_size_over(num_buckets);
    if (num_buckets == bucket_count()) {
      return;
    }

    int8_t new_max_lookups = compute_max_lookups(num_buckets);

    dist_buffer_t new_distances(num_buckets + new_max_lookups);
    ind_buffer_t new_indices(num_buckets + new_max_lookups);

    size_t special_end_index = num_buckets + new_max_lookups - 1;
    for (size_t i = 0; i != special_end_index; ++i) {
      new_distances[i] = -1;
    }
    new_distances[special_end_index] = 0;

    new_indices.swap(indices_);
    new_distances.swap(distances_);

    std::swap(num_slots_minus_one_, num_buckets);
    --num_slots_minus_one_;
    hash_policy_.commit(new_prime_index);

    max_lookups_ = new_max_lookups;

    num_elements_ = 0;
    INDEX_T elem_num = static_cast<INDEX_T>(keys_.size());
    for (INDEX_T lid = 0; lid < elem_num; ++lid) {
      emplace(lid);
    }
  }

  void reset_to_empty_state() {
    keys_.clear();

    indices_.clear();
    distances_.clear();
    indices_.resize(id_indexer_impl::min_lookups);
    distances_.resize(id_indexer_impl::min_lookups, -1);
    distances_[id_indexer_impl::min_lookups - 1] = 0;

    num_slots_minus_one_ = 0;
    hash_policy_.reset();
    max_lookups_ = id_indexer_impl::min_lookups - 1;
    num_elements_ = 0;
  }

  static int8_t compute_max_lookups(size_t num_buckets) {
    int8_t desired = id_indexer_impl::log2(num_buckets);
    return std::max(id_indexer_impl::min_lookups, desired);
  }

  key_buffer_t keys_;
  ind_buffer_t indices_;
  dist_buffer_t distances_;

  ska::ska::prime_number_hash_policy hash_policy_;
  int8_t max_lookups_ = id_indexer_impl::min_lookups - 1;
  size_t num_elements_ = 0;
  size_t num_slots_minus_one_ = 0;

  // std::hash<KEY_T> hasher_;
  GHash<KEY_T> hasher_;

  template <typename _INDEX_T>
  friend void build_lf_indexer(const IdIndexer<int64_t, _INDEX_T>& input,
                               const std::string& filename,
                               LFIndexer<_INDEX_T>& output, double rate);
};

#if OV
template <class INDEX_T>
void build_lf_indexer(const IdIndexer<int64_t, INDEX_T>& input,
                      const std::string& filename, LFIndexer<INDEX_T>& lf,
                      double rate) {
  double indices_rate = static_cast<double>(input.keys_.size()) /
                        static_cast<double>(input.indices_.size());
  CHECK_LT(indices_rate, rate);

  size_t size = input.keys_.size();
  size_t lf_size = static_cast<double>(size) / rate + 1;
  lf_size = std::max(lf_size, static_cast<size_t>(1024));

  lf.keys_.open(filename + ".keys", false);
  lf.keys_.resize(lf_size);
  memcpy(lf.keys_.data(), input.keys_.data(), sizeof(int64_t) * size);
  for (size_t k = size; k != lf_size; ++k) {
    lf.keys_.set(k, std::numeric_limits<int64_t>::max());
  }

  lf.num_elements_.store(size);

  lf.indices_.open(filename + ".indices", false);
  lf.indices_.resize(input.indices_.size());
  for (size_t k = 0; k != input.indices_.size(); ++k) {
    lf.indices_[k] = std::numeric_limits<INDEX_T>::max();
  }
  lf.indices_size_ = input.indices_.size();

  lf.hash_policy_.set_mod_function_by_index(
      input.hash_policy_.get_mod_function_index());
  lf.num_slots_minus_one_ = input.num_slots_minus_one_;
  std::vector<std::pair<int64_t, INDEX_T>> extra;

  for (auto oid : input.keys_) {
    size_t index = input.hash_policy_.index_for_hash(
        input.hasher_(oid), input.num_slots_minus_one_);
    for (int8_t distance = 0; input.distances_[index] >= distance;
         ++distance, ++index) {
      INDEX_T ret = input.indices_[index];
      if (input.keys_[ret] == oid) {
        if (index >= input.num_slots_minus_one_) {
          extra.emplace_back(oid, ret);
        } else {
          lf.indices_[index] = ret;
        }
        break;
      }
    }
  }

  static constexpr INDEX_T sentinel = std::numeric_limits<INDEX_T>::max();
  for (auto& pair : extra) {
    size_t index = input.hash_policy_.index_for_hash(
        input.hasher_(pair.first), input.num_slots_minus_one_);
    while (true) {
      if (lf.indices_[index] == sentinel) {
        lf.indices_[index] = pair.second;
        break;
      }
      index = (index + 1) % input.num_slots_minus_one_;
    }
  }

  lf.dump_meta(filename + ".meta");
}
#else
template <class INDEX_T>
void build_lf_indexer(const IdIndexer<int64_t, INDEX_T>& input,
                      const std::string& filename, LFIndexer<INDEX_T>& lf,
                      double rate) {
  double indices_rate = static_cast<double>(input.keys_.size()) /
                        static_cast<double>(input.indices_.size());
  CHECK_LT(indices_rate, rate);

  size_t size = input.keys_.size();
  size_t lf_size = static_cast<double>(size) / rate + 1;
  lf_size = std::max(lf_size, static_cast<size_t>(1024));

  lf.keys_.open(filename + ".keys", false);
  lf.keys_.resize(lf_size);

  // FIXME: input.keys_是mmap分配的还是malloc分配的？？？
  auto keys_item = lf.keys_.get(0, lf_size);
  for (size_t t = 0; t < size; t++) {
    gbp::BufferBlock::UpdateContent<int64_t>(
        [&](int64_t& item) { item = input.keys_[t]; }, keys_item, t);
  }

  for (size_t k = size; k != lf_size; ++k) {
    gbp::BufferBlock::UpdateContent<int64_t>(
        [&](int64_t& item) { item = std::numeric_limits<int64_t>::max(); },
        keys_item, k);
  }

  lf.num_elements_.store(size);

  lf.indices_.open(filename + ".indices", false);
  lf.indices_.resize(input.indices_.size());

  index_key_item<INDEX_T> empty_value_1{std::numeric_limits<INDEX_T>::max(), 0};
  for (size_t k = 0; k != input.indices_.size(); ++k) {
    lf.indices_.set(k, &empty_value_1);
  }
  lf.indices_size_ = input.indices_.size();

  lf.hash_policy_.set_mod_function_by_index(
      input.hash_policy_.get_mod_function_index());
  lf.num_slots_minus_one_ = input.num_slots_minus_one_;
  std::vector<std::pair<int64_t, INDEX_T>> extra;

  for (auto oid : input.keys_) {
    size_t index = input.hash_policy_.index_for_hash(
        input.hasher_(oid), input.num_slots_minus_one_);
    for (int8_t distance = 0; input.distances_[index] >= distance;
         ++distance, ++index) {
      INDEX_T ret = input.indices_[index];
      if (input.keys_[ret] == oid) {
        if (index >= input.num_slots_minus_one_) {
          extra.emplace_back(oid, ret);
        } else {
          empty_value_1.index = ret;
          empty_value_1.key = oid;
          lf.indices_.set(index, &empty_value_1);
        }
        break;
      }
    }
  }

  static constexpr INDEX_T sentinel = std::numeric_limits<INDEX_T>::max();
  for (auto& pair : extra) {
    size_t index = input.hash_policy_.index_for_hash(
        input.hasher_(pair.first), input.num_slots_minus_one_);
    while (true) {
      auto item = lf.indices_.get(index);
      if (gbp::BufferBlock::Ref<index_key_item<INDEX_T>>(item).index ==
          sentinel) {
        empty_value_1.index = pair.second;
        empty_value_1.key = pair.first;
        lf.indices_.set(index, &(empty_value_1));
        break;
      }
      index = (index + 1) % input.num_slots_minus_one_;
    }
  }

  lf.dump_meta(filename + ".meta");
}
#endif
}  // namespace gs
