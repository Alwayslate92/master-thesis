#include <iostream>
#include <string.h>

struct A{
private:
    int a;
    friend class B;
};

struct B{
    A b;
    void print_a(){
        std::cout << b.a << "\n";
    }
};
