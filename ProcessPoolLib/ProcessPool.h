#pragma once

#include <string>

#include "JobObject.h"


class ProcessPool
{
public:
    void CreateProcess(const std::string& process, const std::string& args);

private:
    JobObject m_job;
};
