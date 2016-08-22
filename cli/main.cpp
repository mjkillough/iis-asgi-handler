#include <iostream>

#include <msgpack.hpp>

#include "ProcessPool.h"


void main()
{
    {
        ProcessPool p;
        p.CreateProcess("C:\\Windows\\System32\\cmd.exe", "cmd.exe");
        std::cin.get();
    }
    std::cin.get();
}
