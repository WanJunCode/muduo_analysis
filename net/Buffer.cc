// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "muduo/net/Buffer.h"

#include "muduo/net/SocketsOps.h"

#include <errno.h>
#include <sys/uio.h>

using namespace muduo;
using namespace muduo::net;

// 定义换行字符
const char Buffer::kCRLF[] = "\r\n";

// 在头文件中 声明并定义
const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

// 读取 fd 中的数据
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
  // 使用 FIONREAD 获得要读取多少长度
  // saved an ioctl()/FIONREAD call to tell how much to read
  char extrabuf[65536];
  struct iovec vec[2];        // 1 使用 vector char 2 使用 额外的 extrabuf
  // 获得还可以写入多少数据
  const size_t writable = writableBytes();
  vec[0].iov_base = begin()+writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  // when there is enough space in this buffer, don't read into extrabuf.
  // when extrabuf is used, we read 128k-1 bytes at most.
  // 当空间足够时，不要使用 extrabuf
  const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
  // 读取数据到 iovcnt 个 内存块中
  const ssize_t n = sockets::readv(fd, vec, iovcnt);
  if (n < 0)
  {
    // 保存错误编号
    *savedErrno = errno;
  }
  else if (implicit_cast<size_t>(n) <= writable)
  {
    writerIndex_ += n;
  }
  else
  {
    // 写入索引到尾部
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  // if (n == writable + sizeof extrabuf)
  // {
  //   goto line_30;
  // }
  return n;
}

