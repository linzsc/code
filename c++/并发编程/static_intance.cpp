#include <iostream>
#include <atomic>
using namespace std;

class static_instance {
public:
    int num;
    int num2;
    
   
    static_instance(const static_instance&) = delete;
    static_instance& operator=(const static_instance&) = delete;

    
    static static_instance& a(){
        static static_instance ins;
        return ins;
    }

    ~static_instance() {
        cout << "Static Instance Destructor Called" << endl;
    }
private:
    static_instance(int n): num(n){
        cout << "Static Instance Constructor Called" << endl;
    }

    static_instance() {
        cout << "Static Instance Constructor Called" << endl;
    }  
};

int main() {
    static_instance& intance = static_instance::a();
    //static_instance intance(20);
    //cout << intance.num << endl;
    intance.num = 20;
    intance.num2 = 30;
    cout << intance.num << endl;
    cout << intance.num2 << endl;

    static_instance& intance2 = static_instance::a();
    //static_instance intance2(10);
    cout << intance2.num << endl;
    cout << intance2.num2 << endl;

    atomic<bool> value;
    value = true;
    atomic_bool value2 = true;
    atomic_long value3 = 10;
    return 0;
}