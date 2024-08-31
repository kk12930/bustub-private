#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/comparison_expression.h"
#include "execution/expressions/logic_expression.h"
#include "execution/plans/index_scan_plan.h"
#include "execution/plans/seq_scan_plan.h"
#include "optimizer/optimizer.h"
#include "storage/index/generic_key.h"

namespace bustub {

auto Optimizer::OptimizeSeqScanAsIndexScan(const bustub::AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // TODO(student): implement seq scan with predicate -> index scan optimizer rule
  // The Filter Predicate Pushdown has been enabled for you in optimizer.cpp when forcing starter rule
  // 对所有子节点递归应用这一优化
  std::vector<bustub::AbstractPlanNodeRef> optimized_children;
  for (const auto &child : plan->GetChildren()) {
    optimized_children.emplace_back(OptimizeSeqScanAsIndexScan(child));
  }
  // 递归转化其所有的子节点
  auto optimized_plan = plan->CloneWithChildren(std::move(optimized_children));
  // 如果plan计划为顺序扫描，则转化为索引扫描
  if (optimized_plan->GetType() == PlanType::SeqScan) {
    const auto &seq_plan = dynamic_cast<const bustub::SeqScanPlanNode &>(*optimized_plan);
    // 获取计划的谓词（where语句之后的内容）
    auto predicate = seq_plan.filter_predicate_;
    // 如果谓词为空仍然执行顺序扫描
    if (predicate != nullptr) {
      auto table_name = seq_plan.table_name_;
      // 获取表的索引，看该表是否支持索引扫描
      auto table_idx = catalog_.GetTableIndexes(table_name);
      // 将predicate转化为LogicExpression，查看是否为逻辑谓词
      auto logic_expr = std::dynamic_pointer_cast<LogicExpression>(predicate);
      // 沒有索引或者有多个谓词条件,返回顺序扫描
      if (!table_idx.empty() && !logic_expr) {
        auto equal_expr = std::dynamic_pointer_cast<ComparisonExpression>(predicate);
        // 需要判断是否为条件谓词
        if (equal_expr) {
          auto com_type = equal_expr->comp_type_;
          // 只能是等值判断才能转化为索引扫描
          if (com_type == ComparisonType::Equal) {
            // 获取表的id
            auto table_oid = seq_plan.table_oid_;
            // 返回索引扫描节点
            auto column_expr = dynamic_cast<const ColumnValueExpression &>(*equal_expr->GetChildAt(0));
            // 根据谓词的列，获取表的索引信息
            auto column_index = column_expr.GetColIdx();
            auto col_name = this->catalog_.GetTable(table_oid)->schema_.GetColumn(column_index).GetName();
            // 如果存在相关索引，获取表索引info
            for (auto *index : table_idx) {
              const auto &columns = index->index_->GetKeyAttrs();
              std::vector<uint32_t> column_ids;
              column_ids.push_back(column_index);
              if (columns == column_ids) {
                // 获取pred-key
                auto pred_key = std::dynamic_pointer_cast<ConstantValueExpression>(equal_expr->GetChildAt(1));
                // 从智能指针中获取裸指针
                ConstantValueExpression *raw_pred_key = pred_key ? pred_key.get() : nullptr;
                return std::make_shared<IndexScanPlanNode>(seq_plan.output_schema_, table_oid, index->index_oid_,
                                                           predicate, raw_pred_key);
              }
            }
          }
        }
      }
    }
  }
  return optimized_plan;
}

}  // namespace bustub
