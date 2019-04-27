// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/ThreadPool.h"

#include "muduo/base/Exception.h"

#include <assert.h>
#include <stdio.h>

using namespace muduo;

ThreadPool::ThreadPool(const string& nameArg)
  : mutex_(),
    notEmpty_(mutex_),
    notFull_(mutex_),
    name_(nameArg),
    maxQueueSize_(0),
    running_(false)
{
}

ThreadPool::~ThreadPool()
{
  // 如果正在运行则 stop
  if (running_)
  {
    stop();
  }
}

void ThreadPool::start(int numThreads)
{
  assert(threads_.empty());
  running_ = true;
  threads_.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i)
  {
    char id[32];
    snprintf(id, sizeof id, "%d", i+1);
    // 绑定 pool 的 runInThread 函数
    threads_.emplace_back(new muduo::Thread(
          std::bind(&ThreadPool::runInThread, this), name_+id));
    threads_[i]->start();
  }
  // 如果不是多线程 并且有线程初始化回调函数
  if (numThreads == 0 && threadInitCallback_)
  {
    threadInitCallback_();
  }
}

void ThreadPool::stop()
{
  {
    MutexLockGuard lock(mutex_);
    running_ = false;
    notEmpty_.notifyAll();
  }
  for (auto& thr : threads_)
  {
    thr->join();
  }
}

size_t ThreadPool::queueSize() const
{
  MutexLockGuard lock(mutex_);
  return queue_.size();
}

// 存入新的任务
void ThreadPool::run(Task task)
{
  // 如果是单线程(子线程容器为空)，直接运行 task
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    // 在工作线程都满的情况 等待
    while (isFull())
    {
      notFull_.wait();  // 等待未满的条件
    }
    assert(!isFull());

    // 将 task 添加到任务队列中，提醒工作线程去执行
    queue_.push_back(std::move(task));
    notEmpty_.notify(); // 提示未空，有新的任务添加了
  }
}

ThreadPool::Task ThreadPool::take()
{
  MutexLockGuard lock(mutex_);
  // always use a while-loop, due to spurious wakeup
  // 等待工作队列中的元素
  while (queue_.empty() && running_)
  {
    notEmpty_.wait();
  }

  // std::funtional<void()>
  Task task;
  if (!queue_.empty())
  {
    task = queue_.front();
    queue_.pop_front();
    // 取出一个任务后 ， 通知线程池可以放入新的任务
    if (maxQueueSize_ > 0)
    {
      notFull_.notify();
    }
  }
  return task;
}

// 判断 任务队列是否已满
bool ThreadPool::isFull() const
{
  mutex_.assertLocked();
  return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
  try
  {
    // 执行 线程初始化回调函数
    if (threadInitCallback_)
    {
      threadInitCallback_();
    }
    while (running_)
    {
      // std::funtional<void()>
      Task task(take());
      if (task)
      {
        task();
      }
    }
  }
  catch (const Exception& ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
    abort();
  }
  catch (const std::exception& ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    abort();
  }
  catch (...)
  {
    fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
    throw; // rethrow
  }
}

