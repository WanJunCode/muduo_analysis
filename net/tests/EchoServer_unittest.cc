// 回文 服务器

#include "muduo/net/TcpServer.h"

#include "muduo/base/Logging.h"
#include "muduo/base/Thread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

int numThreads = 0;

// 回文服务器
class EchoServer
{
 public:
  EchoServer(EventLoop* loop, const InetAddress& listenAddr)
    : loop_(loop),
      server_(loop, listenAddr, "EchoServer")
  {
    server_.setConnectionCallback(
        std::bind(&EchoServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&EchoServer::onMessage, this, _1, _2, _3));
    server_.setThreadNum(numThreads);
  }

  void start()
  {
    server_.start();
  }
  // void stop();

 private:
  // 只要有新的客户端连接，打印相关信息，发送 “hello” 给客户端
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
        << conn->localAddress().toIpPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");
    LOG_INFO << conn->getTcpInfoString();

    conn->send("hello\n");
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
  {
    // 获取tcp client的缓冲区中所有的数据
    string msg(buf->retrieveAllAsString());
    LOG_TRACE << conn->name() << " recv " << msg.size() << " bytes at " << time.toString();
    if (msg == "exit\n")
    {
      conn->send("bye\n");
      // 客户端关闭
      conn->shutdown();
    }
    if (msg == "quit\n")
    {
      // 服务器关闭
      loop_->quit();
    }
    // 回文
    conn->send(msg);
  }

  EventLoop* loop_;
  TcpServer server_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
  LOG_INFO << "sizeof TcpConnection = " << sizeof(TcpConnection);
  if (argc > 1)
  {
    numThreads = atoi(argv[1]);
  }
  bool ipv6 = argc > 2;
  EventLoop loop;
  InetAddress listenAddr(2000, false, ipv6);
  EchoServer server(&loop, listenAddr);

  server.start();

  loop.loop();
}

