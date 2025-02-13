/*
    std:thread t(fun,arg,...);创建新的线程
    t.join();让主线程等待子线程启动运行，子线程运行结束后主线程再运行
    t.detach(),分离，主线程不等待子线程

    
    
    lambdab表达式[捕获值](传递参数)-> type{函数体+返回值（可有可无）}

    不能vector<int &>x;
    但是可以vector<int> &x;
    两者是不同的。

    可以通过std::ref()进行引用包装

    在thread中，如果有int &的参数，可以通过
    如f(int &x);
    thread(f,std::ref(x));


    可以通过move将任务进行转移
    std::thread t1(some_function); 
    转移t1管理的线程给t2，转移后t1无效    
    std::thread t2 =  std::move(t1);
    t1 = std::thread(some_other_function);重新绑定
    但是不能move给已经绑定了任务的线程


    用vector存储时，注意emplace和push的区别

    
*/



#include <iostream>
#include <thread>
#include <vector>
using namespace std;



class background_task {
public:
    string s; 
    // 重载函数调用运算符，用于输出字符串
    void operator()(std::string str) {
        // 输出字符串
        std::cout << "str is " << str << std::endl;
    }   
    background_task  operator+(const std::string& ss){  
        background_task new1;
        //this->s+"___"+ss;
        new1.s=this->s+"___"+ss;
        return  new1;
    }
};

void task() {
    for (int i = 0; i < 10; ++i) {
        std::cout << "Background task running: " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Background task completed" << std::endl;
}

void print_str(int num,string s){
    std::this_thread::sleep_for(std::chrono::seconds(5));
    cout<<s<<endl;
}
void danger_oops(int som_param) {
    char buffer[1024]="231321";
    sprintf(buffer, "%i", som_param);
    //在线程内部将char const* 转化为std::string
    //指针常量  char const*  指针本身不能变
    //常量指针  const char * 指向的内容不能变
    std::thread t(print_str, 3, buffer);
    t.detach();
    std::cout << "danger oops finished " << std::endl;
}



void safe_oops(int some_param) {
    char buffer[1024]="231321";
    sprintf(buffer, "%i", some_param);
    std::thread t(print_str, 3, std::string(buffer));
    t.detach();
}
int main() {

    int a=1;
    //x.push_back(&a);
    background_task task1;
    task1.s="aaa";
    task1=task1+"sss";
    std::cout<<task1.s<<std::endl;
    // 通过实例化 background_task 类创建线程
    std::thread t3(background_task(), "Hello from t3!");  // 正确
    std::thread t4(background_task(), "Hello from t4!");  // 正确

    {
        std::thread t(task);  // 创建一个线程

        t.detach();  // 将线程与当前线程分离
        std::cout << "{} thread continues execution" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "{} thread ends" << std::endl;
    }

   
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "Main thread ends" << std::endl;



    t3.join();  // 等待 t3 完成
    t4.join();  // 等待 t4 完成

    {
        danger_oops(1);
        safe_oops(2);
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}




