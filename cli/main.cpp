#include <iostream>

#include <msgpack.hpp>

#include "ProcessPool.h"


void main()
{
    {
        ProcessPool p;
        p.CreateProcess("C:\\Windows\\System32\\cmd.exe", "/K \"set A=1\"");
        std::cin.get();
    }
    std::cin.get();
}
