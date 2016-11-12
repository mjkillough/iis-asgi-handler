#include "ProcessPool.h"

#include <vector>
#include <iostream>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Logger.h"


ProcessPool::ProcessPool(
    const Logger& logger, const std::wstring& process,
    const std::wstring& arguments
)
    : logger(logger), m_process(process), m_arguments(arguments)
{
    // Make a combined command line, escaping process as necessary.
    m_command_line = EscapeArgument(process) + L" " + arguments;

    logger.debug() << "ProcessPool initialized with command line: "
                   << m_command_line;

    CreateProcess();
}


void ProcessPool::CreateProcess()
{
    // We must make a copy as a char*, as CreateProcessW() can modify it.
    auto cmd_buffer = std::vector<char>(m_command_line.length() + 1, '\0');
    std::copy(m_command_line.begin(), m_command_line.end(), cmd_buffer.begin());

    auto startup_info = STARTUPINFO{ 0 };
    auto proc_info = PROCESS_INFORMATION{ 0 };
    auto created = ::CreateProcess(
        nullptr, cmd_buffer.data(),
        nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr,
        &startup_info, &proc_info
    );
    if (!created) {
        logger.debug() << "Could not create process: " << ::GetLastError();
        return;
    }

    // Assign the process to the job and resume it. If we fail to do either,
    // then terminate the process.
    auto assigned = ::AssignProcessToJobObject(m_job.GetHandle(), proc_info.hProcess);
    auto resumed = ::ResumeThread(proc_info.hThread) != -1;
    if (!assigned || !resumed) {
        logger.debug() << "AssignProcessToJobObject=" << assigned << " ResumeThread="
                       << resumed << " GetLastError()=" << GetLastError();
        ::TerminateProcess(proc_info.hProcess, 0);
        ::CloseHandle(proc_info.hProcess);
        ::CloseHandle(proc_info.hThread);
    }
}


std::wstring ProcessPool::EscapeArgument(const std::wstring& argument)
{
    // See https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
    // Note: we do not escape in such a way that it can be passed to cmd.exe.

    // Only surround with quotes (and escape contents) if there is whitespace
    // or quotes in the argument.
    if (argument.find_first_of(L" \t\n\v\"") == argument.npos) {
        return argument;
    }

    auto escaped = std::wostringstream{ L"\"", std::ios_base::ate };
    auto escaped_it = static_cast<std::ostreambuf_iterator<wchar_t>>(escaped);
    auto num_backslashes = 0;

    for (auto& character : argument) {
        switch (character) {
        case L'\\':
            ++num_backslashes;
            continue;
        case L'"':
            // Escape all of the backslashes, plus add one more escape for
            // this quotation mark.
            std::fill_n(escaped_it, num_backslashes * 2 + 1, L'\\');
            escaped << L'"';
            break;
        default:
            // We don't need to escape the backslashes if they're not followed
            // by a quote.
            std::fill_n(escaped_it, num_backslashes, L'\\');
            escaped << character;
            break;
        }
        num_backslashes = 0;
    }
    // As we're going to append a final " to the argument, we need to escape
    // all the remaining backslashes.
    std::fill_n(escaped_it, num_backslashes * 2, L'\\');
    // We don't escape our final quote:
    escaped << L'\"';

    return escaped.str();
}
