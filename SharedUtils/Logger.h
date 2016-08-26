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

    LoggerStream& operator<<(const std::wstring& string);

protected:
    std::ostringstream m_sstream;
    const Logger& logger;
};


class Logger
{
    friend class LoggerStream;
public:
    virtual LoggerStream debug() const { return GetStream(); }
protected:
    virtual void Log(const std::string& msg) const = 0;
    virtual LoggerStream GetStream() const { return LoggerStream(*this); }
};


class EtwLogger : public Logger
{
public:
    EtwLogger(const GUID& etw_guid);
    virtual ~EtwLogger();
protected:
    virtual void Log(const std::string& msg) const;
private:
    ::REGHANDLE m_etw_handle{ 0 };
};
