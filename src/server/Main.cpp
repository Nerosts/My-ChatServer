#include "chatserver.hpp"
#include "chatservice.hpp"
#include <signal.h>
#include <iostream>
using namespace std;
using namespace muduo;
using namespace muduo::net;

// 处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0); // 处理后直接结束程序
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "运行命令错误，示例：./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }
    // 得到命令行传来的ip与port
    string ip = argv[1];
    uint16_t port = stoi(argv[2]);

    signal(SIGINT, &resetHandler);

    EventLoop loop;
    InetAddress add(ip, port);
    ChatServer server(&loop, add, "mychat");

    server.Start();
    loop.loop(); // 相当于循环调用epoll_wait
    return 0;
}