#include <string>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <evntprov.h>

#include "Logger.h"


LoggerStream::~LoggerStream()
{
    logger.Log(m_sstream.str());
}


EtwLogger::EtwLogger(const GUID& etw_guid)
{
    ::EventRegister(&etw_guid, nullptr, nullptr, &m_etw_handle);
}


EtwLogger::~EtwLogger()
{
    if (m_etw_handle) {
        ::EventUnregister(m_etw_handle);
    }
}


void EtwLogger::Log(const std::string& msg) const
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    std::wstring wmsg = utf8_conv.from_bytes(msg);
    ::EventWriteString(m_etw_handle, 0, 0, wmsg.c_str());
}
