// check 日志文件管理,使用 AppendFile 增加新的日志记录

// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include "muduo/base/Mutex.h"
#include "muduo/base/Types.h"

#include <memory>

namespace muduo
{

// class declare
namespace FileUtil
{
  // append 添加到文件末尾
class AppendFile;
}

// 日志文件  不可以复制
class LogFile : noncopyable
{
 public:
  LogFile(const string& basename,
          off_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 1024);
  ~LogFile();

  void append(const char* logline, int len);
  void flush();
  bool rollFile();

 private:
//  无锁条件下 append
  void append_unlocked(const char* logline, int len);
// 获得日志文件名称
  static string getLogFileName(const string& basename, time_t* now);

// data member
private:
  const string basename_;
  const off_t rollSize_;
  const int flushInterval_;
  const int checkEveryN_;     // 写入多少次检查

  int count_;

  // 唯一指针
  std::unique_ptr<MutexLock> mutex_;
  time_t startOfPeriod_;    // 日志roll 周期
  time_t lastRoll_;         // 最新 roll 时间
  time_t lastFlush_;        // 最新 刷新 时间
  // 唯一指针 appendFile
  std::unique_ptr<FileUtil::AppendFile> file_;

//   一天时间的秒数
  const static int kRollPerSeconds_ = 60*60*24;
};

}  // namespace muduo
#endif  // MUDUO_BASE_LOGFILE_H
