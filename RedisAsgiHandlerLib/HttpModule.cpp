#include <cvt/wstring>
#include <codecvt>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModule.h"

#include "HttpModuleFactory.h"
#include "AsgiHttpRequestMsg.h"
#include "AsgiHttpResponseMsg.h"
#include "RedisChannelLayer.h"


HttpModule::HttpModule(const HttpModuleFactory& factory)
    : m_factory(factory)
{
}


void SetErrorResponse(IHttpResponse *response, int status = 500, std::string error = "Internal Error")
{
    response->Clear();
    response->SetStatus(500, error.c_str());
}

REQUEST_NOTIFICATION_STATUS HttpModule::OnAcquireRequestState(
    IHttpContext *http_context,
    IHttpEventProvider *provider)
{
    m_factory.Log(L"OnAcquireRequestState");

    // Do we need a pool for channel managers? I am not sure we can
    // share the same RedisChannelLayer between threads and connecting
    // to redis is probably too expensive to do per-request.
    RedisChannelLayer channels;

    IHttpRequest *request = http_context->GetRequest();
    HTTP_REQUEST *raw_request = request->GetRawHttpRequest();
    IHttpResponse *response = http_context->GetResponse();

    std::string http_version;
    {
        USHORT http_ver_major, http_ver_minor;
        request->GetHttpVersion(&http_ver_major, &http_ver_minor);
        http_version = std::to_string(http_ver_major);
        if (http_ver_major == 1) {
            http_version += "." + std::to_string(http_ver_minor);
        }
    }
    
    std::string scheme, path, query_string;
    {
        std::wstring url(
            raw_request->CookedUrl.pFullUrl,
            raw_request->CookedUrl.FullUrlLength / sizeof(wchar_t)
        );
        std::wstring path_w(
            raw_request->CookedUrl.pAbsPath,
            raw_request->CookedUrl.AbsPathLength / sizeof(wchar_t)
        );
        std::wstring query_string_w(
            raw_request->CookedUrl.pQueryString,
            raw_request->CookedUrl.QueryStringLength / sizeof(wchar_t)
        );

        auto colon_idx = url.find(':');
        std::wstring scheme_w;
        // http.sys suggests a CookedUrl will always have a scheme, but
        // we play it safe. (We don't assume http/https though).
        if (colon_idx != url.npos) {
            scheme_w = url.substr(0, colon_idx);
        }

        std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
        scheme = utf8_conv.to_bytes(scheme_w);
        path = utf8_conv.to_bytes(path_w);
        query_string = utf8_conv.to_bytes(query_string_w);
    }

    struct AsgiHttpRequestMsg request_msg;
    request_msg.reply_channel = channels.NewChannel("http.request!");
    request_msg.http_version = http_version;
    request_msg.method = std::string(request->GetHttpMethod());
    request_msg.scheme = scheme;
    request_msg.path = path;
    request_msg.query_string = query_string;
    request_msg.root_path = ""; // TODO: Same as SCRIPT_NAME in WSGI. What r that?

    // Parse headers. We give "known headers" first, then unknown headers, in the
    // order that IIS gives them to us.
    // We need the Content-Length header. Grab it when we see it.
	// TODO: ASGI specifies to lower case headers.
    DWORD content_length = 0;
    {
        // TODO: Check these (and see if http.sys gives us them
        // anywhere - it must have them)! MJK did these on a train
        // based on the values in http.h
        std::map<USHORT, std::string> known_header_map({
            { HttpHeaderCacheControl, "Cache-Control" },
            { HttpHeaderConnection, "Connection" },
            { HttpHeaderDate, "Date" },
            { HttpHeaderKeepAlive, "Keep-Alive" },
            { HttpHeaderPragma, "Pragma" },
            { HttpHeaderTrailer, "Trailer" },
            { HttpHeaderTransferEncoding, "Transfer-Encoding" },
            { HttpHeaderUpgrade, "Upgrade" },
            { HttpHeaderVia, "Via" },
            { HttpHeaderWarning, "Warning" },

            { HttpHeaderAllow, "Allow" },
            { HttpHeaderContentLength, "Content-Length" },
            { HttpHeaderContentType, "ContentType" },
            { HttpHeaderContentEncoding, "Content-Encoding" },
            { HttpHeaderContentLanguage, "Content-Language" },
            { HttpHeaderContentLocation, "Content-Location" },
            { HttpHeaderContentMd5, "Content-Md5" },
            { HttpHeaderContentRange, "Content-Range" },
            { HttpHeaderExpires, "Expires" },
            { HttpHeaderLastModified, "Last-Modified" },

            { HttpHeaderAccept, "Accept" },
            { HttpHeaderAcceptCharset, "Accept-Charset" },
            { HttpHeaderAcceptEncoding, "Accept-Encoding" },
            { HttpHeaderAcceptLanguage, "Accept-Language" },
            { HttpHeaderAuthorization, "Authorization" },
            { HttpHeaderCookie, "Cookie" },
            { HttpHeaderExpect, "Expect" },
            { HttpHeaderFrom, "From" },
            { HttpHeaderHost, "Host" },
            { HttpHeaderIfMatch, "If-Match" },

            { HttpHeaderIfModifiedSince, "If-Modified-Since" },
            { HttpHeaderIfNoneMatch, "If-None-Match" },
            { HttpHeaderIfRange, "If-Range" },
            { HttpHeaderIfUnmodifiedSince, "If-Unmodified-Since" },
            { HttpHeaderMaxForwards, "Max-Forwards" },
            { HttpHeaderProxyAuthorization, "Proxy-Authorization" },
            { HttpHeaderReferer, "Referer" },
            { HttpHeaderRange, "Range" },
            { HttpHeaderTe, "Te" },
            { HttpHeaderTranslate, "Translate" },

            { HttpHeaderUserAgent, "User-Agent" }
        });

        PHTTP_KNOWN_HEADER known_headers = raw_request->Headers.KnownHeaders;
        for (USHORT i = 0; i < HttpHeaderRequestMaximum; i++) {
            if (known_headers[i].RawValueLength > 0) {
                auto value = std::string(known_headers[i].pRawValue, known_headers[i].RawValueLength);
                request_msg.headers.push_back(std::make_tuple(known_header_map[i], value));

                if (i == HttpHeaderContentLength) {
                    content_length = std::stoi(value);
                }
            }
        }

        PHTTP_UNKNOWN_HEADER unknown_headers = raw_request->Headers.pUnknownHeaders;
        for (USHORT i = 0; i < raw_request->Headers.UnknownHeaderCount; i++) {
            auto name = std::string(unknown_headers[i].pName, unknown_headers[i].NameLength);
            auto value = std::string(unknown_headers[i].pRawValue, unknown_headers[i].RawValueLength);
            request_msg.headers.push_back(std::make_tuple(name, value));
        }
    }

    // TODO: Consider writing the body directly to the msgpack object? We are copying around a lot.
    // TODO: Split the body into chunks?
    // TODO: Use async IO.
    // TODO: Timeout if we don't get enough data.
    request_msg.body.resize(content_length);
    DWORD bytes_received = 0;
    while (bytes_received < content_length) {
        request->ReadEntityBody(
            request_msg.body.data() + bytes_received,
            content_length - bytes_received,
            false, &bytes_received, nullptr
        );
    }

    channels.Send("http.request", request_msg);

    RedisData data = channels.Receive(request_msg.reply_channel);
    msgpack::object_handle response_msg_handle = msgpack::unpack(data.get(), data.length());
    AsgiHttpResponseMsg response_msg = response_msg_handle.get().as<AsgiHttpResponseMsg>();

    response->Clear();
    response->SetStatus(response_msg.status, ""); // Can we pass nullptr for the reason?
    DWORD bytes_sent = 0;
	// WriteEntityChunks doesn't seem to return bytes_sent?
    // while (bytes_sent < response_msg.content.length()) {
        HTTP_DATA_CHUNK chunk;
        chunk.DataChunkType = HttpDataChunkFromMemory;
        chunk.FromMemory.pBuffer = (PVOID)(response_msg.content.c_str() + bytes_sent);
        chunk.FromMemory.BufferLength = response_msg.content.length() - bytes_sent;
        response->WriteEntityChunks(&chunk, 1, false, false, &bytes_sent, nullptr);
    // }
	for (auto header : response_msg.headers) {
		std::string header_name = std::get<0>(header);
		std::string header_value = std::get<1>(header);
		response->SetHeader(header_name.c_str(), header_value.c_str(), header_value.length(), true);
	}

    return RQ_NOTIFICATION_FINISH_REQUEST;
}
