#include <cvt/wstring>
#include <codecvt>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModule.h"

#include "HttpModuleFactory.h"
#include "RequestHandler.h"
#include "HttpRequestHandler.h"


HttpModule::HttpModule(const HttpModuleFactory& factory, ResponsePump& response_pump, const Logger& logger)
    : m_factory(factory), m_response_pump(response_pump), logger(logger)
{
    logger.debug() << "Created new HttpModule";
}

HttpModule::~HttpModule()
{
    logger.debug() << "Destroying HttpModule";
}

REQUEST_NOTIFICATION_STATUS HttpModule::OnExecuteRequestHandler(
    IHttpContext *http_context,
    IHttpEventProvider *provider
)
{
    logger.debug() << "HttpModule::OnExecuteRequestHandler()";

    // Freed by IIS when the IHttpContext is destroyed, via StoredRequestContext::CleanupStoredContext()
    RequestHandler *request_handler = new HttpRequestHandler(m_response_pump, m_channels, logger, http_context);
    http_context->GetModuleContextContainer()->SetModuleContext(request_handler, m_factory.module_id());

    return request_handler->OnExecuteRequestHandler();
}

REQUEST_NOTIFICATION_STATUS HttpModule::OnAsyncCompletion(
    IHttpContext* http_context, DWORD notification, BOOL post_notification,
    IHttpEventProvider* provider, IHttpCompletionInfo* completion_info
)
{
    logger.debug() << "HttpModule::OnAsyncCompletion()";

    // TODO: Assert we have a HttpRequestHandler in the context container?
    auto request_handler = static_cast<RequestHandler*>(
        http_context->GetModuleContextContainer()->GetModuleContext(m_factory.module_id())
    );

    return request_handler->OnAsyncCompletion(completion_info);
}
