#pragma once

#include "gmock/gmock.h"

#include "Logger.h"


class MockLogger : public Logger
{
public:
    MOCK_CONST_METHOD1(Log, void(const std::string& msg));

    // The default GetStream() and level() methods are fine. The LogStream class doesn't do
    // anything but call bak to Log(). By not mocking them, we can test them.
    // MOCK_CONST_METHOD1(debug, LoggerStream());
    // MOCK_CONST_METHOD1(GetStream, LoggerStream());
};
