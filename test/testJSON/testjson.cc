#include "json.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

using json = nlohmann::json;

string test1()
{
    json js;
    js["int"] = 1;
    js["string"] = "abc";
    // 添加数组
    js["id"] = {1, 2, 3, 4, 5};
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china"; // 这个这样理解：js["msg"]是一个value，本身也是一个js。就能跟第二个放一起了
    return js.dump();
}

void test2()
{
    json js;
    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;
    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    js["path"] = m;
    cout << js << endl;
    cout << js.dump() << endl; // 直接使用函数打印结果跟JS是一样的，这个函数能方便我们JSON字符串
}
int main()
{
    string jsonbuffer = test1();       // 先得到返回值的JSON字符串
    json js = json::parse(jsonbuffer); // 再由字符串转为json

    cout << js["int"] << endl; // 直接单个

    auto js_msg = js["msg"];
    cout << js_msg["zhang san"] << endl;
    return 0;
}