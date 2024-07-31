#include "redis.hpp"
#include <iostream>

using namespace std;

Redis::Redis()
    : _publish_context(nullptr), _subcribe_context(nullptr)
{
}
Redis::~Redis()
{
    // 不为空就free一下
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if (_subcribe_context != nullptr)
    {
        redisFree(_subcribe_context);
    }
}

// 连接redis服务器
bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379); // 这里是与该服务器上的Redis服务进行连接，默认端口号都是6379
    if (nullptr == _publish_context)
    {
        cerr << "connect redis failed!（连接Redis失败）" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    _subcribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _subcribe_context)
    {
        cerr << "connect redis failed!（连接Redis失败）" << endl;
        return false;
    }

    // 单独开一个线程去subscribe订阅阻塞，为什么？
    // 因为如果主线程来subscribe，那就一直阻塞了，这个线程不能干其他的了
    // 所以我们选择单开一个线程来特地subscribe进行等待得到消息后向上汇报
    thread t([&]()
             { observer_channel_message(); }); //
    t.detach();                                // 线程分离

    cout << "connect redis-server success!" << endl;

    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message) // 通道号与消息
{
    redisReply *reply = nullptr;
    // 执行命令的函数
    reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (reply == nullptr)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    // 使用完后就能销毁啦
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    // 这里不能像上面publish一样直接执行redisCommand，使用Redis语句，该函数会等待语句的执行，所以依然阻塞
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!（订阅指令错误）" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "subscribe command failed!（订阅指令错误）" << endl;
            return false;
        }
    }
    // redisGetReply
    // 到这里就说明指令已经发过去了
    return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done) // done为0是在才进入
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->_subcribe_context, (void **)&reply)) // 从订阅里得到响应
    {
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 说明收到了符合规则的响应，那么就给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
    }
    freeReplyObject(reply);
}

// 初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn) // 传过来一个函数
{
    _notify_message_handler = fn;
}