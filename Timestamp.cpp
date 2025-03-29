#include "Timestamp.h"

// 用当前时间初始化对象
Timestamp::Timestamp()    
{
    secsinceepoch_ = time(0);
}

// 用一个整数表示的时间初始化对象
Timestamp::Timestamp(int64_t secsinceepoch) : secsinceepoch_(secsinceepoch)   
{
    
}

// 返回当前时间的Timestamp对象
Timestamp Timestamp::now() 
{
    return Timestamp();     // 返回当前时间
}

// 返回整数表示的时间
time_t Timestamp::toint() const   
{
    return secsinceepoch_;
}

// 返回字符串表示的时间，格式：yyyy-mm-dd hh24:mi:ss
std::string Timestamp::tostring() const   
{
    using std::to_string;
    tm tmnow;
    localtime_r(&secsinceepoch_, &tmnow);
    std::string stime = to_string(tmnow.tm_year+1900)+"-"
               + to_string(tmnow.tm_mon+1)+"-"
               + to_string(tmnow.tm_mday)+" "
               + to_string(tmnow.tm_hour)+":"
               + to_string(tmnow.tm_min)+":"
               + to_string(tmnow.tm_sec);
    
    return stime;
}

/*
#include <unistd.h>
#include <iostream>

int main() 
{
    Timestamp ts;
    std::cout << ts.toint() << std::endl;
    std::cout << ts.tostring() << std::endl;

    sleep(1);
    std::cout << Timestamp::now().toint() << std::endl;
    std::cout << Timestamp::now().tostring() << std::endl;
}

// g++ -o test Timestamp.cpp
*/