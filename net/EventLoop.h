// check ； loop用于io复用，接受来自客户端的连接以及处理 client fd 上发生的事件


// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <atomic>
#include <functional>
#include <vector>

#include <boost/any.hpp>

#include "muduo/base/Mutex.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/TimerId.h"

namespace muduo
{
namespace net
{

class Channel;
class Poller;
class TimerQueue;       // 时间器队列

///
/// Reactor, at most one per thread.
///
/// This is an interface class, so don't expose too much details.
class EventLoop : noncopyable
{
 public:
  typedef std::function<void()> Functor;

  EventLoop();
  ~EventLoop();  // force out-line dtor, for std::unique_ptr members.

  ///
  /// Loops forever.
  /// 
  /// Must be called in the same thread as creation of the object.
  /// 只能在创建当前实例的线程执行loop函数
  void loop();

  /// Quits loop.
  /// 使用共享指针的loop实例调用可能不安全
  /// This is not 100% thread safe, if you call through a raw pointer,
  /// better to call through shared_ptr<EventLoop> for 100% safety.
  void quit();

  ///
  /// Time when poll returns, usually means data arrival.
  ///
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  int64_t iteration() const { return iteration_; }

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.
  // 在loop中直接调用回调函数
  void runInLoop(Functor cb);

  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  // 将回调函数存入线程池的队列中
  void queueInLoop(Functor cb);

  // 返回队列长度
  size_t queueSize() const;

  // timers

  ///
  /// Runs callback at 'time'.
  /// Safe to call from other threads.
  /// 在指定的事件戳运行时间回调函数
  TimerId runAt(Timestamp time, TimerCallback cb);
  ///
  /// Runs callback after @c delay seconds.
  /// Safe to call from other threads.
  /// 经过delay的延时后调用回调函数
  TimerId runAfter(double delay, TimerCallback cb);
  ///
  /// Runs callback every @c interval seconds.
  /// Safe to call from other threads.
  /// 每过interval时间后调用回调函数
  TimerId runEvery(double interval, TimerCallback cb);
  ///
  /// Cancels the timer.
  /// Safe to call from other threads.
  /// 取消时间器的回调函数
  void cancel(TimerId timerId);

  // internal usage
  void wakeup();
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);
  bool hasChannel(Channel* channel);

  // pid_t threadId() const { return threadId_; }
  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  // bool callingPendingFunctors() const { return callingPendingFunctors_; }
  bool eventHandling() const { return eventHandling_; }

  // 设置context
  void setContext(const boost::any& context)
  { context_ = context; }
  // 获得context
  const boost::any& getContext() const
  { return context_; }
  // 获得非常量的context
  boost::any* getMutableContext()
  { return &context_; }

  // 获得当前线程的eventloop
  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();      // 退出程序
  void handleRead();  // waked up
  void doPendingFunctors();

  void printActiveChannels() const; // DEBUG

  typedef std::vector<Channel*> ChannelList;      // 用于存储channel

  bool looping_; /* atomic */
  std::atomic<bool> quit_;                      // 判断是否离开loop
  bool eventHandling_; /* atomic */
  bool callingPendingFunctors_; /* atomic */
  int64_t iteration_;                           // 记录 loop 循环的次数
  const pid_t threadId_;
  Timestamp pollReturnTime_;                    // poll返回时间戳
  std::unique_ptr<Poller> poller_;
  std::unique_ptr<TimerQueue> timerQueue_;      // 时间队列
  int wakeupFd_;                                // 唤醒fd
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  std::unique_ptr<Channel> wakeupChannel_;      // 唤醒fd 的 channel
  boost::any context_;

  // scratch variables
  ChannelList activeChannels_;                  // 激活的 channel
  Channel* currentActiveChannel_;               // 当前的活动 channel

  mutable MutexLock mutex_;
  std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_);   // 挂起的 函数体
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
