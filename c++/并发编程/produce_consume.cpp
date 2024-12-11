#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

std::queue<int> dataQueue;           // 数据队列
std::mutex mtx;                      // 互斥锁保护队列
std::condition_variable cv;          // 条件变量
bool done = false;                   // 标记是否完成生产

void producer() {
    for (int i = 0; i < 10; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            dataQueue.push(i);  // 生产数据
            std::cout << "Produced: " << i << std::endl;
        }
        cv.notify_one();  // 通知消费者线程
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    {
        std::lock_guard<std::mutex> lock(mtx);
        done = true;
    }
    cv.notify_all();  // 通知所有等待的线程
}

void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !dataQueue.empty() ||done; });

        while (!dataQueue.empty()) {
            int data = dataQueue.front();
            dataQueue.pop();
            std::cout << "Consumed: " << data << std::endl;
        }

        if (done && dataQueue.empty())
            break;
    }
}

int main() {
    std::thread t1(producer);
    std::thread t2(consumer);

    t1.join();
    t2.join();
    std::cout<<"finished!"<<std::endl;
    return 0;
}
