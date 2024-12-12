#include "head.h"
#include <vector>
#include <list>
#include "thread_poor.h"
//c++ 版本的快速排序算法

ThreadPool& thread_pool=ThreadPool::instance(500);

template<typename T>
void quick_sort_recursive( std::vector<T>& arr, int start, int end) {
    if (start >= end) return;
    T key = arr[start];
    //std::cout<<"start: "<<start<<" end: "<<end<<std::endl;
    int left = start, right = end;
    while(left < right) {
        while (arr[right] >= key && left < right) right--;
        while (arr[left] <= key && left < right) left++;
        std::swap(arr[left], arr[right]);

        
    }
    //for(int )
    if (arr[left] < key) {
        std::swap(arr[left], arr[start]);
    }
    

    quick_sort_recursive(arr, start, left - 1);
    quick_sort_recursive(arr, left + 1, end);
}


void test_quick_sort() {
    std::vector<int> numlists(100000);
    std::generate(numlists.begin(), numlists.end(), std::rand);
    quick_sort_recursive(numlists, 0, 100000 - 1);
}



template<typename T>
std::list<T> sequential_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;

    //  ① 将input中的第一个元素放入result中，并且将这第一个元素从input中删除
    result.splice(result.begin(), input, input.begin());  

    //  ② 取result的第一个元素，将来用这个元素做切割，切割input中的列表。
    T const& pivot = *result.begin();    

    //  ③std::partition 是一个标准库函数，用于将容器或数组中的元素按照指定的条件进行分区，
    // 使得满足条件的元素排在不满足条件的元素之前。
    // 所以经过计算divide_point指向的是input中第一个大于等于pivot的元素
        auto divide_point = std::partition(input.begin(), input.end(),
            [&](T const& t) {return t < pivot; });    

    // ④ 我们将小于pivot的元素放入lower_part中
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
        divide_point);  

    // ⑤我们将lower_part传递给sequential_quick_sort 返回一个新的有序的从小到大的序列
    //lower_part 中都是小于divide_point的值
        auto new_lower(
            sequential_quick_sort(std::move(lower_part)));    
    // ⑥我们剩余的input列表传递给sequential_quick_sort递归调用，input中都是大于divide_point的值。
        auto new_higher(
            sequential_quick_sort(std::move(input)));    
        //⑦到此时new_higher和new_lower都是从小到大排序好的列表
        //将new_higher 拼接到result的尾部
        result.splice(result.end(), new_higher);    
        //将new_lower 拼接到result的头部
        result.splice(result.begin(), new_lower);   
        return result;
}

//并行版本
template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t) {return t < pivot; });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
        divide_point);
    // ①因为lower_part是副本，所以并行操作不会引发逻辑错误，这里可以启动future做排序
    std::future<std::list<T>> new_lower(
        std::async(&parallel_quick_sort<T>, std::move(lower_part)));

    // ②
    auto new_higher(
        parallel_quick_sort(std::move(input)));    
        result.splice(result.end(), new_higher);    
        result.splice(result.begin(), new_lower.get());    
        return result;
}

//线程池版本
//并行版本
template<typename T>
std::list<T> thread_pool_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t) {return t < pivot; });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
        divide_point);
    // ①因为lower_part是副本，所以并行操作不会引发逻辑错误，这里投递给线程池处理
    auto new_lower = thread_pool.commit(&parallel_quick_sort<T>, std::move(lower_part));
    // ②
    auto new_higher(
        parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}
void test_thread_pool_sort() {
    //std::list<int> numlists = { 6,1,0,7,5,2,9,-1 };
    std::list<int> numlists(100000);
    std::generate(numlists.begin(), numlists.end(), std::rand);
    auto sort_result = thread_pool_quick_sort(numlists);
    std::cout << "sorted result is ";
    for (auto iter = sort_result.begin(); iter != sort_result.end(); iter++) {
        std::cout << " " << (*iter);
    }
    std::cout << std::endl;
}

void test_sequential_quick() {//时间减半
    //std::list<int> numlists = { 6,1,0,7,5,2,9,-1 };
    std::list<int> numlists(100000);
    std::generate(numlists.begin(), numlists.end(), std::rand);
    auto sort_result = sequential_quick_sort(numlists);
    //std::cout << "sorted result is ";
    /*
    for (auto iter = sort_result.begin(); iter != sort_result.end(); iter++) {
        std::cout << " " << (*iter);
    }
    */
    std::cout << std::endl;
}

template <typename Func, typename... Args>  //测量函数执行时间
void measure_time(Func&& func, Args&&... args) {
    time_t start = clock();  // 记录开始时间
    std::forward<Func>(func)(std::forward<Args>(args)...);  // 调用传入的函数
    time_t end = clock();    // 记录结束时间
    std::cout << "time: " << end - start << " clock ticks" << std::endl;
}

template <typename Func, typename... Args>
decltype(auto) measure_time_with_val(Func&& func, Args&&... args) {
    time_t start = clock();  // 记录开始时间
    // 调用传入的函数，并捕获其返回值
    decltype(auto) result = std::forward<Func>(func)(std::forward<Args>(args)...);
    time_t end = clock();    // 记录结束时间
    std::cout << "time: " << static_cast<double>(end - start) / CLOCKS_PER_SEC << " seconds" << std::endl;
    return result;           // 返回被调用函数的返回值  
}

int main() {
    measure_time(test_quick_sort);
    measure_time(test_sequential_quick);
    
    //measure_time(test_thread_pool_sort);
    return 0;
}

