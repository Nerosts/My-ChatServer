#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <string>
#include <functional>

using namespace std;

class ChatServer
{
public:
    ChatServer(muduo::net::EventLoop *loop,
               const muduo::net::InetAddress &listenAddr,
               const string &nameArg)
        : _server(loop, listenAddr, nameArg), _loop(loop) // 初始化完成
    {
        // 此时我知道如果有用户进行连接后应该怎么做，但是我不知道何时会发生。这种情况就使用回调函数

        // 给服务器注册用户连接的创建和断开回调
        // typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
        _server.setConnectionCallback(bind(&ChatServer::onConnection, this, placeholders::_1)); // 把我们自己的函数发过去
        // 给服务器注册用户读写事件回调
        // typedef std::function<void(const TcpConnectionPtr &,
        //                            Buffer *,
        //                            Timestamp)>
        //     MessageCallback;
        _server.setMessageCallback(bind(&ChatServer::onMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));

        // 设置服务器端的线程数量 1个I/O线程   3个worker线程
        _server.setThreadNum(4);
    }
    // 回调函数是一种特殊的函数，它作为参数传递给另一个函数（通常是某个库函数或API），并在某个特定事件发生时由后者调用
    void Start()
    {
        _server.start();
    }

    ~ChatServer()
    {
        _loop->quit(); // 停止事件循环
    }

private:
    // 专门处理用户的连接创建和断开
    void onConnection(const muduo::net::TcpConnectionPtr &con)
    {
        if (con->connected())
        {
            cout << con->peerAddress().toIpPort() << " -> " << con->localAddress().toIpPort() << " state:online" << endl;
        }
        else
        {
            cout << con->peerAddress().toIpPort() << " -> " << con->localAddress().toIpPort() << " state:offline" << endl;
            con->shutdown(); // 就相当于close(fd)
        }
    }
    // 专门处理用户的读写事件
    void onMessage(const muduo::net::TcpConnectionPtr &con,
                   muduo::net::Buffer *buffer, // 缓冲区，其中包含了从 TCP 连接中读取到的数据
                   muduo::Timestamp time)      // 接收到数据的时间信息
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << " time:" << time.toFormattedString() << endl;
        con->send(buf);
    }
    muduo::net::TcpServer _server; // TcpServer对象
    muduo::net::EventLoop *_loop;
};

int main()
{
    muduo::net::EventLoop loop;
    // InetAddress(StringArg ip, uint16_t port, bool ipv6 = false);
    muduo::net::InetAddress listenAddr("127.0.0.1", 8888);
    ChatServer server(&loop, listenAddr, "MyCharRoom");
    server.Start(); // 启动服务
    loop.loop();    // 相当于epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等
    return 0;
}