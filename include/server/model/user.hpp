#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

// User表的ORM类(对象关系映射，表格通常会映射到与之对应的类)
// class User
// {
// public:
//     User(int id = -1, string name = "", string pwd = "", string state = "offline")
//     {
//         this->id = id;
//         this->name = name;
//         this->password = pwd;
//         this->state = state;
//     }

//     void setId(int id) { this->id = id; }
//     void setName(string name) { this->name = name; }
//     void setPwd(string pwd) { this->password = pwd; }
//     void setState(string state) { this->state = state; }

//     int getId() { return this->id; }
//     string getName() { return this->name; }
//     string getPwd() { return this->password; }
//     string getState() { return this->state; }

// protected:
//     int id;
//     string name;
//     string password;
//     string state;
// };

class User
{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
    {
        _id = id;
        _name = name;
        _password = pwd;
        _state = state;
    }
    // 提供一系列的get和set
    void setId(int id) { _id = id; }
    void setName(string name) { _name = name; }
    void setPwd(string pwd) { _password = pwd; }
    void setState(string state) { _state = state; }

    int getId() { return _id; }
    string getName() { return _name; }
    string getPwd() { return _password; }
    string getState() { return _state; }

protected:
    int _id;
    string _name;
    string _password;
    string _state; // 用户状态：在线，离线
};

#endif