#include "friendmodel.hpp"
#include "db.h"
// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    // 1.组装SQL语句，过会要直接传入的
    char sql[1024] = {0};
    sprintf(sql, "insert into Friend values('%d', '%d')", userid, friendid);

    // 2.创建MySQL，开始对表进行操作
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        mysql.update(sql);
    }
}

// 返回用户好友列表
vector<User> FriendModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from User a inner join Friend b on b.friendid = a.id where b.userid=%d", userid);

    // 2.定义好返回值，开始填充。一般有多行就循环
    vector<User> friends;
    MySQL mysql;
    if (mysql.connect()) // 先连接数据库
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 说明有离线信息

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                // 一直循环读多行
                User user;
                user.setId(atoi(row[0])); // 每个元素是一个指向字符串的指针
                user.setName(row[1]);
                user.setState(row[2]);
                friends.push_back(user);
            }
            mysql_free_result(res);
            return friends;
        }
    }
    return friends;
}