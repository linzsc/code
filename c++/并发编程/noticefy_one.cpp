#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>

std::mutex mtx;
std::condition_variable cv;
std::queue<int> task_queue;  // 存放任务目标线程 ID
bool stop = false;

void thread_function(int id) {
    int count=0;
    while (true) {
        std::cout<<"id:"<<id<<" count:"<<count<<std::endl;
        std::unique_lock<std::mutex> lock(mtx);

        // 等待任务到达或线程池停止
        cv.wait(lock, [&] { return stop || (!task_queue.empty() && task_queue.front() == id); });
        std::cout<<"id "<<id<<" is awoken"<<std::endl;
        if (stop) break;  // 退出线程

        // 执行任务
        task_queue.pop();
        std::cout << "Thread " << id << " is executing its task.\n";
    }
    std::cout<<"id "<<id<<" is stopped"<<std::endl;
}

int main() {
    const int thread_count = 5;
    std::vector<std::thread> threads;

    // 启动多个线程
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(thread_function, i);
    }

    // 分配任务给特定线程
    {
        std::lock_guard<std::mutex> lock(mtx);
        task_queue.push(2);  // 将任务分配给线程 2
    }
    cv.notify_all();

    // 停止线程池
    {   
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(mtx);
        stop = true;
    }
    cv.notify_all();

    // 等待所有线程结束
    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
