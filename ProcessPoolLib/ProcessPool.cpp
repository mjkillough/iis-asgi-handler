#include "ProcessPool.h"

#include <vector>
#include <iostream>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


void ProcessPool::CreateProcess(const std::string& process, const std::string& args)
{
    // Make a combined command line, escaping process as necessary.
    std::string cmd = EscapeArgument(process) + " " + args;
    // We must make a copy as a char*, as CreateProcessW() can modify it.
    std::vector<char> cmd_buffer(cmd.length() + 1, '\0');
    std::copy(cmd.begin(), cmd.end(), cmd_buffer.begin());

    STARTUPINFO startup_info = { 0 };
    PROCESS_INFORMATION proc_info = { 0 };
    BOOL created = ::CreateProcess(
        nullptr, cmd_buffer.data(),
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


std::string ProcessPool::EscapeArgument(const std::string& argument)
{
    // See https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
    // Note: we do not escape in such a way that it can be passed to cmd.exe.

    // Only surround with quotes (and escape contents) if there is whitespace
    // or quotes in the argument.
    if (argument.find_first_of(" \t\n\v\"") == argument.npos) {
        return argument;
    }

    std::ostringstream escaped;
    auto escaped_it = static_cast<std::ostreambuf_iterator<char>>(escaped);
    escaped << '\"';

    auto num_backslashes = 0;
    for (auto& character : argument) {
        switch (character) {
        case '\\':
            ++num_backslashes;
            continue;
        case '"':
            // Escape all of the backslashes, plus add one more escape for
            // this quotation mark.
            std::fill_n(escaped_it, num_backslashes * 2 + 1, '\\');
            escaped << '"';
            break;
        default:
            // We don't need to escape the backslashes if they're not followed
            // by a quote.
            std::fill_n(escaped_it, num_backslashes, '\\');
            escaped << character;
            break;
        }
        num_backslashes = 0;
    }
    // As we're going to append a final " to the argument, we need to escape
    // all the remaining backslashes.
    std::fill_n(escaped_it, num_backslashes * 2, '\\');
    // We don't escape our final quote:
    escaped << '\"';

    return escaped.str();
}
