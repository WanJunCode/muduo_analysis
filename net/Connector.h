// check ，连接； 主要用于发起连接  用于 tcpclient

// socket 是一次性的, 当 connect 出错, 我们必须关闭该 socket, 重新创建一个 socket, 
// 但是Connector是可以反复使用的,


// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H

#include "muduo/base/noncopyable.h"
#include "muduo/net/InetAddress.h"

#include <functional>
#include <memory>

namespace muduo
{
namespace net
{

class Channel;
class EventLoop;

class Connector : noncopyable,
                  public std::enable_shared_from_this<Connector>
{
 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;   // 新连接回调函数

  Connector(EventLoop* loop, const InetAddress& serverAddr);
  ~Connector();

  // 设置新的连接 回调函数
  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  void start();  // can be called in any thread
  void restart();  // must be called in loop thread
  void stop();  // can be called in any thread

  const InetAddress& serverAddress() const { return serverAddr_; }

 private:
  // 状态：  未连接、连接中、已连接
  enum States { kDisconnected, kConnecting, kConnected };
  static const int kMaxRetryDelayMs = 30*1000;
  static const int kInitRetryDelayMs = 500;

  void setState(States s) { state_ = s; }
  void startInLoop();
  void stopInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWrite();
  void handleError();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

  EventLoop* loop_;                                 // 所属的EventLoop
  InetAddress serverAddr_;                          // 要连接的server地址
  bool connect_;                                    // atomic 用于标识当前的连接状态
  States state_;  // FIXME: use atomic variable
  std::unique_ptr<Channel> channel_;                // 绑定的 channel
  NewConnectionCallback newConnectionCallback_;     // 回调函数
  int retryDelayMs_;                                // 重连延时
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_CONNECTOR_H
