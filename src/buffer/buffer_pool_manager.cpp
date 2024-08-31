//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"

#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/page_guard.h"

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_scheduler_(std::make_unique<DiskScheduler>(disk_manager)), log_manager_(log_manager) {
  // TODO(students): remove this line after you have implemented the buffer pool manager
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  Page *page;
  frame_id_t frame_id = -1;
  std::scoped_lock lock(latch_);
  // 如果free_list_里有值
  if (!free_list_.empty()) {
    // 获取free_list_容器的最后一个元素并移除，并让page为新内存地址
    frame_id = free_list_.back();
    free_list_.pop_back();
    page = pages_ + frame_id;
  } else {
    // free_list_里没值，看replacer_里有没有能替换的
    if (!replacer_->Evict(&frame_id)) {
      return nullptr;
    }
    page = pages_ + frame_id;
  }
  // 和flushpage方法一样，如果page地址上原frame里存放的从内存中拿出的page_id对应的页是脏的
  if (page->IsDirty()) {
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
    future.get();
    // ! clean
    page->is_dirty_ = false;
  }
  // 获取一个新的页面ID(注释里给的)
  *page_id = AllocatePage();
  // 把旧的映射删掉
  page_table_.erase(page->GetPageId());
  // 建立新的映射
  page_table_.emplace(*page_id, frame_id);
  // 把新page的参数更新下
  page->page_id_ = *page_id;
  page->pin_count_ = 1;
  // ResetMemory方法：将页面中的所有数据清零
  page->ResetMemory();
  // 更新replacer_
  replacer_->RecordAccess(frame_id);
  replacer_->SetEvictable(frame_id, false);
  return page;
}

auto BufferPoolManager::FetchPage(page_id_t page_id, [[maybe_unused]] AccessType access_type) -> Page * {
  if (page_id == INVALID_PAGE_ID) {
    return nullptr;
  }
  std::scoped_lock lock(latch_);
  if (page_table_.find(page_id) != page_table_.end()) {
    // ! get page
    auto frame_id = page_table_[page_id];
    auto page = pages_ + frame_id;
    // 更新replacer
    replacer_->RecordAccess(frame_id);
    replacer_->SetEvictable(frame_id, false);
    // ! update pin count
    page->pin_count_ += 1;
    return page;
  }
  // Newpage 方法里的
  Page *page;
  frame_id_t frame_id = -1;
  if (!free_list_.empty()) {
    frame_id = free_list_.back();
    free_list_.pop_back();
    page = pages_ + frame_id;
  } else {
    if (!replacer_->Evict(&frame_id)) {
      return nullptr;
    }
    page = pages_ + frame_id;
  }
  if (page->IsDirty()) {
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
    future.get();
    page->is_dirty_ = false;
  }
  page_table_.erase(page->GetPageId());
  page_table_.emplace(page_id, frame_id);
  page->page_id_ = page_id;
  page->pin_count_ = 1;
  page->ResetMemory();
  replacer_->RecordAccess(frame_id);
  replacer_->SetEvictable(frame_id, false);
  // 从磁盘中读页，读完后写回（之前的是写完后写回）
  auto promise = disk_scheduler_->CreatePromise();
  auto future = promise.get_future();
  disk_scheduler_->Schedule({false, page->GetData(), page->GetPageId(), std::move(promise)});
  future.get();
  return page;
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, [[maybe_unused]] AccessType access_type) -> bool {
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }
  std::scoped_lock lock(latch_);
  if (page_table_.find(page_id) == page_table_.end()) {
    return false;
  }
  auto frame_id = page_table_[page_id];
  auto page = pages_ + frame_id;
  // 设置脏位,如果原本是脏的或传进的is_dirty是脏的，最终就是脏的
  page->is_dirty_ = is_dirty || page->is_dirty_;
  // if pin count is 0
  if (page->GetPinCount() == 0) {
    return false;
  }
  // pin要-1
  page->pin_count_ -= 1;
  // 如果-1后为0,调用lru-k中的SetEvictable方法，把帧设为可驱逐的
  if (page->GetPinCount() == 0) {
    replacer_->SetEvictable(frame_id, true);
  }
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }
  std::scoped_lock lock(latch_);
  // 如果映射里没有
  if (page_table_.find(page_id) == page_table_.end()) {
    return false;
  }
  // 获得page_id在缓冲池中的位置
  auto page = pages_ + page_table_[page_id];
  // 写回，这里creatpromise方法返回了一个std::promise对象
  auto promise = disk_scheduler_->CreatePromise();
  auto future = promise.get_future();
  disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
  future.get();
  // 赃位恢复
  page->is_dirty_ = false;
  return true;
}

void BufferPoolManager::FlushAllPages() {
  std::scoped_lock lock(latch_);
  for (size_t current_size = 0; current_size < pool_size_; current_size++) {
    // 获得page_id在缓冲池中的位置
    auto page = pages_ + current_size;
    if (page->GetPageId() == INVALID_PAGE_ID) {
      continue;
    }
    // 和flush方法一样
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
    future.get();
    page->is_dirty_ = false;
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  if (page_id == INVALID_PAGE_ID) {
    return true;
  }
  std::scoped_lock lock(latch_);
  // 如果页面存在
  if (page_table_.find(page_id) != page_table_.end()) {
    auto frame_id = page_table_[page_id];
    auto page = pages_ + frame_id;
    // 如果页面用着呢
    if (page->GetPinCount() > 0) {
      return false;
    }
    // 删除页面
    page_table_.erase(page_id);
    free_list_.push_back(frame_id);
    replacer_->Remove(frame_id);
    // 把内存该清的清，page的参数该换的换
    page->ResetMemory();
    page->page_id_ = INVALID_PAGE_ID;
    page->is_dirty_ = false;
    page->pin_count_ = 0;
  }
  // 注释里要求的：调用DeallocatePage()来模仿在磁盘上释放页面。
  DeallocatePage(page_id);
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard {
  auto page = FetchPage(page_id);
  return {this, page};
}

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  auto page = FetchPage(page_id);
  if (page != nullptr) {
    page->RLatch();
  }
  return {this, page};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  auto page = FetchPage(page_id);
  if (page != nullptr) {
    page->WLatch();
  }
  return {this, page};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard {
  auto page = NewPage(page_id);
  return {this, page};
}
}  // namespace bustub
