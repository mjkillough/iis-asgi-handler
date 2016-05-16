#define WIN32_LEAN_AND_MEAN
#include <ppltasks.h>

#include "HttpRequestHandler.h"
#include "HttpRequestHandlerSteps.h"
#include "Logger.h"


REQUEST_NOTIFICATION_STATUS HttpRequestHandler::OnExecuteRequestHandler()
{
    IHttpRequest *request = m_http_context->GetRequest();
    HTTP_REQUEST *raw_request = request->GetRawHttpRequest();

    auto asgi_request_msg = std::make_unique<AsgiHttpRequestMsg>();
    asgi_request_msg->reply_channel = m_channels.NewChannel("http.request!");
    asgi_request_msg->http_version = GetRequestHttpVersion(request);
    asgi_request_msg->method = std::string(request->GetHttpMethod());
    asgi_request_msg->scheme = GetRequestScheme(raw_request);
    asgi_request_msg->path = GetRequestPath(raw_request);
    asgi_request_msg->query_string = GetRequestQueryString(raw_request);
    asgi_request_msg->root_path = ""; // TODO: Same as SCRIPT_NAME in WSGI. What r that?
    asgi_request_msg->headers = GetRequestHeaders(raw_request);
    asgi_request_msg->body.resize(request->GetRemainingEntityBytes());

    m_current_step = std::make_unique<ReadBodyStep>(*this, std::move(asgi_request_msg));

    logger.debug() << typeid(*m_current_step.get()).name() << "->Enter() being called";

    auto result = m_current_step->Enter();

    logger.debug() << typeid(*m_current_step.get()).name() << "->Enter() = " << result;

    return HandlerStateMachine(result);
}

REQUEST_NOTIFICATION_STATUS HttpRequestHandler::OnAsyncCompletion(IHttpCompletionInfo* completion_info)
{
    bool async_pending = false;
    HRESULT hr = completion_info->GetCompletionStatus();
    DWORD bytes = completion_info->GetCompletionBytes();

    logger.debug() << typeid(*m_current_step.get()).name() << "->OnAsyncCompletion() being called";

    auto result = m_current_step->OnAsyncCompletion(hr, bytes);

    logger.debug() << typeid(*m_current_step.get()).name() << "->OnAsyncCompletion() = " << result;

    return HandlerStateMachine(result);
}

bool HttpRequestHandler::ReturnError(USHORT status, const std::string& reason)
{
    // TODO: Flush? Pass hr to SetStatus() to give better error message?
    IHttpResponse* response = m_http_context->GetResponse();
    response->Clear();
    response->SetStatus(status, reason.c_str());
    return false; // No async pending.
}

REQUEST_NOTIFICATION_STATUS HttpRequestHandler::HandlerStateMachine(StepResult _result)
{
    StepResult result = _result;
    // This won't loop forever. We expect to return AsyncPending fairly often.
    while (true) {
        switch (result) {
        case kStepAsyncPending:
            return RQ_NOTIFICATION_PENDING;
        case kStepFinishRequest:
            return RQ_NOTIFICATION_FINISH_REQUEST;
        case kStepRerun: {

            logger.debug() << typeid(*m_current_step.get()).name() << "->Enter() being called";
            result = m_current_step->Enter();
            logger.debug() << typeid(*m_current_step.get()).name() << "->Enter() = " << result;

            break;
        }
        case kStepGotoNext: {

            logger.debug() << typeid(*m_current_step.get()).name() << "->GetNextStep() being called";
            m_current_step = m_current_step->GetNextStep();

            logger.debug() << typeid(*m_current_step.get()).name() << "->Enter() being called";
            result = m_current_step->Enter();
            logger.debug() << typeid(*m_current_step.get()).name() << "->Enter() = " << result;

            break;
        }}
    }
}

