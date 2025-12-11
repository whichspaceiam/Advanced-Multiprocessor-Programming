#pragma once
#include "generics.hpp"


class BaseQueue{

    public:
    using value_t = generics::value_t;

    virtual bool push(value_t v)= 0;
    virtual value_t pop()=0;
    virtual int get_size() const =0;
    virtual ~BaseQueue() = default; 
};