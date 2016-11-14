#include "ProcessPool.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Logger.h"


ProcessPool::ProcessPool(
    const Logger& logger, const std::wstring& process,
    const std::wstring& arguments, size_t num_processes
)
    : logger{ logger }, m_process{ process }, m_arguments{ arguments },
      m_num_processes{ num_processes }
{
    // Make a combined command line, escaping process as necessary.
    m_command_line = EscapeArgument(process) + L" " + arguments;

    logger.debug() << "ProcessPool initialized with command line: "
                   << m_command_line;

    m_thread_exit_event = ScopedHandle{ ::CreateEvent(nullptr, TRUE, FALSE, nullptr) };
    m_thread = std::thread{ &ProcessPool::ThreadMain, this };
}

ProcessPool::~ProcessPool()
{
    // Stop the monitoring thread and wait for it to join.
    // We don't explicitly terminate the running processes - they will terminate
    // when our job object is destructed.
    Stop();
    m_thread.join();
}

void ProcessPool::Stop() const
{
    // Signal for the thread to stop.
    ::SetEvent(m_thread_exit_event.Get());
}

void ProcessPool::ThreadMain()
{
    logger.debug() << "ProcessPool::ThreadMain(): Enter";

    while (true) {
        logger.debug() << "ProcessPool::ThreadMain(): Loop";

        // Start any missing processes.
        // TODO: Determine what to do if process creation fails. Should we retry
        //       a certain number of times and then give up? Should this affect
        //       the timeout we give to ::WaitForMultipleObjects()?
        for (auto i = m_processes.size(); i < m_num_processes; ++i) {
            CreateProcess();
        }

        // Get the handles for all processes we've already started and wait until one
        // of them is signaled. This will indicate that the process has been terminated.
        // Also watch m_thread_exit_event to see if we should terminate.
        auto handles = std::vector<HANDLE>(m_processes.size() + 1);
        std::transform(
            std::begin(m_processes), std::end(m_processes),
            std::begin(handles),
            [](const auto& sh) { return sh.Get(); }
        );
        handles[m_processes.size()] = m_thread_exit_event.Get();
        auto wait_result = ::WaitForMultipleObjects(
            handles.size(),
            handles.data(),
            FALSE,
            INFINITE
        );
        auto signaled_idx = wait_result - WAIT_OBJECT_0;

        if (signaled_idx == m_processes.size()) {
            logger.debug() << "ProcessPool::ThreadMain(): Exit signaled";
            break;
        }

        // If one of the handles was signaled, remove the associated process from our
        // list (as it has terminated). We'll attempt to start one in its place when
        // we loop around.
        if (signaled_idx >= 0 && signaled_idx < m_processes.size()) {
            logger.debug() << "ProcessPool::ThreadMain(): Process terminated";
            m_processes.erase(std::begin(m_processes) + signaled_idx);
        }

        // Regardless, go round the loop again. If it returned because of error,
        // lot it but continue.
        if (wait_result == WAIT_FAILED) {
            logger.debug() << "::WaitForMultipleObjects()=" << ::GetLastError();
        }
    }

    logger.debug() << "ProcessPool::ThreadMain(): Exit";
}

void ProcessPool::CreateProcess()
{
    logger.debug() << "Creating process with command line: " << m_command_line;

    // We must make a copy as a char*, as CreateProcessW() can modify it.
    auto cmd_buffer = std::vector<char>(m_command_line.length() + 1, '\0');
    std::copy(m_command_line.begin(), m_command_line.end(), cmd_buffer.begin());

    // We create the process with its main thread suspended, so that we can assign
    // it to our job object before it runs.
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
    auto process = ScopedHandle{ proc_info.hProcess };
    auto thread = ScopedHandle{ proc_info.hThread };

    // Assign the process to the job and resume it. If we fail to do either,
    // then terminate the process.
    auto assigned = ::AssignProcessToJobObject(m_job.GetHandle(), process.Get());
    auto resumed = ::ResumeThread(thread.Get()) != -1;
    if (!assigned || !resumed) {
        logger.debug() << "AssignProcessToJobObject=" << assigned << " ResumeThread="
                       << resumed << " GetLastError()=" << GetLastError();
        ::TerminateProcess(process.Get(), 0);
        return;
    }

    m_processes.push_back(std::move(process));
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
