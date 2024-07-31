#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h> //打印日志
using namespace muduo;
ChatService *ChatService::instance()
{
    static ChatService service; // 静态变量只初始化一次
    return &service;
}

ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    // 登出业务
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});

    // 连接Redis服务器
    if (_redis.connect())
    {
        // 连接成功，就把我们的回调函数发过去。我们知道subscribe收到消息要干什么，就是不知道什么时候发生
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}
MsgHandler ChatService::getHandler(int msgid)
{
    // 这里还能打印错误日志，就是id没有对应的方法
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    { // 使用lambda表达式，这样即便没有相应的处理，程序也不会挂掉
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << msgid << "can't find it's handler funtion";
            // 千万不要endl，人家没有重载
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}
void ChatService::reset()
{
    // 把所有id的state变为Offline
    _userModel.resetState();
}

// 处理登录业务 就使用id和pwd了
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    //  1.拿到id和pwd
    int id = js["id"]; //.get<int>();
    string pwd = js["password"];
    // 2.得到该id对应的User对象，先比较用户传来的id\pwd和数据库内的一样吗
    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd) // 先判断用户存在否
    {
        if (user.getState() == "online")
        {
            // 重复登录
            json responce;
            responce["msgid"] = LOGIN_MSG_ACK;
            responce["errno"] = 2;
            responce["errmsg"] = "用户已经登录";
            conn->send(responce.dump());
        }
        else
        {
            // 登录成功后，要记录用户的连接信息
            //_connMutex.lock();
            //_userConnMap.insert({id, conn});
            //_connMutex.unlock();
            {
                lock_guard<mutex> lock(_connMutex); // 出了作用域就析构了，相当于智能指针了
                _userConnMap.insert({id, conn});
            } // 至于多个线程对一个数据库进行增删改查的并发问题，数据库进行处理

            // 登录成功后Redis，要订阅channel——userid
            _redis.subscribe(id); // 对这个用户感兴趣

            json responce;
            // 登录成功后，①要改变用户状态
            user.setState("online");
            _userModel.updateState(user);

            // 登录后②把离线消息拼到js里，给用户
            vector<string> offmsg = _offlineMsgModel.query(id);
            if (!offmsg.empty())
            {
                // 读取后，就删掉
                responce["offlinemsg"] = offmsg;
                _offlineMsgModel.remove(id);
            }

            // 登陆后③把好友信息返回
            vector<User> friends = _friendModel.query(id);
            if (!friends.empty())
            {
                vector<string> f; // 不能直接vector里放User整体放进json里。就先一个User信息变成string
                for (auto user : friends)
                {
                    // 一个一个的User对象
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();

                    f.push_back(js.dump()); // 每一个对象信息放进js串后转string后push进vector
                }
                responce["friends"] = f;
            }

            // 登陆后④把群组信息返回
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV; // 这个是存User的
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump()); // 把group类里的第四个变量处理好了
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump()); // 然后json串导进vector<string>
                }
                responce["groups"] = groupV;
            }

            //  给客户端返回信息——结果
            responce["msgid"] = LOGIN_MSG_ACK;
            responce["errno"] = 0;
            responce["id"] = user.getId();
            responce["name"] = user.getName();
            conn->send(responce.dump()); // 把返回的消息发回去
        }
    }
    else
    {
        // 登录失败
        json responce;
        responce["msgid"] = LOGIN_MSG_ACK;
        responce["errno"] = 1;
        responce["errmsg"] = "用户不存在或者pwd错误";
        conn->send(responce.dump()); // 把返回的消息发回去
    }
}

