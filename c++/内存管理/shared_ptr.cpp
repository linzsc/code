#include <iostream>
#include<memory>
using namespace std;


/*
move 转移所有权，用于将一个对象显式地转换为右值引用，从而表明该对象的资源可以安全地“移动”（即转移所有权）
.release()释放原指针并返回对象
使用reset(.release())进行转移

unique_ptr转递的方式move()->nique_ptr<t> p
                   unique_ptr->unique_ptr<int>& p;
                   unique_ptr.get()->T* p     

*/
void print(unique_ptr<int> p)
{
    cout << *p << endl;
}

void print2(unique_ptr<int>& p) //用引用的方式进行转递
{
    cout << *p << endl;
}


int main()
{
    unique_ptr<int> p1(new int(10));
    print(move(p1));

    unique_ptr<int> p2(new int(20));
    unique_ptr<int> p3 = move(p2);
    print2(p3);

    //cout<<*p1<<endl; //报错，p1已经转移所有权，p1为空指针
    return 0;
}