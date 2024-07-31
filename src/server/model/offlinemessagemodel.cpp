#include "offlinemessagemodel.hpp"
#include "db.h"
// 存储用户的离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
    // 1.组装SQL语句，过会要直接传入的
    char sql[1024] = {0};
    sprintf(sql, "insert into OfflineMessage values('%d', '%s')", userid, msg.c_str());

    // 2.创建MySQL，开始对表进行操作
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        mysql.update(sql);
    }
}

// 删除用户的离线消息
void OfflineMsgModel::remove(int userid)
{
    // 1.组装SQL语句，过会要直接传入的
    char sql[1024] = {0};
    sprintf(sql, "delete from OfflineMessage where userid=%d", userid);

    // 2.创建MySQL，开始对表进行操作
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        mysql.update(sql);
    }
}

// 查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from OfflineMessage where userid = %d", userid);

    // 2.定义好返回值，开始填充。一般有多行就循环
    vector<string> offmsg;
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
                offmsg.push_back(row[0]); // 只有一个message的属性
            }
            mysql_free_result(res);
            return offmsg;
        }
    }
    return offmsg;
}