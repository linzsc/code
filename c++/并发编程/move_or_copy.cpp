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
    TestCopy b(std::move(a));
   
    return 0;
}