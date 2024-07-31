#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono> //时间和时间段的类和功能
#include <ctime>
#include <iomanip>
#include <unordered_map>
#include <functional>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList; // 三个全局变量

// 有一个接收线程：要是发送和接受都一个线程，我们发送的时候在键盘输入时阻塞的，那也不能接收了
void readTaskHandler(int clientfd);
// 获取系统时间
string getCurrentTime();
// 显示聊天主页面
void mainMenu(int);
// 显示当前登录成功用户的基本信息
void showCurrentUserData();
// 控制是否进行聊天页面的循环
bool isMainMenuRunning = false;

// main函数作为发送线程，创建一个子线程作为接收
// 期望的运行方式 ./ChatClient 127.0.0.1 6666
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "运行命令错误，示例：./ChatClient 127.0.0.1 8000" << endl;
        exit(-1);
    }
    // 得到命令行传来的ip与port
    string ip = argv[1];
    uint16_t port = stoi(argv[2]);

    // 创建socket，开始通信
    int clientfd = socket(AF_INET, SOCK_STREAM, 0); // IPv4+TCP
    if (clientfd == -1)
    {
        cerr << "client's socket create error" << endl;
        exit(-1);
    }
    // 创建套接字结构体，进行服务器的（目标的）ip和port的填充
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);                  // 把本机字节序转网络字节序
    server.sin_addr.s_addr = inet_addr(ip.c_str()); // 或者使用
    // inet_pton(AF_INET, ip.c_str(), &server.sin_addr.s_addr);

    socklen_t len = sizeof(server);
    // 客户端只需要调用`connect()`函数来发起与服务器的连接请求即可
    int n = connect(clientfd, (sockaddr *)&server, len);
    if (n < 0)
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    //  main线程用于接收用户输入，负责发送数据
    while (true)
    {
        // 菜单
        cout << endl;
        cout << string(30, '=') << endl;
        cout << setw(10) << left << "Menu:" << endl;
        cout << string(30, '-') << endl;
        cout << setw(2) << left << "1." << " " << "Login" << endl;
        cout << setw(2) << left << "2." << " " << "Register" << endl;
        cout << setw(2) << left << "3." << " " << "Quit" << endl;
        cout << string(30, '=') << endl;
        cout << "Choice: ";

        int choice = 0;
        cin >> choice;
        cin.get(); // 拿走缓冲区的回车
        // 开始处理
        switch (choice)
        {
        case 1: // Login登录
        {
            // 登录需要输入id与pwd
            int id = -1;
            char pwd[64] = {0};
            cout << "user-id:";
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车
            cout << "user-password:";
            cin.getline(pwd, 64);
            // 开始组装json字符串
            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump(); // 序列化为字符串发过去

            ssize_t n = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (n < 0)
            {
                cerr << "client sent login msg to server error" << endl;
            }
            else
            {
                // 登录成功
                char buffer[1024] = {0};
                n = recv(clientfd, buffer, sizeof(buffer), 0);
                if (n < 0)
                {
                    cerr << "recv log msg error:" << request << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);   // 反序列化得到json字符串
                    if (0 != responsejs["errno"].get<int>()) // 登录失败
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else
                    {
                        // 登录成功
                        // 记录当前用户的id和name
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        // 记录好友列表信息
                        if (responsejs.contains("friends"))
                        {
                            g_currentUserFriendList.clear(); // 每次进到重新加入
                            // 先直接取出来
                            vector<string> vec = responsejs["friends"];
                            for (auto &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }

                        // 记录当前用户的群组列表信息
                        if (responsejs.contains("groups"))
                        {
                            g_currentUserGroupList.clear();
                            // 先直接取出来
                            vector<string> vec1 = responsejs["groups"];
                            for (auto &groupstr : vec1)
                            {
                                json grpjs = json::parse(groupstr);
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);
                                // 前三个弄完，现在弄第四个变量
                                vector<string> vec2 = grpjs["users"];
                                for (auto &userstr : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    // 用户信息填好了，就把这个用户push进vector里了
                                    group.getUsers().push_back(user);
                                } // 循环结束，组员信息也都存进vector里了，这里初始化好了
                                g_currentUserGroupList.push_back(group);
                            }
                        }
                        // 显示登录用户的基本信息
                        showCurrentUserData();

                        // 显示当前用户的离线消息：也分为个人聊天信息或者群组消息
                        cout << "这里是离线信息：" << endl;
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str); // 反序列化，就是一个一个json串
                                int msgtype = js["msgid"].get<int>();
                                // time + [id] + name + " said: " + xxx
                                if (ONE_CHAT_MSG == msgtype)
                                {
                                    // 一对一聊天
                                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                                         << " said: " << js["msg"].get<string>() << endl;
                                }
                                else
                                {
                                    // 说明是组聊天信息
                                    cout << "这是群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                                         << " said: " << js["msg"].get<string>() << endl;
                                }
                            }
                        }
                        static int once = 0;
                        if (once == 0)
                        {
                            // 登录成功，就开始启动一个新线程来进行接收消息了。一直就只有一个
                            std::thread readTask(readTaskHandler, clientfd); // 新线程执行的函数与其需要的参数
                            readTask.detach();                               // 进行线程分离
                            once++;
                        }

                        // 进入聊天主页面
                        isMainMenuRunning = true;
                        mainMenu(clientfd);
                    }
                }
            }
        }
        break;
        case 2: // register业务
        {
            // 注册使用用户名和密码，服务器给我们id
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "user-name:";
            cin.getline(name, 50);
            cout << "user-password:";
            cin.getline(pwd, 50);
            // 开始组装json字符串
            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump(); // 序列化为字符串准备发过去

            ssize_t n = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);

            if (n < 0)
            {
                cerr << "client sent reg msg to server error" << endl;
            }
            else
            {
                // 发送过去了，准备接收响应
                char buffer[1024] = {0};
                n = recv(clientfd, buffer, sizeof(buffer), 0);

                if (n < 0)
                {
                    cerr << "client recv reg msg from server error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer); // 反序列化得到json字符串

                    if (0 != responsejs["errno"]) // 注册失败
                    {
                        cerr << "name is already exist, register error!" << endl;
                    }
                    else // 注册成功
                    {
                        cout << "name register success, userid is " << responsejs["id"]
                             << ", do not forget it!" << endl;
                    }
                }
            }
        }
        break;
        case 3: // quit业务
        {
            close(clientfd);
            exit(0);
        }
        break;
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }
    return 0;
}
//---------------------------------------------------main--------------------------------------
// 这是接收线程哦
void readTaskHandler(int clientfd)
{
    cout << "进入了子线程" << endl;
    // 这里接收服务器发来的数据
    while (true)
    {
        cout << "这里子线程在循环读取" << endl;
        char buffer[1024];
        ssize_t n = recv(clientfd, buffer, sizeof(buffer), 0); // 会阻塞在这里，不会下去到下一次判断
        if (n <= 0)
        {
            // 说明读取出错，或者啥都没收到
            close(clientfd);
            exit(-1);
        }
        // 这里说明读到了string
        json js = json::parse(buffer); // 进行反序列化得到json串
        int msgtype = js["msgid"].get<int>();
        if (msgtype == ONE_CHAT_MSG) // 如果是一对一聊天
        {
            // 说明是聊天信息
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgtype == GROUP_CHAT_MSG)
        {
            // 说明是组聊天信息
            cout << "这是群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
    }
}

void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}

