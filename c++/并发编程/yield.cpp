#include <iostream>
#include <thread>
#include <chrono>

// 模拟一个任务，线程执行时会间歇性调用 yield 让出 CPU 时间
void taskWithYield(int id) {
    for (int i = 0; i < 5; ++i) {
        std::cout << "Task " << id << " is running, iteration " << i << std::endl;

        // 每次输出后让出 CPU 时间片，允许其他线程执行
        std::this_thread::yield();  // 主动让出 CPU 时间片，允许其他线程执行

        // 模拟任务的执行，稍微延迟一下
        std::this_thread::sleep_for(std::chrono::seconds(2));
        printf("sleep 2 seconds\n");
        std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::seconds(2));
    }
}

int main() {
    // 创建两个线程，模拟两个任务
    std::thread t1(taskWithYield, 1);
    std::thread t2(taskWithYield, 2);

    // 等待两个线程完成
    t1.join();
    t2.join();

    return 0;
}
