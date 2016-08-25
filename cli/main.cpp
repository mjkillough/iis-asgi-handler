#include <iostream>

#include <msgpack.hpp>

#include "ProcessPool.h"


void main()
{
    {
        ProcessPool p{ L"C:\\Windows\\System32\\cmd.exe", L"/K \"set A=1\"" };
        std::cin.get();
    }
    std::cin.get();
}
