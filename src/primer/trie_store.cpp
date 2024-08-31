#include "primer/trie_store.h"
#include "common/exception.h"

namespace bustub {

template <class T>
auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<T>> {
  // Pseudo-code:
  // (1) Take the root lock, get the root, and release the root lock. Don't lookup the value in the
  //     trie while holding the root lock.
  // (2) Lookup the value in the trie.
  // (3) If the value is found, return a ValueGuard object that holds a reference to the value and the
  //     root. Otherwise, return std::nullopt.
  // throw NotImplementedException("TrieStore::Get is not implemented.");

  std::shared_ptr<const TrieNode> root_copy;
  auto trie = Trie();
  // 使用互斥锁来保护根节点的读取
  std::lock_guard<std::mutex> lock(root_lock_);
  // 获取Trie的根节点
  trie = root_;
  // 在Trie中查找键
  const T *value = trie.Get<T>(key);  // 假设TrieNode有Get方法
  // 根据查找结果返回
  if (value) {
    return ValueGuard<T>(trie, *value);
  }
  return std::nullopt;
}

template <class T>
void TrieStore::Put(std::string_view key, T value) {
  // You will need to ensure there is only one writer at a time. Think of how you can achieve this.
  // The logic should be somehow similar to `TrieStore::Get`.
  // throw NotImplementedException("TrieStore::Put is not implemented.");

  std::shared_ptr<const TrieNode> root_copy;
  // lock_guard 加锁，当离开局部作用域，析构函数自动完成解锁功能
  // 读写加锁
  std::lock_guard<std::mutex> lock(write_lock_);
  // 复制当前根节点，对其进行修改
  Trie new_trie = root_.Put<T>(key, std::move(value));
  std::lock_guard<std::mutex> root_lock(root_lock_);
  root_ = new_trie;
}

void TrieStore::Remove(std::string_view key) {
  // You will need to ensure there is only one writer at a time. Think of how you can achieve this.
  // The logic should be somehow similar to `TrieStore::Get`.
  // throw NotImplementedException("TrieStore::Remove is not implemented.");

  std::lock_guard<std::mutex> lock(write_lock_);
  // 执行删除操作并获取新的 Trie 实例
  Trie new_trie_version = root_.Remove(key);
  std::lock_guard<std::mutex> root_lock(root_lock_);
  // 更新 Trie 的根节点
  root_ = new_trie_version;
}

// Below are explicit instantiation of template functions.

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<uint32_t>>;
template void TrieStore::Put(std::string_view key, uint32_t value);

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<std::string>>;
template void TrieStore::Put(std::string_view key, std::string value);

// If your solution cannot compile for non-copy tests, you can remove the below lines to get partial score.

using Integer = std::unique_ptr<uint32_t>;

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<Integer>>;
template void TrieStore::Put(std::string_view key, Integer value);

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<MoveBlocked>>;
template void TrieStore::Put(std::string_view key, MoveBlocked value);

}  // namespace bustub
