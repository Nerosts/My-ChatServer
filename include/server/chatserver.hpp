// #ifndef CHATSERVER_HPP
// #define CHATSERVER_HPP

// #endif // CHATSERVER_HPP

#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

// 聊天服务器的主类
class ChatServer
{
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr, // 存ip和port的
               const string &nameArg);        // 主机名

    // 启动服务
    void Start();

private:
    // 上报链接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &);
    // 上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,               // 缓冲区
                   Timestamp time);              // 接收到数据的时间信息

    TcpServer _server; // 组合的muduo库，实现服务器功能的类对象
    EventLoop *_loop;  // 有这个是为了能够在合适处中断循环，使用quit()
};
#endif