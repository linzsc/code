#include <iostream>
#include <future>
#include <stdexcept>
#include <thread>

void may_throw()
{
    // 这里我们抛出一个异常。在实际的程序中，这可能在任何地方发生。
    throw std::runtime_error("Oops, something went wrong!");
}

int main()
{
    // 创建一个异步任务
    std::future<void> result=(std::async(std::launch::async, may_throw));

    try
    {
        // 获取结果（如果在获取结果时发生了异常，那么会重新抛出这个异常）
        result.get();
    }
    catch (const std::exception &e)
    {
        // 捕获并打印异常
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }

    return 0;
}