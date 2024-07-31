#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;

class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
    {
        _id = id;
        _name = name;
        _desc = desc;
    }

    // 一系列的get和set方法
    void setId(int id) { _id = id; }
    void setName(string name) { _name = name; }
    void setDesc(string desc) { _desc = desc; }

    int getId() { return _id; }
    string getName() { return _name; }
    string getDesc() { return _desc; }
    vector<GroupUser> &getUsers() { return this->users; }

private:
    int _id;
    string _name;
    string _desc;            // 组的描述
    vector<GroupUser> users; // 这里存一个组的成员
    // 我们在查看一个组的成员信息时，不仅要看的成员的User信息，还要有一个组内的role信息
    // 所以选择GroupUser继承User
};

#endif