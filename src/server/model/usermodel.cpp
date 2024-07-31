#include "usermodel.hpp"
#include "db.h" //这里就要进行数据库操作了
#include <iostream>
// user表的添加
bool UserModel::insert(User &user)
{
    // 1.组装SQL语句，过会要直接传入的
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    // 2.创建MySQL，开始对表进行操作
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        if (mysql.update(sql)) // 更新成功
        {
            // 开始设置id，能返回回去
            // 获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
// 表的更新
bool UserModel::updateState(User user)
{
    // 1.组装SQL语句，过会要直接传入的
    char sql[1024] = {0};
    sprintf(sql, "update User set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    // 2.创建MySQL，开始对表进行操作
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        if (mysql.update(sql)) // 更新成功
        {
            return true;
        }
    }
    return false;
}

User UserModel::query(int id) // 根据id得到对象的拷贝
{
    // 1.组装SQL语句，过会要直接传入的
    char sql[1024] = {0};
    sprintf(sql, "select * from User where id=%d", id);

    // 2.创建MySQL，开始对表进行操作
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        MYSQL_RES *res = mysql.query(sql); // 数据库中执行查询后返回的结果集
        if (res != nullptr)
        {
            // 现在有了结果集，肯定就是一行
            // mysql_fetch_row当前行的数据以数组形式返回
            MYSQL_ROW row = mysql_fetch_row(res); // MYSQL_ROW本身就是指针
            if (row != nullptr)
            {
                User user;                // 创造一个对象
                user.setId(atoi(row[0])); // 每个元素是一个指向字符串的指针
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);

                mysql_free_result(res); // 处理指针对象
                return user;
            }
        }
    }
    return User(); // 返回临时对象，不用担心
                   // 找不到就返回一个默认的临时对象，id为-1
}

void UserModel::resetState()
{
    // 1.组装SQL语句，过会要直接传入的
    char sql[1024] = {0};
    sprintf(sql, "update User set state = 'offline' where state = 'online'");

    // 2.创建MySQL，开始对表进行操作
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        mysql.update(sql);
    }
}