#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"
using namespace std;
// user表的数据操作类
class UserModel
{
public:
    bool insert(User &user); // 这里传入&是为了能得到SQL自动添加后的id

    bool updateState(User user);

    User query(int id); // 根据用户id得到User对象

    void resetState(); // 把状态重置
};

#endif