#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接redis服务器
    bool connect();

    // 向redis指定的通道channel发布消息
    bool publish(int channel, string message);

    // 向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);

    // 向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);

    // 在独立线程中接收订阅通道中的消息
    void observer_channel_message();

    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int, string)> fn); // 函数本体

private:
    // hiredis同步上下文对象，包括连接的状态、套接字文件描述符、缓冲区、错误信息等
    redisContext *_publish_context; // 负责publish消息
    // 我们可以简单理解为进行连接Redis
    redisContext *_subcribe_context; // 负责subscribe消息
    // 需要两个是因为当其中一个在subscribe时，是阻塞的

    // 回调操作，一旦收到订阅的消息，给service层上报
    function<void(int, string)> _notify_message_handler; // 函数的回调
    // 参数说明：int是订阅的编号也就是用户id，string就是发来的消息
};

#endif