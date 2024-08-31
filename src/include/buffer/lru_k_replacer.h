//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.h
//
// Identification: src/include/buffer/lru_k_replacer.h
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <algorithm>
#include <limits>
#include <list>
#include <mutex>  // NOLINT
#include <unordered_map>
#include <vector>

#include "common/config.h"
#include "common/macros.h"

namespace bustub {

enum class AccessType { Unknown = 0, Lookup, Scan, Index };

class LRUKNode {
 private:
  /** History of last seen K timestamps of this page. Least recent timestamp stored in front. */
  // Remove maybe_unused if you start using them. Feel free to change the member variables as you want.

  /** 该页面最后看到的 K 个时间戳的历史记录。最近的时间戳存储在前面。 */
  // 如果您开始使用它们，请删除 Maybe_unused。您可以根据需要随意更改成员变量。
  [[maybe_unused]] std::list<size_t> history_;
  [[maybe_unused]] size_t k_;
  [[maybe_unused]] frame_id_t fid_;
  [[maybe_unused]] bool is_evictable_{false};
};

/**
 * LRUKReplacer implements the LRU-k replacement policy.
 *
 * The LRU-k algorithm evicts a frame whose backward k-distance is maximum
 * of all frames. Backward k-distance is computed as the difference in time between
 * current timestamp and the timestamp of kth previous access.
 *
 * A frame with less than k historical references is given
 * +inf as its backward k-distance. When multiple frames have +inf backward k-distance,
 * classical LRU algorithm is used to choose victim.
 */

/**
 * LRUKReplacer 实现了 LRU-k 替换策略。
 *
 * LRU-k算法驱逐向后k距离最大的帧
 * 所有帧。后向 k 距离计算为之间的时间差
 * 当前时间戳和第 k 次访问的时间戳。
 *
 * 给出了少于k个历史参考的框架
 * +inf 为其后向 k 距离。当多个帧具有+inf后向k距离时，
 * 使用经典的LRU算法来选择受害者。
 */
class LRUKReplacer {
 public:
  /**
   *
   * TODO(P1): Add implementation
   *
   * @brief a new LRUKReplacer.
   * @param num_frames the maximum number of frames the LRUReplacer will be required to store
   */

  /**
   *
   * TODO(P1): 添加实现
   *
   * @brief 一个新的 LRUKReplacer。
   * @param num_frames LRUReplacer 需要存储的最大帧数
   */
  explicit LRUKReplacer(size_t num_frames, size_t k);

  DISALLOW_COPY_AND_MOVE(LRUKReplacer);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Destroys the LRUReplacer.
   */
  /**
   * TODO(P1): 添加实现
   *
   * @brief 销毁 LRUReplacer。
   */
  ~LRUKReplacer() = default;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Find the frame with largest backward k-distance and evict that frame. Only frames
   * that are marked as 'evictable' are candidates for eviction.
   *
   * A frame with less than k historical references is given +inf as its backward k-distance.
   * If multiple frames have inf backward k-distance, then evict frame with earliest timestamp
   * based on LRU.
   *
   * Successful eviction of a frame should decrement the size of replacer and remove the frame's
   * access history.
   *
   * @param[out] frame_id id of frame that is evicted.
   * @return true if a frame is evicted successfully, false if no frames can be evicted.
   */
  /**
   * TODO(P1): 添加实现
   *
   * @brief 找到具有最大向后 k 距离的帧并逐出该帧。仅框架
   * 标记为“可驱逐”的是要驱逐的候选者。
   *
   * 具有少于 k 个历史参考的帧被赋予 +inf 作为其向后 k 距离。
   * 如果多个帧具有 inf 向后 k 距离，则逐出具有最早时间戳的帧
   * 基于LRU。
   *
   * 成功驱逐框架应该减少替换者的大小并删除框架的
   * 访问历史记录。
   *
   * @param[out]frame_id 被驱逐的帧的ID。
   * @return true 如果帧被成功驱逐， false 如果没有帧可以被驱逐。
   */
  auto Evict(frame_id_t *frame_id) -> bool;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Record the event that the given frame id is accessed at current timestamp.
   * Create a new entry for access history if frame id has not been seen before.
   *
   * If frame id is invalid (ie. larger than replacer_size_), throw an exception. You can
   * also use BUSTUB_ASSERT to abort the process if frame id is invalid.
   *
   * @param frame_id id of frame that received a new access.
   * @param access_type type of access that was received. This parameter is only needed for
   * leaderboard tests.
   */

