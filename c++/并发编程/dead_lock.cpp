#include "head.h"



std::mutex  t_lock1;
std::mutex  t_lock2;
int m_1 = 0;
int m_2 = 1;

void dead_lock1() {
    while (true) {
        std::cout << "dead_lock1 begin " << std::endl;
        t_lock1.lock();
        m_1 = 1024;
        t_lock2.lock();
        m_2 = 2048;
        t_lock2.unlock();
        t_lock1.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        std::cout << "dead_lock2 end " << std::endl;
    }
}

void dead_lock2() {
    while (true) {
        std::cout << "dead_lock2 begin " << std::endl;
        t_lock2.lock();
        m_2 = 2048;
        t_lock1.lock();
        m_1 = 1024;
        t_lock1.unlock();
        t_lock2.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        std::cout << "dead_lock2 end " << std::endl;
    }
}



void test_dead_lock() {
    std::thread t1(dead_lock1);
    std::thread t2(dead_lock2);
    t1.join();
    t2.join();
}

int main(){
    test_dead_lock();
    return 0;
}