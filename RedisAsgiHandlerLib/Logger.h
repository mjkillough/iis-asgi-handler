#pragma once

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <evntprov.h>


class Logger
{
public:
    Logger();
    ~Logger();

    void Log(const std::wstring& msg) const;

private:
    ::REGHANDLE m_etw_handle;
};
