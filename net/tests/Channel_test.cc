// check ； 通道管理

#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"

#include <functional>
#include <map>

#include <stdio.h>
#include <unistd.h>
#include <sys/timerfd.h>

using namespace muduo;
using namespace muduo::net;

void print(const char* msg)
{
  static std::map<const char*, Timestamp> lasts;
  Timestamp& last = lasts[msg];
  Timestamp now = Timestamp::now();
  printf("%s tid %d %s delay %f\n", now.toString().c_str(), CurrentThread::tid(),
         msg, timeDifference(now, last));
  last = now;
}

namespace muduo
{
namespace net
{
namespace detail
{
int createTimerfd();
void readTimerfd(int timerfd, Timestamp now);
}
}
}

// 周期性的时间器
// Use relative time, immunized to wall clock changes.
// typedef std::function<void()> TimerCallback;                  // 时间器 回调函数
// std::bind(print,"xxx") -> std::function<void()>
class PeriodicTimer
{
 public:
  PeriodicTimer(EventLoop* loop, double interval, const TimerCallback& cb)
    : loop_(loop),
      timerfd_(muduo::net::detail::createTimerfd()),    // 创建一个 时间fd
      timerfdChannel_(loop, timerfd_), // 创建时间fd 的 channel
      interval_(interval),    // 获得时间器间隔
      cb_(cb)  // 回调函数
  {
    // channel设置读取回调函数
    timerfdChannel_.setReadCallback(std::bind(&PeriodicTimer::handleRead, this));
    // 开启监听
    timerfdChannel_.enableReading();
  }

  // 开启周期计时器，为 时间fd 设置时间间隔
  void start()
  {
    // 初始化 itimerspec
    struct itimerspec spec;
    memZero(&spec, sizeof spec);
    spec.it_interval = toTimeSpec(interval_);
    spec.it_value = spec.it_interval;

    // 从 it_value 开始，每过 it_interval 秒执行一次

    // 为 时间fd 设置间隔
    int ret = ::timerfd_settime(timerfd_, 0 /* relative timer */, &spec, NULL);
    if (ret)
    {
      LOG_SYSERR << "timerfd_settime()";
    }
  }

  ~PeriodicTimer()
  {
    // channel 的清理过程
    timerfdChannel_.disableAll();
    // 调用channel绑定的loop去删除该channel
    timerfdChannel_.remove();
    // 关闭 时间fd
    ::close(timerfd_);
  }

 private:
  // 通道读取事件 回调函数
  void handleRead()
  {
    loop_->assertInLoopThread();
    muduo::net::detail::readTimerfd(timerfd_, Timestamp::now());
    // 执行回调函数
    if (cb_)
      cb_();
  }

  // seconds ; from doubles to timespec
  static struct timespec toTimeSpec(double seconds)
  {
    // timespec.tv_sec 秒; timespec.tv_nsec 毫秒;
    struct timespec ts;
    memZero(&ts, sizeof ts);
    const int64_t kNanoSecondsPerSecond = 1000000000;
    const int kMinInterval = 100000;    // 最小的间隔
    int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);
    if (nanoseconds < kMinInterval)
      nanoseconds = kMinInterval;
    ts.tv_sec = static_cast<time_t>(nanoseconds / kNanoSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(nanoseconds % kNanoSecondsPerSecond);
    return ts;
  }

  EventLoop* loop_;         // 关联的 loop
  const int timerfd_;       // 时间fd
  Channel timerfdChannel_;  // 时间fd的channel
  const double interval_;   // 时间间隔 in seconds
  TimerCallback cb_;        // 回调函数
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid()
           << " Try adjusting the wall clock, see what happens.";
  EventLoop loop;
  // print 作为定时器回调函数
  PeriodicTimer timer(&loop, 1, std::bind(print, "PeriodicTimer"));
  timer.start(); // 为 时间fd 设置时间
  loop.runEvery(1, std::bind(print, "EventLoop::runEvery"));
  loop.loop();
}
