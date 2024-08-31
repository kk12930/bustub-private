//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_executor.cpp
//
// Identification: src/execution/aggregation_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>
#include <vector>

#include "execution/executors/aggregation_executor.h"

namespace bustub {

AggregationExecutor::AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                                         std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) {
  this->plan_ = plan;
  this->child_executor_ = std::move(child_executor);
}

void AggregationExecutor::Init() {
  child_executor_->Init();
  //获取聚合表达式
  auto agg_exprs = plan_->GetAggregates();
  //获取聚合类型
  auto agg_types = plan_->GetAggregateTypes();
  // 根据聚合表达式以及聚合类型创建哈希表
  // SimpleAggregationHashTable 是一个在聚合查询中专为计算聚合设计的散列表（哈希表）
  // 它用于快速地分组数据，并对每个分组应用聚合函数。
  aht_ = std::make_unique<SimpleAggregationHashTable>(plan_->GetAggregates(), plan_->GetAggregateTypes());
  // NEXT方法的輸出參數，用于存储查询结果
  Tuple child_tuple{};
  RID rid{};
  // 遍历子执行器，将子执行器中的获取的数据插入到聚合哈希表中
  // 不能在聚合执行器中完成，因为聚合执行器需要先从子执行器中获取所有数据，然后对这些数据进行分组和聚合操作，最后才能产生输出结果
  while (child_executor_->Next(&child_tuple, &rid)) {
    // 通过tuple获取聚合键和聚合值
    // 聚合键在聚合操作中用来区分不同的分组
    auto agg_key = MakeAggregateKey(&child_tuple);
    auto agg_val = MakeAggregateValue(&child_tuple);
    // 将聚合键和聚合值插入到聚合哈希表中
    aht_->InsertCombine(agg_key, agg_val);
  }
  // 一个指向哈希表开始的迭代器，后面用于遍历哈希表并生成聚合查询的结果。
  aht_iterator_ = std::make_unique<SimpleAggregationHashTable::Iterator>(aht_->Begin());
  has_inserted_ = false;
}

auto AggregationExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (aht_->Begin() != aht_->End()) {
    if (*aht_iterator_ == aht_->End()) {
      return false;
    }
    // 获取聚合键和聚合值
    auto agg_key = aht_iterator_->Key();
    auto agg_val = aht_iterator_->Val();
    //根据聚合键和聚合值生成查询结果元组
    std::vector<Value> values{};
    // 遍历聚合键和聚合值，生成查询结果元组
    // 根据文件要求，有groupby和aggregate两个部分的情况下，groupby也要算上，都添加到value中
    values.reserve(agg_key.group_bys_.size() + agg_val.aggregates_.size());
    for (auto &group_values : agg_key.group_bys_) {
      values.emplace_back(group_values);
    }
    for (auto &agg_value : agg_val.aggregates_) {
      values.emplace_back(agg_value);
    }
    *tuple = {values, &GetOutputSchema()};
    //迭代到下一个聚合键和聚合值
    ++*aht_iterator_;
    // 表示成功返回了一个聚合结果
    return true;
  }
  if (has_inserted_) {
    return false;
  }
  has_inserted_ = true;
  // 没有groupby语句则生成一个初始的聚合值元组并返回
  if (plan_->GetGroupBys().empty()) {
    std::vector<Value> values{};
    Tuple tuple_buffer{};
    // 检查当前表是否为空，如果为空生成默认的聚合值，对于 count(*) 来说是 0，对于其他聚合函数来说是 integer_null(
    // 默认聚合值要求由GenerateInitialAggregateValue实现
    for (auto &agg_value : aht_->GenerateInitialAggregateValue().aggregates_) {
      values.emplace_back(agg_value);
    }
    *tuple = {values, &GetOutputSchema()};
    return true;
  }
  return false;
}

auto AggregationExecutor::GetChildExecutor() const -> const AbstractExecutor * { return child_executor_.get(); }
}  // namespace bustub
