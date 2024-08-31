#include <memory>

#include "execution/executors/delete_executor.h"

namespace bustub {

DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void DeleteExecutor::Init() {
  // 空指针检查
  if (child_executor_ == nullptr) {
    throw std::runtime_error("Child executor is not initialized.");
  }

  child_executor_->Init();
  this->has_deleted_ = false;
}

auto DeleteExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  if (has_deleted_) {
    return false;
  }
  has_deleted_ = true;

  // 获取待删除的表信息及其索引列表
  TableInfo *table_info = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableOid());
  if (table_info == nullptr) {
    throw std::runtime_error("Table not found.");
  }

  std::vector<IndexInfo *> index_info = exec_ctx_->GetCatalog()->GetTableIndexes(table_info->name_);

  // 从子执行器中逐个获取元组并删除
  int delete_count = 0;
  Tuple child_tuple{};
  RID child_rid{};
  while (child_executor_->Next(&child_tuple, &child_rid)) {
    // 删除元组并更新元数据
    table_info->table_->UpdateTupleMeta(TupleMeta{0, true}, child_rid);

    // 更新索引
    for (const auto &index : index_info) {
      // 根据索引的模式从数据元组中构造索引元组，并从索引中删除
      Tuple key_tuple = child_tuple.KeyFromTuple(child_executor_->GetOutputSchema(), index->key_schema_,
                                                 index->index_->GetMetadata()->GetKeyAttrs());
      index->index_->DeleteEntry(key_tuple, child_rid, exec_ctx_->GetTransaction());
    }
    delete_count++;
  }

  // 返回删除的行数
  std::vector<Value> result{Value(INTEGER, delete_count)};
  *tuple = Tuple(result, &plan_->OutputSchema());

  return true;
}

}  // namespace bustub
