#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
using namespace std;

class MySQL
{
public:
    // 初始化数据库连接，这个只是开辟空间不是那种输入用户密码的连接
    MySQL();
    // 释放数据库连接资源
    ~MySQL();
    // 连接数据库，这才是
    bool connect();
    // 更新操作
    bool update(string sql);
    // 查询操作
    MYSQL_RES *query(string sql);
    // 获取连接
    MYSQL *getConnection();

private:
    MYSQL *_conn;
};

#endif