// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_CALLBACKS_H
#define MUDUO_NET_CALLBACKS_H

#include "muduo/base/Timestamp.h"

#include <functional>
#include <memory>

namespace muduo
{

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

// should really belong to base/Types.h, but <memory> is not included there.

// 获得 shared_ptr 和 unique_ptr 中的 普通指针
template<typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr)
{
  return ptr.get();
}

template<typename T>
inline T* get_pointer(const std::unique_ptr<T>& ptr)
{
  return ptr.get();
}

// Adapted from google-protobuf stubs/common.h
// see License in muduo/base/Types.h
template<typename To, typename From>
inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f) {
  if (false)
  {
    implicit_cast<From*, To*>(0);
  }

#ifndef NDEBUG
  assert(f == NULL || dynamic_cast<To*>(get_pointer(f)) != NULL);
#endif
  return ::std::static_pointer_cast<To>(f);
}

// muduo::net
namespace net
{

// All client visible callbacks go here.

class Buffer;
class TcpConnection;
// 重命名 所有客户端可见的灰调函数
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;      // tcp 连接 智能指针
typedef std::function<void()> TimerCallback;                  // 时间器 回调函数
typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback; // tcp 连接回调函数
typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;      // tcp 关闭回调函数
typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;  // tcp 写入完成回调函数
typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;    // 高水位标记回调函数

// the data has been read to (buf, len)
typedef std::function<void (const TcpConnectionPtr&,
                            Buffer*,
                            Timestamp)> MessageCallback;      // 信息回调函数

void defaultConnectionCallback(const TcpConnectionPtr& conn);   // 默认的连接回调函数
void defaultMessageCallback(const TcpConnectionPtr& conn,       // 默认的message回调函数
                            Buffer* buffer,
                            Timestamp receiveTime);

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_CALLBACKS_H
