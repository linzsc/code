#include <iostream>
#include <future>
#include <chrono>
#include <thread>

// 定义一个异步任务
std::string fetchDataFromDB(std::string query) {
    // 模拟一个异步任务，比如从数据库中获取数据
    std::this_thread::sleep_for(std::chrono::seconds(6));
    //std::this_thread::sleep_for(std::chrono::seconds(5));
    return "Data: " + query;
}

//实现过一秒输出一次
void printData() {
    for (int i = 0; i < 5;i++) {
        std::cout << "Data: " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
int main() {
    // 使用 std::async 异步调用 fetchDataFromDB
    std::future<std::string> resultFromDB = std::async(std::launch::async, fetchDataFromDB, "Data");


    
    // 在主线程中做其他事情
    std::cout << "Doing something else..." << std::endl;


    //异步调用printData函数
    std::thread t(printData);
    // 从 future 对象中获取数据
    std::string dbData = resultFromDB.get();
    std::cout << dbData << std::endl;

    return 0;
}