// "help" command handler
void help(int fd = 0, string str = "");
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "loginout" command handler
void loginout(int, string);
// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = { // 打印出来给用户看的
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}};

// 注册系统支持的客户端命令处理
// 大体思路是：截取用户输入的：之前的来进行映射，后面的就是参数
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}}; // 这些都是返回空，参数一个int，一个string

void mainMenu(int clientfd)
{
    help(); // 直接上了就到打印命令

    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer); // 这个是整个命令
        string command;            // 这个是存前段命令
        int pos = commandbuf.find(":");
        if (pos == string::npos) // 说明没找到
        {
            command = commandbuf; // 就是help或者loginout
            // 这里不用关心如果直接输入了不规范的内容，下面有方法处理
        }
        else
        {
            command = commandbuf.substr(0, pos);
        }

        auto it = commandHandlerMap.find(command); // 在处理表里找command
        if (it == commandHandlerMap.end())
        {
            cerr << "输入了错误的指令" << endl;
            continue;
        }
        // 到了这里就要调用函数处理了
        it->second(clientfd, commandbuf.substr(pos + 1, commandbuf.size() - pos));
    }
}

// "help"
void help(int fd, string str) // 函数定义，无需默认值
{
    cout << "command list : " << endl;
    // 这里我们就遍历commandMap，打印一下
    for (auto &e : commandMap)
    {
        cout << e.first << " --> " << e.second << endl;
    }
    cout << endl;
}
// "chat" chat:friendid:message
void chat(int clientfd, string str)
{
    int pos = str.find(":"); // friendid:message
    if (string::npos == pos) // 没有找到
    {
        cerr << "chat command invalid!(聊天命令错误)" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, pos).c_str());
    string message = str.substr(pos + 1, str.size() - pos); // 从str里面得到需要的

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime(); // 组装好json串
    string buffer = js.dump();

    ssize_t n = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (n < 0)
    {
        cerr << "send chat msg error ->(聊天发送错误) " << buffer << endl;
    }
}
// "addfriend" 格式addfriend:friendid
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    ssize_t n = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (n < 0)
    {
        cerr << "send addfriend msg error(添加好友错误) -> " << buffer << endl;
    }
}
// "creategroup" creategroup:groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int pos = str.find(":"); // groupname:groupdesc
    if (string::npos == pos) // 没有找到
    {
        cerr << "creategroup command invalid!(创建组命令错误)" << endl;
        return;
    }
    string groupname = str.substr(0, pos);
    string groupdesc = str.substr(pos + 1, str.size() - pos); // 从str里面得到需要的

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    ssize_t n = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (n < 0)
    {
        cerr << "send creategroup msg error(创建组信息错误) -> " << buffer << endl;
    }
}
// "addgroup" addgroup:groupid
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    int userid = g_currentUser.getId();

    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = userid;
    js["groupid"] = groupid;
    string buffer = js.dump();
    ssize_t n = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (n < 0)
    {
        cerr << "send addgroup msg error(加入组信息错误) -> " << buffer << endl;
    }
}
// "groupchat" groupchat:groupid:message
void groupchat(int clientfd, string str)
{
    int pos = str.find(":");
    if (pos == string::npos)
    {
        // 说明没找到
        cerr << "groupchat command invalid!(组聊天命令错误)" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, pos).c_str());
    string message = str.substr(pos + 1, str.size() - pos);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    ssize_t n = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (n < 0)
    {
        cerr << "send groupchat msg error(组聊信息错误) -> " << buffer << endl;
    }
}
// "loginout" loginout
void loginout(int clientfd, string)
{
    // 直接就组合json串
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    ssize_t n = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (n < 0)
    {
        cerr << "send loginout msg error(注销信息错误) -> " << buffer << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}