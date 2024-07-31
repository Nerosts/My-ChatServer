#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

// 群组用户，多了一个role角色信息，从User类直接继承，复用User的其它信息
// 默认构造函数会自动调用父类构造函数
class GroupUser : public User
{
public:
    void setRole(string role) { _role = role; }
    string getRole() { return _role; }

private:
    string _role;
};

#endif