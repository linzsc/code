#include <iostream>
using namespace std;
template<typename T>class test{
    T a;
    public:
    test(T x){
        a=x;
    }
    void display(){
        cout<<a<<endl;
    }
};
int main(){
    cout<<"hello world"<<endl;
    test<int> t(10);
    t.display();
    return 0;
}