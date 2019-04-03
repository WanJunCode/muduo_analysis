// 读取文件工具类

// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/FileUtil.h"
#include "muduo/base/Logging.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace muduo;

FileUtil::AppendFile::AppendFile(StringArg filename)
  : fp_(::fopen(filename.c_str(), "ae")),  // 'e' for O_CLOEXEC
    writtenBytes_(0)
{
  assert(fp_);
  // 给文件设置 buffer 缓冲
  ::setbuffer(fp_, buffer_, sizeof buffer_);
  // posix_fadvise POSIX_FADV_DONTNEED ?
}

FileUtil::AppendFile::~AppendFile()
{
  ::fclose(fp_);
}

void FileUtil::AppendFile::append(const char* logline, const size_t len)
{
  size_t n = write(logline, len);
  size_t remain = len - n;
  // 循环写入，直到写入 len 长度
  while (remain > 0)
  {
    size_t x = write(logline + n, remain);
    if (x == 0)
    {
      int err = ferror(fp_);
      if (err)
      {
        fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
      }
      break;
    }
    n += x;
    remain = len - n; // remain -= x
  }

  writtenBytes_ += len;
}

void FileUtil::AppendFile::flush()
{
  ::fflush(fp_);
}

size_t FileUtil::AppendFile::write(const char* logline, size_t len)
{
  // #undef fwrite_unlocked
  return ::fwrite_unlocked(logline, 1, len, fp_);
}

FileUtil::ReadSmallFile::ReadSmallFile(StringArg filename)
  : fd_(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)),
    err_(0)
{
  buf_[0] = '\0';
  if (fd_ < 0)
  {
    err_ = errno;
  }
}

FileUtil::ReadSmallFile::~ReadSmallFile()
{
  if (fd_ >= 0)
  {
    ::close(fd_); // FIXME: check EINTR
  }
}

// return errno
template<typename String>
int FileUtil::ReadSmallFile::readToString(int maxSize,
                                          String* content,
                                          int64_t* fileSize,
                                          int64_t* modifyTime,
                                          int64_t* createTime)
{
  // 静态断言
  static_assert(sizeof(off_t) == 8, "_FILE_OFFSET_BITS = 64");
  assert(content != NULL);
  int err = err_;
  if (fd_ >= 0)
  {
    // std::string::clear();
    content->clear();

    if (fileSize)
    {
      struct stat statbuf;
      // 获取 文件的信息 存放在 statbuf 中
      if (::fstat(fd_, &statbuf) == 0)
      {
        // 判断是否是一个常规文件
        if (S_ISREG(statbuf.st_mode))
        {
          // 获得文件的大小
          *fileSize = statbuf.st_size;
          // std::string::reserve()
          content->reserve(static_cast<int>(std::min(implicit_cast<int64_t>(maxSize), *fileSize)));
        }
        else if (S_ISDIR(statbuf.st_mode))
        {
          err = EISDIR;
        }
        if (modifyTime)
        {
          *modifyTime = statbuf.st_mtime;
        }
        if (createTime)
        {
          *createTime = statbuf.st_ctime;
        }
      }
      else
      {
        err = errno;
      }
    }

    // 读取的文件数据大小 < 64K
    // content->size() == 0
    while (content->size() < implicit_cast<size_t>(maxSize))
    {
      size_t toRead = std::min(implicit_cast<size_t>(maxSize) - content->size(), sizeof(buf_));
      ssize_t n = ::read(fd_, buf_, toRead);
      if (n > 0)
      {
        // 从 buffer 中读取数据添加到 string piece 中
        content->append(buf_, n);
      }
      else
      {
        if (n < 0)
        {
          err = errno;
        }
        break;
      }
    }
  }
  return err;
}

// 返回值 errno
// 参数返回得到 pread 读取到的数据大小
int FileUtil::ReadSmallFile::readToBuffer(int* size)
{
  int err = err_;
  if (fd_ >= 0)
  {
    // pread简单来说就是在指定偏移offset位置开始读取count个字节，同理可推 pwrite
    ssize_t n = ::pread(fd_, buf_, sizeof(buf_)-1, 0);
    if (n >= 0)
    {
      if (size)
      {
        *size = static_cast<int>(n);
      }
      buf_[n] = '\0';
    }
    else
    {
      err = errno;
    }
  }
  return err;
}

// 偏特化 函数模板 string
template int FileUtil::readFile(StringArg filename,
                                int maxSize,
                                string* content,
                                int64_t*, int64_t*, int64_t*);

// 偏特化 string 类型
template int FileUtil::ReadSmallFile::readToString(
    int maxSize,
    string* content,
    int64_t*, int64_t*, int64_t*);

