// 这里面就是服务端的代码了
// 一个消息id映射一个方法

#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "json.hpp"
using json = nlohmann::json;

#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"

#include "redis.hpp"

// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

class ChatService
{
public:
    static ChatService *instance(); // 获取那个单例的函数

    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注销、登出业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理客户端异常退出，这个不是由得到信息回调处理
    void clientCloseException(const TcpConnectionPtr &conn);

    // 服务器异常后，进行重置
    void reset();
    //  获取消息对应的处理器
    MsgHandler getHandler(int msgid); // 给人家chatserver根据msgid得到回调函数用的

    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);

private:
    ChatService(); // 使用单例模式
    // static ChatService _singal;

    // 存储消息msgid和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap; // ①方法的处理是初始化后就定了下来不会改变

    // 我们要保证用户与服务器的长连接，这样即便a给b发消息，a先给服务器，服务器会给b
    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap; // ②但是这里用户id与连接的映射关系是会随着用户上下线来变的
    // ③有可能多个用户同时登陆-》同时对map进行修改，这就不线程安全了

    // 定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex; // 我这里加锁，是为了在发送消息时其他线程里用户下线了，但是下线会引起_userConnMap改变
    // 而改变是在锁里面的

    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    // Redis操作对象
    Redis _redis;
};

#endif