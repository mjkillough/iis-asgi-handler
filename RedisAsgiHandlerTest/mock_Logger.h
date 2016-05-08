#pragma once

#include "gmock/gmock.h"

#include "Logger.h"


class MockLogger : public Logger
{
public:
    MOCK_METHOD1(Log, void(std::wstring));
};
