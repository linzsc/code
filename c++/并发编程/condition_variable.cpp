#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>

std::mutex mtx;                 // 用于保护共享资源的互斥锁
std::condition_variable cv;     // 条件变量，用于线程同步
bool ready = false;             // 共享资源，标记条件是否满足

void worker_thread() {
    // 创建 unique_lock，并自动上锁
    std::unique_lock<std::mutex> lock(mtx);
    std::cout<<"worker_thread is running before wait!"<<std::endl;
    // 当前线程等待条件变量的通知
    cv.wait(lock, [] { return ready; });
    std::cout<<"worker_thread is awaken"<<std::endl;
    // 被唤醒后执行
    std::cout << "Worker thread is proceeding.\n";
}

int main() {
    
    // 创建一个工作线程
    std::thread worker(worker_thread);

    // 主线程模拟一些操作
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 修改条件，并通知等待的线程
    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;
    }
    std::cout<<"cv is ready to wake thread!"<<std::endl;

    cv.notify_all();  // 通知一个等待的线程

    worker.join();
    return 0;
}
