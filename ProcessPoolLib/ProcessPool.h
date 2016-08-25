#pragma once

#include <string>

#include "JobObject.h"


class ProcessPool
{
public:
    ProcessPool(const std::wstring& process, const std::wstring& arguments);

protected:
    void CreateProcess();
    static std::wstring EscapeArgument(const std::wstring& argument);

private:
    JobObject m_job;
    std::wstring m_process;
    std::wstring m_arguments;
    std::wstring m_command_line;
};
