// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/AsyncLogging.h"
#include "muduo/base/LogFile.h"
#include "muduo/base/Timestamp.h"

#include <stdio.h>

using namespace muduo;

AsyncLogging::AsyncLogging(const string& basename,
                           off_t rollSize,
                           int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    rollSize_(rollSize),
    thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"), // std::functional<void()>
    latch_(1),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_()
{
  // 初始化 缓冲区
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  // vector reserve 16个
  buffers_.reserve(16);
}

// 添加日志记录到缓冲区
void AsyncLogging::append(const char* logline, int len)
{
  muduo::MutexLockGuard lock(mutex_);
  if (currentBuffer_->avail() > len)
  {
    currentBuffer_->append(logline, len);
  }
  else
  {
    // 将使用完后的 buffer 添加到 buffer vector 后
    buffers_.push_back(std::move(currentBuffer_));

    if (nextBuffer_)
    {
      currentBuffer_ = std::move(nextBuffer_);
    }
    else
    {
      // 如果 备用buffer 也用完了
      // Buffer 是 unique_ptr 不需要显示调用 delete
      currentBuffer_.reset(new Buffer); // Rarely happens 极少数会发生
    }
    currentBuffer_->append(logline, len);
    // 发出信号，有新的 buffer 添加到 vector 后面
    cond_.notify();
  }
}

// 异步线程执行函数
void AsyncLogging::threadFunc()
{
  assert(running_ == true);
  latch_.countDown();
  // 创建一个 日志文件 LogFile
  LogFile output(basename_, rollSize_, false);
  // BufferPtr 属于 unique_ptr 只能使用 std::move 移动
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  BufferVector buffersToWrite;  // 要写入 LogFile 的 buffer vector
  buffersToWrite.reserve(16);

  // loop
  while (running_)
  {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());
    // buffersToWrite 必须是空的，newBuffer1、newBuffer2必须是空的并且可用


    // 等待主程序，与主程序交互，交换 buffer vector 完成数据的写入
    {
      muduo::MutexLockGuard lock(mutex_);
      // buffers_ 是主程序中写入的缓冲区
      if (buffers_.empty())  // unusual usage!  主程序中没有写入日志
      {
        // 等待主程序中 vector 有新的 buffer 被添加
        cond_.waitForSeconds(flushInterval_);
      }
      // 将主程序中正在使用的 currentBuffer_ 存入 buffers_
      buffers_.push_back(std::move(currentBuffer_));
      // 使用 newBuffer1 替换正在使用的 currentBuffer_
      currentBuffer_ = std::move(newBuffer1);
      // 使用新的未使用的 buffersToWrite 交换 buffers_，将buffers_中的数据在异步线程中写入 LogFile 中
      buffersToWrite.swap(buffers_);
      if (!nextBuffer_)
      {
        // 如果主程序中的 nextBuffer_ 为nullptr ，使用 newBuffer2 代替
        nextBuffer_ = std::move(newBuffer2);
      }
    }

    // 从主程序中获得的 buffer vector 不可以是空的
    assert(!buffersToWrite.empty());  // 不可以是 nullptr

    // 如果从主程序获得的 buffer vector 长度大于 25，则删除到只剩下 2 个 buffer
    if (buffersToWrite.size() > 25)
    {
      char buf[256];
      snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               buffersToWrite.size()-2);
      // 输出到标准错误流
      fputs(buf, stderr);
      // output is LogFile 
      output.append(buf, static_cast<int>(strlen(buf)));
      // buffer vector 只留下两个 buffer 使用 std::move 移动，节省资源
      buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
    }

    // 将 buffer vector 中的 数据写入 LogFile 中
    for (const auto& buffer : buffersToWrite)
    {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffer->data(), buffer->length());
    }

    if (buffersToWrite.size() > 2)
    {
      // drop non-bzero-ed buffers, avoid trashing 丢弃非零的缓冲区，避免垃圾
      buffersToWrite.resize(2);
    }

    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());  // not null
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }

  output.flush();
}