  /**
   * TODO(P1): 添加实现
   *
   * @brief 记录在当前时间戳访问给定帧 ID 的事件。
   * 如果之前没有见过帧 ID，则为访问历史记录创建一个新条目。
   *
   * 如果帧id无效（即大于replacer_size_），则抛出异常。你可以
   * 如果帧 ID 无效，还可以使用 BUSTUB_ASSERT 中止进程。
   *
   * @param frame_id 接收新访问的帧的 ID。
   * @param access_type 收到的访问类型。仅需要此参数
   * 排行榜测试。
   */
  void RecordAccess(frame_id_t frame_id, AccessType access_type = AccessType::Unknown);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Toggle whether a frame is evictable or non-evictable. This function also
   * controls replacer's size. Note that size is equal to number of evictable entries.
   *
   * If a frame was previously evictable and is to be set to non-evictable, then size should
   * decrement. If a frame was previously non-evictable and is to be set to evictable,
   * then size should increment.
   *
   * If frame id is invalid, throw an exception or abort the process.
   *
   * For other scenarios, this function should terminate without modifying anything.
   *
   * @param frame_id id of frame whose 'evictable' status will be modified
   * @param set_evictable whether the given frame is evictable or not
   */
  /**
   * TODO(P1): 添加实现
   *
   * @brief 切换帧是可逐出的还是不可逐出的。这个功能还
   * 控制替代品的大小。请注意，大小等于可驱逐条目的数量。
   *
   * 如果一个框架以前是可驱逐的并且要设置为不可驱逐的，那么大小应该
   * 递减。如果某个帧以前是不可驱逐的并且要设置为可驱逐的，
   * 那么大小应该增加。
   *
   * 如果帧id无效，则抛出异常或中止进程。
   *
   * 对于其他场景，该函数应该终止而不做任何修改。
   *
   * @param frame_id '可退出'状态将被修改的帧的ID
   * @param set_evictable 给定的帧是否可逐出
   */
  void SetEvictable(frame_id_t frame_id, bool set_evictable);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Remove an evictable frame from replacer, along with its access history.
   * This function should also decrement replacer's size if removal is successful.
   *
   * Note that this is different from evicting a frame, which always remove the frame
   * with largest backward k-distance. This function removes specified frame id,
   * no matter what its backward k-distance is.
   *
   * If Remove is called on a non-evictable frame, throw an exception or abort the
   * process.
   *
   * If specified frame is not found, directly return from this function.
   *
   * @param frame_id id of frame to be removed
   */
  /**
   * TODO(P1): 添加实现
   *
   * @brief 从替换器中删除可逐出的框架及其访问历史记录。
   * 如果删除成功，此函数还应减少替换程序的大小。
   *
   * 请注意，这与逐出框架不同，后者总是删除框架
   * 具有最大的向后 k 距离。此函数删除指定的帧 ID，
   * 无论其向后 k 距离是多少。
   *
   * 如果在不可驱逐的框架上调用Remove，则抛出异常或中止
   * 过程。
   *
   * 如果没有找到指定的帧，则直接从此函数返回。
   *
   * @paramframe_id 要删除的帧的id
   */

  void Remove(frame_id_t frame_id);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Return replacer's size, which tracks the number of evictable frames.
   *
   * @return size_t
   */

  /**
   * TODO(P1): 添加实现
   *
   * @brief 返回替换器的大小，它跟踪可逐出帧的数量。
   *
   * @返回尺寸_t
   */
  auto Size() -> size_t;

 private:
  // Remove maybe_unused if you start using them.
  // 如果您开始使用它们，请删除 Maybe_unused。
  size_t current_timestamp_{0};  //当前的时间戳,每进行一次record操作加一
  size_t curr_size_{0};          //当前存放的可驱逐页面数量
  size_t max_size_;              //最多可驱逐页面数量
  size_t replacer_size_;         // replacer_size_
  size_t k_;                     // lru-k的k
  std::mutex latch_;

  using timestamp = std::list<size_t>;                   //记录单个页时间戳的列表
  using k_time = std::pair<frame_id_t, size_t>;          //页号对应的第k次的时间戳
  std::unordered_map<frame_id_t, timestamp> hist_;       //用于记录所有页的时间戳
  std::unordered_map<frame_id_t, size_t> recorded_cnt_;  //用于记录,访问了多少次
  std::unordered_map<frame_id_t, bool> evictable_;       //用于记录是否可以被驱逐

  std::list<frame_id_t> new_frame_;  //用于记录不满k次的页
  std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> new_locate_;

  std::list<k_time> cache_frame_;  //用于记录到达k次的页
  std::unordered_map<frame_id_t, std::list<k_time>::iterator> cache_locate_;
  static auto CmpTimestamp(const k_time &f1, const k_time &f2) -> bool;
};

}  // namespace bustub
