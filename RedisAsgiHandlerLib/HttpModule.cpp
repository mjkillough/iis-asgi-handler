#include <cvt/wstring>
#include <codecvt>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModule.h"

#include "HttpModuleFactory.h"
#include "AsgiHttpResponseMsg.h"
#include "HttpRequestHandler.h"
#include "ResponsePump.h"
#include "Logger.h"


HttpModule::HttpModule(const HttpModuleFactory& factory, ResponsePump& response_pump, const Logger& logger)
    : m_factory(factory), m_response_pump(response_pump), m_logger(logger)
{
}


REQUEST_NOTIFICATION_STATUS HttpModule::OnAcquireRequestState(
    IHttpContext *http_context,
    IHttpEventProvider *provider
)
{
    m_logger.Log(L"OnAcquireRequestState");

    // Freed by IIS when the IHttpContext is destroyed, via StoredRequestContext::CleanupStoredContext()
    auto request_handler = new HttpRequestHandler(m_factory, m_response_pump, m_logger, http_context);
    http_context->GetModuleContextContainer()->SetModuleContext(request_handler, m_factory.module_id());
    return request_handler->OnAcquireRequestState(http_context, provider);
}

REQUEST_NOTIFICATION_STATUS HttpModule::OnAsyncCompletion(
    IHttpContext* http_context, DWORD notification, BOOL post_notification,
    IHttpEventProvider* provider, IHttpCompletionInfo* completion_info
)
{
    m_logger.Log(L"OnAsyncCompletion");

    // TODO: Assert we have a HttpRequestHandler in the context container?
    auto request_handler = static_cast<HttpRequestHandler*>(
        http_context->GetModuleContextContainer()->GetModuleContext(m_factory.module_id())
    );
    return request_handler->OnAsyncCompletion(
        http_context, notification, post_notification,
        provider, completion_info
    );
}
