#include <cvt/wstring>
#include <codecvt>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModule.h"

#include "HttpModuleFactory.h"
#include "AsgiHttpResponseMsg.h"
#include "HttpRequestHandler.h"


HttpModule::HttpModule(const HttpModuleFactory& factory)
    : m_factory(factory)
{
}


REQUEST_NOTIFICATION_STATUS HttpModule::OnAcquireRequestState(
    IHttpContext *http_context,
    IHttpEventProvider *provider
)
{
    m_factory.Log(L"OnAcquireRequestState");

    // Freed by IIS when the IHttpContext is destroyed, via StoredRequestContext::CleanupStoredContext()
    auto request_ctx = new HttpRequestHandler();
    http_context->GetModuleContextContainer()->SetModuleContext(request_ctx, m_factory.module_id());
    return request_ctx->OnAcquireRequestState(http_context, provider);
}
