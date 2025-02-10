#include <iostream>
#include <vector>
class TestCopy {
public:
    TestCopy() {}
    TestCopy(const TestCopy& tp) {
        std::cout << "Test Copy Copy " << std::endl;
    }
    TestCopy(TestCopy&& cp) {
        std::cout << "Test Copy Move " << std::endl;
    }
};


int main() {
    TestCopy a;
    TestCopy b(std::move(a));//貌似只有调用move函数才会调用移动构造函数
    
   
    return 0;
}