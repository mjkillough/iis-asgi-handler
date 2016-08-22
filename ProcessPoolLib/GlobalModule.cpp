#include "GlobalModule.h"

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>


GLOBAL_NOTIFICATION_STATUS GlobalModule::OnGlobalApplicationStart(
    IHttpApplicationStartProvider* provider
)
{
    m_pool = std::make_unique<ProcessPool>();
    m_pool->CreateProcess("C:\\Windows\\System32\\cmd.exe", "cmd.exe");

    return GL_NOTIFICATION_CONTINUE;
}


GLOBAL_NOTIFICATION_STATUS GlobalModule::OnGlobalApplicationStop(
    IHttpApplicationStopProvider* provider
)
{
    // Destroy the pool, which will terminate all of the jobs.
    m_pool.reset();

    return GL_NOTIFICATION_CONTINUE;
}
