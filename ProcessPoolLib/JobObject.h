#pragma once

#include "ScopedHandle.h"


class JobObject
{
public:
    JobObject();
    JobObject(JobObject&& other) = default;
    JobObject(JobObject&) = delete;

    void operator=(JobObject) = delete;
    JobObject& operator=(JobObject&&) = default;

    void Terminate();

    HANDLE GetHandle() const { return m_handle.Get(); }

protected:
    void Create();

private:
    ScopedHandle m_handle;
};