// 处理注册业务 用户四个字段id数据库给，state默认offline
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 注册用户就是把用户插入到表中，要使用UserModel进行Insert，就需要一个User对象--》需要name和password--》从json里面拿
    LOG_INFO << "do reg service";
    // 1.先得到用户名和密码
    string name = js["name"];
    string pwd = js["password"];
    // 2.创建user对象
    User user;
    user.setName(name);
    user.setPwd(pwd);
    // 3.使用usermodel进行添加
    bool res = _userModel.insert(user);
    // 4.成功或失败给客户端返回JSON信息
    if (res == true)
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0; // 0代表正常，1代表出错
        response["id"] = user.getId();
        conn->send(response.dump()); // dump()函数将JSON对象序列化为字符串，使用TcpConnectionPtr的send
    }
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
}
// 处理注册业务  name  password
// void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
// {
//     string name = js["name"];
//     string pwd = js["password"];

//     User user;
//     user.setName(name);
//     user.setPwd(pwd);
//     bool state = _userModel.insert(user);
//     if (state)
//     {
//         // 注册成功
//         json response;
//         response["msgid"] = REG_MSG_ACK;
//         response["errno"] = 0;
//         response["id"] = user.getId();
//         conn->send(response.dump());
//     }
//     else
//     {
//         // 注册失败
//         json response;
//         response["msgid"] = REG_MSG_ACK;
//         response["errno"] = 1;
//         conn->send(response.dump());
//     }
// 处理一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 一上来就先获取到to，给谁的
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        unordered_map<int, TcpConnectionPtr>::iterator it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // 找到了说明在线，就直接进行消息转发，说明都在同一个服务器上面登录了
            it->second->send(js.dump()); // 直接根据id得到连接的
            return;
        }
        // 都在一个作用域里面，防止一个正在发突然另外一个下线了
    }
    User user = _userModel.query(toid);
    if (user.getState() == "offline")
    {
        // 这里就是不在线，要存储离线消息
        _offlineMsgModel.insert(toid, js.dump()); // json串转string
    }
    else
    {
        // 在线，但是在其他服务器上面登录了，就pulish
        _redis.publish(toid, js.dump());
    }
}
// 添加好友业务 需要 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    // 得到两个信息后，利用表操作对象调用函数
    _friendModel.insert(userid, friendid);
}
// 创建群组业务 需要用户id，组name ，组desc
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];
    // 从json得到字段后，创建一个Group
    Group group(-1, name, desc);        // 先初始化一个组，因为id是数据库给，我们随便填一个
    if (_groupModel.createGroup(group)) // 因为传引用，就先自己初始化一个
    {
        // 创建成功后，把创建者信息添加
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
// 加入群组业务 用户id 群id
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    _groupModel.addGroup(userid, groupid, "normal");
    // 可以选择发响应
}
// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    // 拿到用户id和组id。查询在这个组内的用户id后，转发消息
    vector<int> groupmemb = _groupModel.queryGroupUsers(userid, groupid); // userid排除自己
    lock_guard<mutex> lock(_connMutex);
    for (auto &id : groupmemb)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 说明在线就直接发
            it->second->send(js.dump()); // 直接转发消息
            return;
        }
        else
        {
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                // 在其他服务器上面
                _redis.publish(id, js.dump());
                return;
            }

            else
            { // 到这里说明是离线
                _offlineMsgModel.insert(id, js.dump());
                return;
            }
        }
    }
}
// 处理注销、登出业务  对方的json串有id和msgid
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            // 说明找到了
            _userConnMap.erase(it);
        }
    }
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
    // Rsdis中取消subscribe
    _redis.unsubscribe(userid);
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn) // 我们传过来一个连接
{
    // 两个任务：改变user的状态 onlion-》offline   ； 把该id与conn 的映射删掉
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        unordered_map<int, TcpConnectionPtr>::iterator it = _userConnMap.begin();
        for (; it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                user.setId(it->first); // 这样就能得到id了，因为我们传conn过来，二者能对应

                _userConnMap.erase(it); // 把这个删掉
                break;
            }
        }
    }
    // Rsdis中取消subscribe
    _redis.unsubscribe(user.getId());

    // Task two
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}
// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    // 现在就是另一个服务器已经拿到了，就直接给用户就好
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        // 说明在线，直接发
        it->second->send(msg);
    }
    else
    {
        // 不在线，就插入到离线信息表中
        _offlineMsgModel.insert(userid, msg);
    }
}