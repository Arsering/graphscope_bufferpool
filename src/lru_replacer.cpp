/**
 * LRU implementation
 */
#include "../include/lru_replacer.h"
#include "../include/page.h"

namespace gbp {

template <typename T>
LRUReplacer<T>::LRUReplacer() {
  head_ = std::make_shared<Node>();
  tail_ = std::make_shared<Node>();
  head_->next = tail_;
  tail_->prev = head_;
}

template <typename T>
LRUReplacer<T>::~LRUReplacer() {}

/*
 * Insert value into LRU
 */
template <typename T>
void LRUReplacer<T>::Insert(const T& value) {
  std::lock_guard<std::mutex> lck(latch);
  std::shared_ptr<Node> cur;
  if (map.find(value) != map.end()) {
    cur = map[value];
    std::shared_ptr<Node> prev = cur->prev;
    std::shared_ptr<Node> succ = cur->next;
    prev->next = succ;
    succ->prev = prev;
  } else {
    cur = std::make_shared<Node>(value);
  }
  std::shared_ptr<Node> fir = head_->next;
  cur->next = fir;
  fir->prev = cur;
  cur->prev = head_;
  head_->next = cur;
  map[value] = cur;
  return;
}

/* If LRU is non-empty, pop the head member from LRU to argument "value", and
 * return true. If LRU is empty, return false
 */
template <typename T>
bool LRUReplacer<T>::Victim(T& value) {
  std::lock_guard<std::mutex> lck(latch);
  if (map.empty()) {
    return false;
  }
  std::shared_ptr<Node> last = tail_->prev;
  tail_->prev = last->prev;
  last->prev->next = tail_;
  value = last->val;
  map.erase(last->val);
  return true;
}

/*
 * Remove value from LRU. If removal is successful, return true, otherwise
 * return false
 */
template <typename T>
bool LRUReplacer<T>::Erase(const T& value) {
  std::lock_guard<std::mutex> lck(latch);
  if (map.find(value) != map.end()) {
    std::shared_ptr<Node> cur = map[value];
    cur->prev->next = cur->next;
    cur->next->prev = cur->prev;
  }
  return map.erase(value);
}

template <typename T>
size_t LRUReplacer<T>::Size() {
  std::lock_guard<std::mutex> lck(latch);
  return map.size();
}

template class LRUReplacer<Page*>;
// test only
template class LRUReplacer<int>;

}  // namespace gbp
