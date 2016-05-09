#pragma once

#include <string>
#include <sstream>
#include <cvt/wstring>
#include <codecvt>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <evntprov.h>


class Logger;

class LoggerStream
{
public:
    LoggerStream(const Logger& logger)
        : logger(logger)
    { }

    LoggerStream(LoggerStream&) = default;
    LoggerStream(LoggerStream&&) = default;

    virtual ~LoggerStream();

    template<typename T>
    LoggerStream& operator<<(const T& value)
    {
        m_sstream << value;
        return *this;
    }

protected:
    std::ostringstream m_sstream;
    const Logger& logger;
};


class Logger
{
    friend class LoggerStream;
public:
    Logger();
    virtual ~Logger();

    virtual LoggerStream debug() const { return GetStream(); }
protected:
    virtual void Log(const std::string& msg) const;
    virtual LoggerStream GetStream() const { return LoggerStream(*this); }
private:
    ::REGHANDLE m_etw_handle;
};
