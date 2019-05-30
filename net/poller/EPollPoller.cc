// epollfd_.data.ptr 保存 channel 指针

// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/poller/EPollPoller.h"

#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

// 在 linux 上 poll 和 epoll 的事件是相似的
// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
static_assert(EPOLLIN == POLLIN,        "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI,      "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT,      "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP,  "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR,      "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP,      "epoll uses same flag values as poll");

namespace
{
const int kNew = -1;            // 新建
const int kAdded = 1;           // 添加
const int kDeleted = 2;         // 删除
}

// epoll_create 和 epoll_create1 类似，但是后者可以带入一个 flag 参数；
// EPOLL_CLOEXEC 可以防止泄露给 exec 后的进程
EPollPoller::EPollPoller(EventLoop* loop)
  : Poller(loop),       // 父类构造函数
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)   // 初始化 vector 的容量大小
{
  if (epollfd_ < 0)
  {
    LOG_SYSFATAL << "EPollPoller::EPollPoller";
  }
}

EPollPoller::~EPollPoller()
{
  ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
  LOG_TRACE << "fd total count " << channels_.size();
  // 等待 epoll； epoll 本来使用 epoll_event 数组来存储，这里使用了动态的 vector epoll_event
  int numEvents = ::epoll_wait(epollfd_,
                               &*events_.begin(),
                               static_cast<int>(events_.size()),
                               timeoutMs);
  int savedErrno = errno;
  Timestamp now(Timestamp::now());
  if (numEvents > 0)
  {
    LOG_TRACE << numEvents << " events happened";
    // 加载 numEvents 个激活的事件到 activeChannels
    fillActiveChannels(numEvents, activeChannels);
    // 如果监听的所有事件都被激活，则events大小翻倍
    if (implicit_cast<size_t>(numEvents) == events_.size())
    {
      events_.resize(events_.size()*2);
    }
  }
  else if (numEvents == 0)
  {
    LOG_TRACE << "nothing happened";
  }
  else
  {
    // error happens, log uncommon ones 记录epoll_wait发生的错误
    // EINTR 表示被信号打断
    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      LOG_SYSERR << "EPollPoller::poll()";
    }
  }
  return now;
}

// 将 events_ 的前 numEvents 个事件激活 并将 channel 存入 activeChannel 中
void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList* activeChannels) const
{
  // epoll 响应的事件个数一定要小于 events 数组的大小
  assert(implicit_cast<size_t>(numEvents) <= events_.size());
  for (int i = 0; i < numEvents; ++i)
  {
    // muduo 高效的地方，从epoll_event中直接获得channel，然后执行
    Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
    int fd = channel->fd();
    ChannelMap::const_iterator it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);
#endif
    // 设置 channel 中接受到的 event 类型
    channel->set_revents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

// 被调用是因为 channel->enableReading() 再调用 channel->update() 然后调用 event_loop->updateChannel()
// 最后调用 poll 或 epoll 的 updateChannel
// index 在 poll 中是下标，在 epoll 中是三种状态
// kNew || kDeleted => kAdded => update( EPOLL_CTL_ADD )
// kAdded =>  update( EPOLL_CTL_DEL || EPOLL_CTL_MOD )
void EPollPoller::updateChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  // index 表示 channel 的索引（索引表示进行什么操作）
  const int index = channel->index();
  LOG_TRACE << "fd = " << channel->fd()
    << " events = " << channel->events() << " index = " << index;
  // 如果执行的操作是 新建 或者 删除，需要先将channel 添加到 channels map 中
  if (index == kNew || index == kDeleted)
  {
    // a new one, add with EPOLL_CTL_ADD
    int fd = channel->fd();
    if (index == kNew)
    {
      // 添加channel 必须保证 vector 中找不到
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    }
    else // index == kDeleted
    {
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }

    // kAdd kDeleted => kAdded
    channel->set_index(kAdded);
    update(EPOLL_CTL_ADD, channel);   // 增加
  }
  else
  {
    // index == kAdded
    // update existing one with EPOLL_CTL_MOD/DEL
    int fd = channel->fd();
    (void)fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == kAdded);
    // 如果 channel 没有事件 就删除
    if (channel->isNoneEvent())
    {
      update(EPOLL_CTL_DEL, channel);     // 删除
      channel->set_index(kDeleted);
    }
    else
    {
      update(EPOLL_CTL_MOD, channel);     // 更改
    }
  }
}

// 只能在当前线程删除 channel
// kAdded => update( EPOLL_CTL_DEL ) => kNew
// kDeleted => kNew
void EPollPoller::removeChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  int fd = channel->fd();
  LOG_TRACE << "fd = " << fd;
  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  assert(channel->isNoneEvent());
  int index = channel->index();
  // 只有 kAdded 和 kDeleted 是可以被删除的状态
  assert(index == kAdded || index == kDeleted);
  // std::map 删除
  size_t n = channels_.erase(fd);
  (void)n;
  assert(n == 1);

  if (index == kAdded)
  {
    update(EPOLL_CTL_DEL, channel);
  }
  // 设置删除后的channel 状态为 kNew
  channel->set_index(kNew);
}

// 更新对 epollfd 的操作
void EPollPoller::update(int operation, Channel* channel)
{
  struct epoll_event event;
  memZero(&event, sizeof event);
  event.events = channel->events();
  event.data.ptr = channel;
  int fd = channel->fd();
  LOG_TRACE << "epoll_ctl op = " << operationToString(operation)
    << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
  {
    if (operation == EPOLL_CTL_DEL)
    {
      // 删除操作需要记录
      LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
    }
    else
    {
      LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
    }
  }
}

// 打印对 fd 的 epoll 操作
// 添加 、 删除 、 修改
const char* EPollPoller::operationToString(int op)
{
  switch (op)
  {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      assert(false && "ERROR op");
      return "Unknown Operation";
  }
}
