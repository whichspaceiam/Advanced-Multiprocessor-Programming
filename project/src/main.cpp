#include <iostream>
#include "timer.hpp"


int main(){
    Timer t;
    std::cout<<"Hello master of distaster"<<std::endl;

    t.checkpoint();
    auto res = t.get_laps();
    for (auto timing: res)
        std::cout<<timing<<" [ms], ";
    std::cout<<std::endl;
}