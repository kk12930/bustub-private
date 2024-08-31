//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_header_page.cpp
//
// Identification: src/storage/page/extendible_htable_header_page.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/extendible_htable_header_page.h"

#include "common/exception.h"

namespace bustub {
void ExtendibleHTableHeaderPage::Init(uint32_t max_depth) {
  this->max_depth_ = max_depth;
  // 初始化目录页ID数组，将所有值设为无效ID
  auto size = MaxSize();
  for (uint32_t i = 0; i < size; ++i) {
    directory_page_ids_[i] = INVALID_PAGE_ID;
  }
}

auto ExtendibleHTableHeaderPage::HashToDirectoryIndex(uint32_t hash) const -> uint32_t {
  if (this->max_depth_ == 0) {
    return 0;
  }
  // 将hash右移(32 - this->max_depth_)位,丢弃哈希值的低this->max_depth_位，保留高位的部分。
  return hash >> (sizeof(uint32_t) * 8 - this->max_depth_);
}

auto ExtendibleHTableHeaderPage::GetDirectoryPageId(uint32_t directory_idx) const -> uint32_t {
  if (directory_idx >= this->MaxSize()) {
    return INVALID_PAGE_ID;
  }
  return this->directory_page_ids_[directory_idx];
}

void ExtendibleHTableHeaderPage::SetDirectoryPageId(uint32_t directory_idx, page_id_t directory_page_id) {
  if (directory_idx >= this->MaxSize()) {
    return;
  }
  directory_page_ids_[directory_idx] = directory_page_id;
}

auto ExtendibleHTableHeaderPage::MaxSize() const -> uint32_t { return 1 << this->max_depth_; }
}  // namespace bustub
