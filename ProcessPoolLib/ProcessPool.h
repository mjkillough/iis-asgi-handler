#pragma once

#include <string>
#include <thread>
#include <vector>

#include "JobObject.h"


class Logger;


class ProcessPool
{
public:
    ProcessPool(
        const Logger& logger, const std::wstring& process,
        const std::wstring& arguments, size_t num_processes
    );
    ~ProcessPool();

    void Stop() const;

protected:
    void ThreadMain();
    void CreateProcess();
    static std::wstring EscapeArgument(const std::wstring& argument);

private:
    const Logger& logger;
    JobObject m_job;

    // Process name, argument list and them both joined as the full command line.
    std::wstring m_process;
    std::wstring m_arguments;
    std::wstring m_command_line;
    // Number of processes we should aim to have in the pool.
    size_t m_num_processes;

    // The pool has its own thread which it uses to monitor the processes it creates,
    // so that it can restart them if they exit.
    std::thread m_thread;
    ScopedHandle m_thread_exit_event;

    // We store one ScopedHandle for each process that we create. These are owned
    // by the thread.
    std::vector<ScopedHandle> m_processes;
};
