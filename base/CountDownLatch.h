// check 倒计时，用于等待某个操作完成；比如等待线程开始，在主线程 wait，在新线程完成后 countDown

// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"

namespace muduo
{

// 默认继承方式是 private
class CountDownLatch : noncopyable
{
 public:

  explicit CountDownLatch(int count);

  void wait();        // 等待 count 为 0 

  void countDown();   // 减少 count 值

  int getCount() const;

 private:
  mutable MutexLock mutex_;
  Condition condition_ GUARDED_BY(mutex_);
  int count_ GUARDED_BY(mutex_);
};

}  // namespace muduo
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
