//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// update_executor.cpp
//
// Identification: src/execution/update_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>

#include "execution/executors/update_executor.h"

namespace bustub {

UpdateExecutor::UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void UpdateExecutor::Init() {
  this->child_executor_->Init();
  this->has_inserted_ = false;
}

auto UpdateExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  // 只要执行一次next函數
  if (has_inserted_) {
    return false;
  }
  this->has_inserted_ = true;
  int count = 0;
  // 获取table_info indexes相关基本信息，后面需要使用
  auto table_info = this->exec_ctx_->GetCatalog()->GetTable(this->plan_->GetTableOid());
  auto indexes = this->exec_ctx_->GetCatalog()->GetTableIndexes(table_info->name_);
  Tuple child_tuple{};
  RID child_rid{};
  while (this->child_executor_->Next(&child_tuple, &child_rid)) {
    count++;
    // 更新逻辑不是直接更新相应的值，而是把原来的元组设置为已删除，然后插入新的元组
    // 将每个元组都标记为已删除
    table_info->table_->UpdateTupleMeta(TupleMeta{0, true}, child_rid);
    std::vector<Value> new_values{};
    // 预分配足够的内存空间，以便 new_values 能够在不重新分配内存的情况下存储至少.size() 个 Value 类型的元素
    new_values.reserve(plan_->target_expressions_.size());
    // 获取要插入的新元组
    for (const auto &expr : plan_->target_expressions_) {
      new_values.push_back(expr->Evaluate(&child_tuple, child_executor_->GetOutputSchema()));
    }
    auto update_tuple = Tuple{new_values, &table_info->schema_};
    // 插入新的元組
    std::optional<RID> new_rid_optional = table_info->table_->InsertTuple(TupleMeta{0, false}, update_tuple);
    // 遍历所有索引，为每个索引更新对应的条目
    RID new_rid = new_rid_optional.value();
    // 更新所有相关索引，需要先删除直接的索引，然后插入新的索引信息
    for (auto &index_info : indexes) {
      auto index = index_info->index_.get();
      auto key_attrs = index_info->index_->GetKeyAttrs();
      auto old_key = child_tuple.KeyFromTuple(table_info->schema_, *index->GetKeySchema(), key_attrs);
      auto new_key = update_tuple.KeyFromTuple(table_info->schema_, *index->GetKeySchema(), key_attrs);
      // 从索引中删除旧元组的条目
      index->DeleteEntry(old_key, child_rid, this->exec_ctx_->GetTransaction());
      // 向索引中插入新元组的条目
      index->InsertEntry(new_key, new_rid, this->exec_ctx_->GetTransaction());
    }
  }

  std::vector<Value> result = {{TypeId::INTEGER, count}};
  *tuple = Tuple(result, &GetOutputSchema());

  return true;
}
}  // namespace bustub
