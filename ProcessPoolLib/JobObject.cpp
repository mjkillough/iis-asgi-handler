#include "JobObject.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


JobObject::JobObject()
{
    Create();
}


void JobObject::Create()
{
    m_handle = ScopedHandle{ ::CreateJobObject(nullptr, nullptr) };

    // Tell the Job to terminate all of its processes when the handle
    // to it is closed.
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info = { 0 };
    job_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    ::SetInformationJobObject(
        m_handle.Get(), JobObjectExtendedLimitInformation,
        &job_info, sizeof(job_info)
    );
}
