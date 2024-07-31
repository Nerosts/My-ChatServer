#include "groupmodel.hpp"
#include "db.h"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 还是先组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup (groupname, groupdesc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());
    // 创建数据库，连接执行语句
    MySQL mysql;
    if (mysql.connect())
    {
        // 连接成功
        if (mysql.update(sql)) // 开始执行
        {
            group.setId(mysql_insert_id(mysql.getConnection())); // 得到数据库自动生成的id
            return true;
        }
        // 失败就是上sql语句执行失败
    }
    return false;
}
// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    // 还是先组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser values(%d, %d, '%s')",
            groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
// 查询用户所在群组信息，Group对象前三个是组表信息，最后一个是用户信息
vector<Group> GroupModel::queryGroups(int userid)
{
    // 还是先组装sql语句
    char sql[1024] = {0};
    // 1. 先根据userid在groupuser表中查询出该用户所属的群组信息
    // 2. 再根据群组信息，查询属于该群组的所有用户的userid，
    // 3. 并且和user表进行多表联合查询，查出用户的详细信息
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from AllGroup a inner join \
         GroupUser b on a.id = b.groupid where b.userid=%d",
            userid);

    vector<Group> groups;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 说明有群组信息
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                // 一直循环读多行
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);

                groups.push_back(group);
                // 到这里还只是把三个基本属性添加完了，还有个 vector<GroupUser> users;
            }
            mysql_free_result(res);
        }
    }
    for (auto &group : groups)
    {
        // 一个组的前三个信息都好了，开始填充user的信息
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from User a \
            inner join GroupUser b on b.userid = a.id where b.groupid=%d",
                group.getId()); // 利用这个组的信息，找的这个组的组员的id，然后就能得到其他信息

        MySQL mysql;
        if (mysql.connect())
        {
            MYSQL_RES *res = mysql.query(sql);
            if (res != nullptr)
            {
                // 说明有
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr)
                {
                    GroupUser gu;
                    gu.setId(atoi(row[0]));
                    gu.setName(row[1]);
                    gu.setState(row[2]);
                    gu.setRole(row[3]);

                    group.getUsers().push_back(gu);
                }
                mysql_free_result(res);
            }
        }
    }
    return groups;
}
// 根据指定的groupid查询群组用户id列表，除userid自己：主要用户群聊业务给群组其它成员群发消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from GroupUser where groupid = %d and userid != %d", groupid, userid);

    vector<int> userids;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                int userid = atoi(row[0]);

                userids.push_back(userid);
            }
            mysql_free_result(res);
        }
    }
    return userids;
}