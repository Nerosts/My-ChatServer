// 这里都进行函数的定义
#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp" //服务模块
#include <functional>
#include <string>
#include <iostream>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr, // 存ip和port的
                       const string &nameArg)         // 主机名
    : _server(loop, listenAddr, nameArg),
      _loop(loop)
{
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::Start()
{
    _server.start();
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        // 连接断了，关闭之前先进行退出登录的操作
        ChatService::instance()->clientCloseException(conn); // telnet测试用Ctrl+] 然后quit
        conn->shutdown();
    }
}
// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn, // 连接
                           Buffer *buffer,               // 缓冲区
                           Timestamp time)               // 接收到数据的时间信息
{
    string message = buffer->retrieveAllAsString();
    cout << message << endl; // 这里打印了原汁原味的字符串

    json js = json::parse(message); // 得到json串，接下来开始根据种类回调
                                    // 达到的目的：完全解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"] 获取=》业务handler=》conn  js  time
    MsgHandler msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>()); // 要把js类型转为int才行
    msgHandler(conn, js, time);                                                          // 这就是这个id对应的处理方法，把参数传过去就好了
}