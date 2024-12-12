#include <iostream>
#include <thread>
#include <future>

int count_of_threads = 0;
void thread1(int x,std::promise<int> prom) {
    // 子线程 1 执行一些工作并返回结果
    std::this_thread::sleep_for(std::chrono::seconds(1));  // 模拟一些工作
    int result = 42;  // 假设子线程 1 计算的结果是 42
    std::cout << "Thread 1 computed the value: " << result+x << std::endl;
    prom.set_value(result);  // 将结果传递给线程 2
}

void thread2(std::future<int> fut, std::promise<int> prom) {
    // 子线程 2 等待子线程 1 的结果并基于此结果继续计算
    int value_from_thread1 = fut.get();  // 获取来自线程 1 的结果
    std::cout << "Thread 2 received value from Thread 1: " << value_from_thread1 << std::endl;

    // 子线程 2 基于线程 1 的结果进行计算
    int result = value_from_thread1 * 2;  // 假设子线程 2 将结果乘以 2
    std::cout << "Thread 2 computed the value: " << result << std::endl;

    // 将结果通过 promise 返回给主线程
    prom.set_value(result);  // 将计算结果传递给主线程
}


void myFunction(std::promise<int>&& promise) {
    // 模拟一些工作
    std::this_thread::sleep_for(std::chrono::seconds(1));
    promise.set_value(42); // 设置 promise 的值
}

void threadFunction(std::shared_future<int> future) {
    try {
        int result = future.get();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        count_of_threads++;
        if(count_of_threads ==  2){
            std::cout<<count_of_threads<<std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << "Result: " << result << " "<< count_of_threads << std::endl;
    }
    catch (const std::future_error& e) {
        std::cout << "Future error: " << e.what() << std::endl;
    }
}

void use_shared_future() {
    std::promise<int> promise;
    std::shared_future<int> future = promise.get_future();

    std::thread myThread1(myFunction, std::move(promise)); // 将 promise 移动到线程中

    // 使用 share() 方法获取新的 shared_future 对象  

    std::thread myThread2(threadFunction, future);

    std::thread myThread3(threadFunction, future);

    myThread1.join();
    myThread2.join();
    myThread3.join();
}
int main() {
    // 创建 promise 和 future
    std::promise<int> prom1;  // 用于线程 1
    std::promise<int> prom2;  // 用于线程 2
    std::future<int> fut1 = prom1.get_future();  // 线程 2 获取线程 1 的结果
    std::future<int> fut2 = prom2.get_future();  // 主线程获取线程 2 的结果

    // 启动子线程 1 和 2
    std::thread t1(thread1, 10,std::move(prom1));  // 启动子线程 1
    std::thread t2(thread2, std::move(fut1), std::move(prom2));   // 启动子线程 2

    // 等待线程 2 完成并获取它的计算结果
    int result_from_thread2 = fut2.get();  // 获取线程 2 的计算结果
    std::cout << "Final result from Thread 2: " << result_from_thread2 << std::endl;

    // 等待子线程结束
    t1.join();
    t2.join();
    std::cout<<"use_shared_future"<<std::endl;
    use_shared_future();

    return 0;
}
