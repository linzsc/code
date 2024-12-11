#include "head.h"
#include <iostream>
#include <vector>
#include <thread>
#include <numeric> // for std::accumulate
#include <algorithm> // for std::min
#include <iterator> // for std::distance, std::advance
#include <functional> // for std::ref

using ulc = unsigned long const;

// 定义 accumulate_block 函数对象
template <typename Iterator, typename T>
struct accumulate_block {
    void operator()(Iterator first, Iterator last, T& result) {
        result = std::accumulate(first, last, T{}); // 计算区间内的累加结果
    }
};

// 定义 parallel_accumulate 函数
template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
    ulc length = std::distance(first, last);
    if (!length) // 如果范围为空，直接返回初始值
        return init;

    ulc min_per_thread = 25;
    ulc max_threads = (length + min_per_thread - 1) / min_per_thread;

    ulc hardware_threads = std::thread::hardware_concurrency(); // 获取硬件支持的线程数
    std::cout<<"hardware_threads:"<<hardware_threads<<std::endl;
    ulc num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads); // 确定线程数量
    std::cout<<"num_threads:"<<num_threads<<std::endl;
    //num_threads=4

    ulc block_size = length / num_threads; // 每个线程处理的块大小
    std::vector<T> results(num_threads); // 存储每个线程的结果
    std::vector<std::thread> threads(num_threads - 1); // 存储线程对象
    Iterator block_start = first;

    // 为每个线程分配一个区间并启动线程
    for (unsigned long i = 0; i < (num_threads - 1); ++i) {
        Iterator block_end = block_start+block_size;
        //std::advance(block_end, block_size); // 计算每个线程的结束迭代器
        threads[i] = std::thread(
            accumulate_block<Iterator, T>(), 
            block_start, block_end, std::ref(results[i])
        );
        std::cout<<threads[i].get_id()<<" start! "<<std::endl;
        //打印线程id的类型
        //std::cout<<"threads[i].get_id():"<<typeid(threads[i].get_id()).name()<<std::endl;
        block_start = block_end; // 更新块的起点
    }

    // 主线程处理最后一个区间
    accumulate_block<Iterator, T>()(
        block_start, last, results[num_threads - 1]
    );

    // 等待所有线程完成
    for (auto& entry : threads)
        entry.join();

    // 合并所有线程的结果
    return std::accumulate(results.begin(), results.end(), init);
}



// 使用 parallel_accumulate 进行测试
void use_parallel_acc() {
    time_t start1 = clock();
    std::vector<int> vec(10000);
    std::iota(vec.begin(), vec.end(), 0); // 填充 0 到 9999

    int sum1 = 0;
    sum1 = parallel_accumulate<std::vector<int>::iterator, int>(
        vec.begin(), vec.end(), sum1);


    std::cout << "Sum is " << sum1 << std::endl;

    time_t end1 = clock();
    std::cout<<"time:"<<end1-start1<<std::endl;

    time_t start = clock();
    int sum2=0;
    for(auto &i:vec){
        sum2+=i;
    }
    std::cout << "Sum is " << sum2 << std::endl;
    time_t end = clock();
    std::cout<<"time:"<<end-start<<std::endl;
}

int main() {
    use_parallel_acc();
    return 0;
}
