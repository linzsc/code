#include <iostream>

void capture_by_value() {
    int z=99;
    {
        int x = 10;
        int y = 20;

        // 捕获外部作用域中的所有变量的副本
        auto lambda = [=]() {
            std::cout << "x: " << x << ", y: " << y << ", Z: "<<z<<std::endl;
        };

        x = 30;  // 修改原始变量
        lambda();  // 输出: x: 10, y: 20
    }
}

int main() {
    capture_by_value();
    return 0;
}
