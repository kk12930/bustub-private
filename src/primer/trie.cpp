#include "primer/trie.h"
#include <string_view>
#include "common/exception.h"

namespace bustub {

template <class T>
auto Trie::Get(std::string_view key) const -> const T * {
  auto node = root_;
  for (auto c : key) {
    if (node == nullptr || node->children_.find(c) == node->children_.end()) {
      return nullptr;
    }
    node = node->children_.at(c);
  }
  if (!node || !node->is_value_node_) {
    return nullptr;
  }
  auto node_value = dynamic_cast<const TrieNodeWithValue<T> *>(node.get());
  if (node_value == nullptr) {
    return nullptr;
  }
  return node_value->value_.get();
}
/*
  put 思路：
  假如我要put的是root 的话：（给root赋值）
  写时赋值的话 要新创建一个root 然后赋值
  假如我要给非root 赋值的话：
  递归处理每个符合条件的node
    ：
    向下查找：
      ：找到的话递归处理下一个节点
      ：没找到的话创建一个新的节点 然后递归 在插入节点 //递归像栈
*/
template <class T>
void PutCycle(const std::shared_ptr<TrieNode> &new_root, std::string_view key, T value) {
  bool flag = false;
  for (auto &pair : new_root->children_) {
    if (key.at(0) == pair.first) {
      flag = true;
      if (key.size() > 1) {
        std::shared_ptr<TrieNode> ptr = pair.second->Clone();
        PutCycle<T>(ptr, key.substr(1), std::move(value));
        pair.second = std::shared_ptr<const TrieNode>(ptr);
      } else {
        auto val_p = std::make_shared<T>(std::move(value));
        TrieNodeWithValue node_with_val(pair.second->children_, val_p);
        pair.second = std::make_shared<const TrieNodeWithValue<T>>(node_with_val);
      }
      return;
    }
  }
  if (!flag) {
    char c = key.at(0);
    if (key.size() == 1) {
      std::shared_ptr<T> val_p = std::make_shared<T>(std::move(value));
      new_root->children_.insert({c, std::make_shared<const TrieNodeWithValue<T>>(val_p)});
    } else {
      auto ptr = std::make_shared<TrieNode>();
      PutCycle<T>(ptr, key.substr(1), std::move(value));
      new_root->children_.insert({c, std::move(ptr)});
    }
  }
}

template <class T>
auto Trie::Put(std::string_view key, T value) const -> Trie {
  if (key.empty()) {
    auto val_p = std::make_shared<T>(std::move(value));
    std::unique_ptr<TrieNodeWithValue<T>> new_root = nullptr;
    if (root_->children_.empty()) {
      new_root = std::make_unique<TrieNodeWithValue<T>>(std::move(val_p));
    } else {
      new_root = std::make_unique<TrieNodeWithValue<T>>(root_->children_, std::move(val_p));
    }
    return Trie(std::move(new_root));
  }
  std::shared_ptr<TrieNode> new_root = nullptr;
  if (root_ == nullptr) {
    new_root = std::make_unique<TrieNode>();
  } else {
    new_root = root_->Clone();
  }
  PutCycle<T>(new_root, key, std::move(value));
  return Trie(std::move(new_root));
}
// You should walk through the trie and remove nodes if necessary. If the node doesn't contain a value any more,
// you should convert it to `TrieNode`. If a node doesn't have children any more, you should remove it.
// 您应该遍历 trie 并在必要时删除节点。如果节点不再包含值，
// 你应该将其转换为`TrieNode`。如果一个节点不再有子节点，则应该将其删除。
/*
  RemoveCycle:
    找有无当前key的字符 在字树是否存在
      当有的时候
        查看是不是最后一个字
          是的话查看有无值
            无值删除失败
            有值查看是否有子树
              有子树的话转为无值
              无子树的话删除
          不是递归处理
*/
auto RemoveCycle(const std::shared_ptr<TrieNode> &new_roottry, std::string_view key) -> bool {
  for (auto &pair : new_roottry->children_) {
    if (key.at(0) != pair.first) {
      continue;
    }
    if (key.size() == 1) {
      if (!pair.second->is_value_node_) {
        return false;
      }
      if (pair.second->children_.empty()) {
        new_roottry->children_.erase(pair.first);
      } else {
        pair.second = std::make_shared<const TrieNode>(pair.second->children_);
      }
      return true;
    }
    std::shared_ptr<TrieNode> ptr = pair.second->Clone();
    bool flag = RemoveCycle(ptr, key.substr(1, key.size() - 1));
    if (!flag) {
      return false;
    }
    if (ptr->children_.empty() && !ptr->is_value_node_) {
      new_roottry->children_.erase(pair.first);
    } else {
      pair.second = std::shared_ptr<const TrieNode>(ptr);
    }
    return true;
  }
  return false;
}
/*
  Remove
    当root为空的时候 返回当前树
    假如key为空的时候
      root有值的时候
        子树为空的话 返回一个新树
        子树不为空的话 返回一个带有子节点的新树
      root无值的时候
        直接返回
    当key不为空的时候
      递归删除
    当删除失败的时候 返回当前的树
    当删除成功的时候 返回新的树
*/

auto Trie::Remove(std::string_view key) const -> Trie {
  if (this->root_ == nullptr) {
    return *this;
  }
  if (key.empty()) {
    if (root_->is_value_node_) {
      if (root_->children_.empty()) {
        return Trie(nullptr);
      }
      std::shared_ptr<TrieNode> new_root = std::make_shared<TrieNode>(root_->children_);
      return Trie(new_root);
    }
    return *this;
  }
  std::shared_ptr<TrieNode> newroot = root_->Clone();
  bool flag = RemoveCycle(newroot, key);
  if (!flag) {
    return *this;
  }
  if (newroot->children_.empty() && !newroot->is_value_node_) {
    newroot = nullptr;
  }
  return Trie(std::move(newroot));
}
}  // namespace bustub
