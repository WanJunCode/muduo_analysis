// check 日志类，根据不同的 日志等级进行输出

// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H

#include "muduo/base/LogStream.h"
#include "muduo/base/Timestamp.h"

namespace muduo
{

class TimeZone;

class Logger
{
public:
  //  日志等级 枚举类型
  enum LogLevel
  {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  // compile time calculation of basename of source file
  // 运行时 计算得到 source 文件的文件名称
  // 用于从完整路径中 获得最后的文件名
  class SourceFile
  {
  public:
    //  使用模板类 完成动态数组的创建
    template <int N>
    SourceFile(const char (&arr)[N])
        : data_(arr),
          size_(N - 1)
    {
      // 找到最后一个 “/” 获得 文件名
      const char *slash = strrchr(data_, '/'); // builtin function
      if (slash)
      {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    explicit SourceFile(const char *filename)
        : data_(filename)
    {
      const char *slash = strrchr(filename, '/');
      if (slash)
      {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    const char *data_;
    int size_;
  }; //end SourceFile

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char *func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream &stream() { return impl_.stream_; }

  static LogLevel logLevel();
  // 设置日志的等级
  static void setLogLevel(LogLevel level);
  // 输出 处理函数
  typedef void (*OutputFunc)(const char *msg, int len);
  // 刷新处理函数
  typedef void (*FlushFunc)();
  // 设置 输出、刷新 处理函数
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);
  // 设置时区
  static void setTimeZone(const TimeZone &tz);

private:
  // Logger 实现类
  class Impl
  {
  public:
    typedef Logger::LogLevel LogLevel;
    Impl(LogLevel level, int old_errno, const SourceFile &file, int line);
    void formatTime();
    void finish();

    Timestamp time_;
    LogStream stream_;
    LogLevel level_;
    int line_;
    SourceFile basename_;
  };

  Impl impl_;
};

// 声明一个全局变量 日志等级
extern Logger::LogLevel g_logLevel;

// 获得日志等级
inline Logger::LogLevel Logger::logLevel()
{
  return g_logLevel;
}

//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//

// 使用以下的 宏定义 使用 log 模块
// trace 0 ， debug 1 ， info  2
#define LOG_TRACE                                        \
  if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                        \
  if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                        \
  if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
  muduo::Logger(__FILE__, __LINE__).stream()

#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()

// 将错误代码转化为 字符串
const char *strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.
// 用于检查输入不是 NULL， 对于初始化列表非常有用
#define CHECK_NOTNULL(val) \
  ::muduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// 一个检查 not null 的帮助器，检查指针 ptr 是否是空指针，如果是空指针则输出到 file
// A small helper for CHECK_NOTNULL().
template <typename T>
T *CheckNotNull(Logger::SourceFile file, int line, const char *names, T *ptr)
{
  if (ptr == NULL)
  {
    Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}

} // namespace muduo

#endif // MUDUO_BASE_LOGGING_H
