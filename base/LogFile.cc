// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/LogFile.h"

#include "muduo/base/FileUtil.h"
#include "muduo/base/ProcessInfo.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace muduo;

LogFile::LogFile(const string& basename,
                 off_t rollSize,
                 bool threadSafe,
                 int flushInterval,
                 int checkEveryN)
  : basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN),
    count_(0),
    mutex_(threadSafe ? new MutexLock : NULL),    // 线程安全的情况下需要 new MutexLock
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
{
  assert(basename.find('/') == string::npos);
  rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char* logline, int len)
{
  // 添加新的记录，有两种方式  加锁条件/无锁条件
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    append_unlocked(logline, len);
  }
  else
  {
    append_unlocked(logline, len);
  }
}

// 刷新
void LogFile::flush()
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    file_->flush();
  }
  else
  {
    file_->flush();
  }
}

// 无锁条件下添加新的日志
// 当写入数据长度足够 需要 roll
// 当写入间隔时间过长，需要刷新
// 当跨越了新的周期 需要刷新日志文件
void LogFile::append_unlocked(const char* logline, int len)
{
  file_->append(logline, len);

  // 如果 写入的数据长度 大于 rollSize 需要重新刷新文件
  if (file_->writtenBytes() > rollSize_)
  {
    rollFile();
  }
  else
  {
    ++count_;
    if (count_ >= checkEveryN_)
    {
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_)
      {
        rollFile();
      }
      else if (now - lastFlush_ > flushInterval_)
      {
        // 如果当前写入时间 与 上次写入时间的间隔大于 flushInterval_ ， 刷新输出
        lastFlush_ = now;
        file_->flush();
      }
    }
  }
}

// 重新设置 AppendFile ， 刷新相对应的时间
bool LogFile::rollFile()
{
  time_t now = 0;
  // 获得文件名
  string filename = getLogFileName(basename_, &now);
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

  // 当前时间  大于  上一次roll的时间
  if (now > lastRoll_)
  {
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    // unique_ptr  AppendFile
    file_.reset(new FileUtil::AppendFile(filename));
    return true;
  }
  return false;
}

// 获得 指定格式的 日志文件名
string LogFile::getLogFileName(const string& basename, time_t* now)
{
  string filename;
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  struct tm tm;
  *now = time(NULL);
  // now ==> tm
  gmtime_r(now, &tm); // FIXME: localtime_r ?
  // string format time
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
  // 日志文件后面添加日期时间
  filename += timebuf;

  filename += ProcessInfo::hostname();

  char pidbuf[32];
  snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
  filename += pidbuf;

  filename += ".log";

  return filename;
}

