//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include "common/exception.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) { max_size_ = num_frames; }

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  if (Size() == 0) {
    return false;
  }
  for (auto it = new_frame_.rbegin(); it != new_frame_.rend(); it++) {
    auto frame = *it;
    if (evictable_[frame]) {
      recorded_cnt_[frame] = 0;
      new_locate_.erase(frame);
      new_frame_.erase(std::next(it).base());
      *frame_id = frame;
      curr_size_--;
      hist_[frame].clear();
      return true;
    }
  }

  for (auto it = cache_frame_.begin(); it != cache_frame_.end(); it++) {
    auto frame = (*it).first;
    if (evictable_[frame]) {
      recorded_cnt_[frame] = 0;
      cache_frame_.erase(it);
      cache_locate_.erase(frame);
      *frame_id = frame;
      curr_size_--;
      hist_[frame].clear();
      return true;
    }
  }
  return false;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
  std::lock_guard<std::mutex> lock(latch_);

  if (frame_id > static_cast<frame_id_t>(replacer_size_)) {
    throw std::exception();
  }
  current_timestamp_++;
  recorded_cnt_[frame_id]++;
  auto cnt = recorded_cnt_[frame_id];
  hist_[frame_id].push_back(current_timestamp_);
  /**
   * 如果是新加入的记录
   */
  if (cnt == 1) {
    if (curr_size_ == max_size_) {
      frame_id_t frame;
      Evict(&frame);
    }
    evictable_[frame_id] = true;
    curr_size_++;
    new_frame_.push_front(frame_id);
    new_locate_[frame_id] = new_frame_.begin();
  }
  /**
   * 如果记录达到k次，则需要从新队列中加入到老队列中
   */
  if (cnt == k_) {
    new_frame_.erase(new_locate_[frame_id]);  //从新队列中删除
    new_locate_.erase(frame_id);

    auto kth_time = hist_[frame_id].front();  //获取当前页面的倒数第k次出现的时间
    k_time new_cache(frame_id, kth_time);
    auto it = std::upper_bound(cache_frame_.begin(), cache_frame_.end(), new_cache, CmpTimestamp);  //找到该插入的位置
    it = cache_frame_.insert(it, new_cache);
    cache_locate_[frame_id] = it;
    return;
  }
  /**
   * 如果记录在k次以上，需要将该frame放到指定的位置
   */
  if (cnt > k_) {
    hist_[frame_id].erase(hist_[frame_id].begin());
    cache_frame_.erase(cache_locate_[frame_id]);  //去除原来的位置
    auto kth_time = hist_[frame_id].front();      //获取当前页面的倒数第k次出现的时间
    k_time new_cache(frame_id, kth_time);

    auto it = std::upper_bound(cache_frame_.begin(), cache_frame_.end(), new_cache, CmpTimestamp);  //找到该插入的位置
    it = cache_frame_.insert(it, new_cache);
    cache_locate_[frame_id] = it;
    return;
  }
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::lock_guard<std::mutex> lock(latch_);

  // 如果该frame没有被记录过，直接返回
  if (recorded_cnt_[frame_id] == 0) {
    return;
  }

  auto &status = evictable_[frame_id];

  // 如果状态没有改变，不做任何操作
  if (status == set_evictable) {
    return;
  }

  // 更新状态
  status = set_evictable;

  // 调整 max_size_ 和 curr_size_
  if (set_evictable) {
    ++max_size_;
    ++curr_size_;
  } else {
    --max_size_;
    --curr_size_;
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::lock_guard<std::mutex> lock(latch_);

  if (frame_id > static_cast<frame_id_t>(replacer_size_)) {
    throw std::exception();
  }

  auto &cnt = recorded_cnt_[frame_id];
  if (cnt == 0) {
    return;  // 如果计数为0，直接返回，避免不必要的操作
  }

  if (!evictable_[frame_id]) {
    throw std::exception();  // 如果帧不可驱逐，抛出异常
  }

  // 根据 cnt 判断是在 new_frame_ 还是 cache_frame_ 中
  auto erase_frame = [&](auto &frame_container, auto &locate_map) {
    frame_container.erase(locate_map[frame_id]);
    locate_map.erase(frame_id);
    cnt = 0;
    hist_[frame_id].clear();
    curr_size_--;
  };

  if (cnt < k_) {
    erase_frame(new_frame_, new_locate_);
  } else {
    erase_frame(cache_frame_, cache_locate_);
  }
}

auto LRUKReplacer::Size() -> size_t { return curr_size_; }
auto LRUKReplacer::CmpTimestamp(const LRUKReplacer::k_time &f1, const LRUKReplacer::k_time &f2) -> bool {
  return f1.second < f2.second;
}
}  // namespace bustub
