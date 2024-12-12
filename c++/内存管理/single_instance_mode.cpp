#include <iostream>
#include <memory>
#include <mutex>
//利用智能指针解决释放问题
class SingleAuto
{

private:
    SingleAuto()
    {
    }

    SingleAuto(const SingleAuto &) = delete;
    SingleAuto &operator=(const SingleAuto &) = delete;

    
public:
    
    ~SingleAuto()
    {
        std::cout << "single auto delete success " << std::endl;
    }

    static std::shared_ptr<SingleAuto> GetInst()
    {
        if (single != nullptr)
        {
            return single;
        }

        s_mutex.lock();
        if (single != nullptr)
        {
            s_mutex.unlock();
            return single;
        }
        single = std::shared_ptr<SingleAuto>(new SingleAuto);
        s_mutex.unlock();
        return single;
    }

private:
    static std::shared_ptr<SingleAuto> single;
    static std::mutex s_mutex;
};


// 智能指针方式
std::shared_ptr<SingleAuto> SingleAuto::single = nullptr;
std::mutex SingleAuto::s_mutex;
void test_singleauto()
{
    auto sp1 = SingleAuto::GetInst();
    auto sp2 = SingleAuto::GetInst();
    std::cout << "sp1  is  " << sp1 << std::endl;
    std::cout << "sp2  is  " << sp2 << std::endl;
    //此时存在隐患，可以手动删除裸指针，造成崩溃
    //delete sp1.get();       //调用sp1.get()的析构函数，

    //可以通过友元函数+删除器，来避免显示地调用析构函数，从而确保智能指针的自动释放，防止程序崩溃


}



// safe deletor
//防止外界delete
//声明辅助类
//该类定义仿函数调用SingleAutoSafe析构函数
//不可以提前声明SafeDeletor，编译时会提示incomplete type
// class SafeDeletor;
//所以要提前定义辅助类
class SingleAutoSafe;
class SafeDeletor
{
public:
    void operator()(SingleAutoSafe *sf)
    {
        std::cout << "this is safe deleter operator()" << std::endl;
        delete sf;
    }
};
class SingleAutoSafe
{
private:
    SingleAutoSafe() {}
    ~SingleAutoSafe()
    {
        std::cout << "this is single auto safe deletor" << std::endl;
    }
    SingleAutoSafe(const SingleAutoSafe &) = delete;
    SingleAutoSafe &operator=(const SingleAutoSafe &) = delete;
    //定义友元类，通过友元类调用该类析构函数
    friend class SafeDeletor;

public:
    static std::shared_ptr<SingleAutoSafe> GetInst()
    {
        if (single != nullptr)
        {
            return single;
        }

        s_mutex.lock();
        if (single != nullptr)
        {
            s_mutex.unlock();
            return single;
        }
        //额外指定删除器
        single = std::shared_ptr<SingleAutoSafe>(new SingleAutoSafe, SafeDeletor());
        //也可以指定删除函数
        // single = std::shared_ptr<SingleAutoSafe>(new SingleAutoSafe, SafeDelFunc);
        s_mutex.unlock();
        return single;
    }

private:
    static std::shared_ptr<SingleAutoSafe> single;
    static std::mutex s_mutex;
};


//智能指针初始化为nullptr
std::shared_ptr<SingleAutoSafe> SingleAutoSafe::single = nullptr;
std::mutex SingleAutoSafe::s_mutex;

void test_singleautosafe()
{
    auto sp1 = SingleAutoSafe::GetInst();
    auto sp2 = SingleAutoSafe::GetInst();
    std::cout << "sp1  is  " << sp1 << std::endl;
    std::cout << "sp2  is  " << sp2 << std::endl;
    //此时无法访问析构函数，非常安全
    // delete sp1.get();
}




int main(){
    test_singleauto();


    
    test_singleautosafe();
    return 0;
}
