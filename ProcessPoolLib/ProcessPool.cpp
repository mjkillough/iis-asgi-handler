#include "ProcessPool.h"

#include <vector>
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


void ProcessPool::CreateProcess(const std::string& process, const std::string& args)
{
    // We must make a copy of args.c_str(), as CreateProcessW()
    // can modify its contents.
    std::vector<char> args_buffer(args.length() + 1, '\0');
    memcpy_s(args_buffer.data(), args_buffer.size(), args.c_str(), args.length());

    STARTUPINFO startup_info = { 0 };
    PROCESS_INFORMATION proc_info = { 0 };
    BOOL created = ::CreateProcess(
        process.c_str(), args_buffer.data(),
        nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr,
        &startup_info, &proc_info
    );
    if (!created) {
        std::cout << "Could not create process " << process << ": "
                  << ::GetLastError() << std::endl;
        return;
    }

    // Assign the process to the job and resume it. If we fail to do either,
    // then terminate the process.
    BOOL assigned = ::AssignProcessToJobObject(m_job.GetHandle(), proc_info.hProcess);
    bool resumed = ::ResumeThread(proc_info.hThread) != -1;
    if (!assigned || !resumed) {
        std::cout << "AssignProcessToJobObject=" << assigned << " ResumeThread="
                  << resumed << " GetLastError()=" << GetLastError() << std::endl;
        ::TerminateProcess(proc_info.hProcess, 0);
        ::CloseHandle(proc_info.hProcess);
        ::CloseHandle(proc_info.hThread);
    }
}
