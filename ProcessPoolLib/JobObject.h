#pragma once

#include "ScopedResources.h"


class JobObject
{
public:
    JobObject();
    JobObject(JobObject&& other) = default;
    JobObject(JobObject&) = delete;

    void operator=(JobObject) = delete;
    JobObject& operator=(JobObject&&) = default;

    HANDLE GetHandle() const { return m_handle.Get(); }

protected:
    void Create();

private:
    ScopedHandle m_handle;
};
