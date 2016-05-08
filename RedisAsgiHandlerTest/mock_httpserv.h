#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "gmock/gmock.h"


class MockIHttpRequest : public IHttpRequest {
public:
    MOCK_METHOD0(GetRawHttpRequest, HTTP_REQUEST*());
    MOCK_CONST_METHOD0(GetRawHttpRequest, const HTTP_REQUEST*());
    MOCK_CONST_METHOD2(GetHeader, PCSTR(PCSTR, USHORT *));
    MOCK_CONST_METHOD2(GetHeader, PCSTR(HTTP_HEADER_ID, USHORT *));
    MOCK_METHOD4(SetHeader, HRESULT(PCSTR pszHeaderName, PCSTR pszHeaderValue, USHORT cchHeaderValue, BOOL fReplace));
    MOCK_METHOD4(SetHeader, HRESULT(HTTP_HEADER_ID ulHeaderIndex, PCSTR pszHeaderValue, USHORT cchHeaderValue, BOOL fReplace));
    MOCK_METHOD1(DeleteHeader, HRESULT(PCSTR pszHeaderName));
    MOCK_METHOD1(DeleteHeader, HRESULT(HTTP_HEADER_ID ulHeaderIndex));
    MOCK_CONST_METHOD0(GetHttpMethod, PCSTR());
    MOCK_METHOD1(SetHttpMethod, HRESULT(PCSTR pszHttpMethod));
    MOCK_METHOD3(SetUrl, HRESULT(PCWSTR pszUrl, DWORD cchUrl, BOOL fResetQueryString));
    MOCK_METHOD3(SetUrl, HRESULT(PCSTR pszUrl, DWORD cchUrl, BOOL fResetQueryString));
    MOCK_CONST_METHOD0(GetUrlChanged, BOOL());
    MOCK_CONST_METHOD0(GetForwardedUrl, PCWSTR());
    MOCK_CONST_METHOD0(GetLocalAddress, PSOCKADDR());
    MOCK_CONST_METHOD0(GetRemoteAddress, PSOCKADDR());
    MOCK_METHOD5(ReadEntityBody, HRESULT(VOID *, DWORD, BOOL, DWORD *, BOOL *));
    MOCK_METHOD2(InsertEntityBody, HRESULT(VOID * pvBuffer, DWORD cbBuffer));
    MOCK_METHOD0(GetRemainingEntityBytes, DWORD());
    MOCK_CONST_METHOD2(GetHttpVersion, VOID(USHORT * pMajorVersion, USHORT * pMinorVersion));
    MOCK_METHOD2(GetClientCertificate, HRESULT(HTTP_SSL_CLIENT_CERT_INFO ** ppClientCertInfo, BOOL * pfClientCertNegotiated));
    MOCK_METHOD2(NegotiateClientCertificate, HRESULT(BOOL, BOOL *));
    MOCK_CONST_METHOD0(GetSiteId, DWORD());
    MOCK_METHOD9(GetHeaderChanges, HRESULT(DWORD dwOldChangeNumber, DWORD * pdwNewChangeNumber, PCSTR knownHeaderSnapshot[HttpHeaderRequestMaximum], DWORD * pdwUnknownHeaderSnapshot, PCSTR ** ppUnknownHeaderNameSnapshot, PCSTR ** ppUnknownHeaderValueSnapshot, DWORD diffedKnownHeaderIndices[HttpHeaderRequestMaximum + 1], DWORD * pdwDiffedUnknownHeaders, DWORD ** ppDiffedUnknownHeaderIndices));
};


class MockIHttpResponse : public IHttpResponse {
public:
    MOCK_METHOD0(GetRawHttpResponse, HTTP_RESPONSE*());
    MOCK_CONST_METHOD0(GetRawHttpResponse, const HTTP_RESPONSE*());
    MOCK_METHOD0(GetCachePolicy, IHttpCachePolicy*());
    MOCK_METHOD6(SetStatus, HRESULT(USHORT, PCSTR, USHORT, HRESULT, IAppHostConfigException *, BOOL));
    MOCK_METHOD4(SetHeader, HRESULT(PCSTR pszHeaderName, PCSTR pszHeaderValue, USHORT cchHeaderValue, BOOL fReplace));
    MOCK_METHOD4(SetHeader, HRESULT(HTTP_HEADER_ID ulHeaderIndex, PCSTR pszHeaderValue, USHORT cchHeaderValue, BOOL fReplace));
    MOCK_METHOD1(DeleteHeader, HRESULT(PCSTR pszHeaderName));
    MOCK_METHOD1(DeleteHeader,  HRESULT(HTTP_HEADER_ID ulHeaderIndex));
    MOCK_CONST_METHOD2(GetHeader, PCSTR(PCSTR, USHORT *));
    MOCK_CONST_METHOD2(GetHeader, PCSTR(HTTP_HEADER_ID, USHORT *));
    MOCK_METHOD0(Clear, VOID());
    MOCK_METHOD0(ClearHeaders, VOID());
    MOCK_METHOD0(SetNeedDisconnect, VOID());
    MOCK_METHOD0(ResetConnection, VOID());
    MOCK_METHOD1(DisableKernelCache, VOID(ULONG));
    MOCK_CONST_METHOD0(GetKernelCacheEnabled, BOOL());
    MOCK_METHOD0(SuppressHeaders, VOID());
    MOCK_CONST_METHOD0(GetHeadersSuppressed, BOOL());
    MOCK_METHOD4(Flush, HRESULT(BOOL, BOOL, DWORD *, BOOL *));
    MOCK_METHOD3(Redirect, HRESULT(PCSTR, BOOL, BOOL));
    MOCK_METHOD2(WriteEntityChunkByReference, HRESULT(HTTP_DATA_CHUNK *, LONG));
    MOCK_METHOD6(WriteEntityChunks, HRESULT(HTTP_DATA_CHUNK *, DWORD, BOOL, BOOL, DWORD *, BOOL *));
    MOCK_METHOD0(DisableBuffering, VOID());
    MOCK_METHOD9(GetStatus, VOID(USHORT *, USHORT *, PCSTR *, USHORT *, HRESULT *, PCWSTR *, DWORD *, IAppHostConfigException**, BOOL *));
    MOCK_METHOD3(SetErrorDescription, HRESULT(PCWSTR, DWORD, BOOL));
    MOCK_METHOD1(GetErrorDescription, PCWSTR(DWORD *));
    MOCK_METHOD9(GetHeaderChanges, HRESULT(DWORD dwOldChangeNumber, DWORD * pdwNewChangeNumber, PCSTR knownHeaderSnapshot[HttpHeaderResponseMaximum], DWORD * pdwUnknownHeaderSnapshot, PCSTR ** ppUnknownHeaderNameSnapshot, PCSTR ** ppUnknownHeaderValueSnapshot, DWORD diffedKnownHeaderIndices[HttpHeaderResponseMaximum + 1], DWORD * pdwDiffedUnknownHeaders, DWORD ** ppDiffedUnknownHeaderIndices));
    MOCK_METHOD0(CloseConnection, VOID());
};



class MockIHttpContext : public IHttpContext {
public:
    MOCK_METHOD0(GetSite, IHttpSite*());
    MOCK_METHOD0(GetApplication, IHttpApplication*());
    MOCK_METHOD0(GetConnection, IHttpConnection*());
    MOCK_METHOD0(GetRequest, IHttpRequest*());
    MOCK_METHOD0(GetResponse, IHttpResponse*());
    MOCK_CONST_METHOD0(GetResponseHeadersSent, BOOL());
    MOCK_CONST_METHOD0(GetUser, IHttpUser*());
    MOCK_METHOD0(GetModuleContextContainer, IHttpModuleContextContainer*());
    MOCK_METHOD1(IndicateCompletion, VOID(REQUEST_NOTIFICATION_STATUS notificationStatus));
    MOCK_METHOD1(PostCompletion, HRESULT(DWORD cbBytes));
    MOCK_METHOD2(DisableNotifications, VOID(DWORD dwNotifications, DWORD dwPostNotifications));
    MOCK_METHOD5(GetNextNotification, BOOL(REQUEST_NOTIFICATION_STATUS status, DWORD * pdwNotification, BOOL * pfIsPostNotification, CHttpModule ** ppModuleInfo, IHttpEventProvider ** ppRequestOutput));

    MOCK_METHOD1(GetIsLastNotification, BOOL(REQUEST_NOTIFICATION_STATUS status));
    MOCK_METHOD5(ExecuteRequest, HRESULT(BOOL fAsync, IHttpContext * pHttpContext, DWORD dwExecuteFlags, IHttpUser * pHttpUser, BOOL * pfCompletionExpected));
    MOCK_CONST_METHOD0(GetExecuteFlags, DWORD());
    MOCK_METHOD3(GetServerVariable, HRESULT(PCSTR pszVariableName, PCWSTR * ppszValue, DWORD * pcchValueLength));
    MOCK_METHOD3(GetServerVariable, HRESULT(PCSTR pszVariableName, PCSTR * ppszValue, DWORD * pcchValueLength));
    MOCK_METHOD2(SetServerVariable, HRESULT(PCSTR pszVariableName, PCWSTR pszVariableValue));
    MOCK_METHOD1(AllocateRequestMemory, VOID*(DWORD cbAllocation));
    MOCK_METHOD0(GetUrlInfo, IHttpUrlInfo*());
    MOCK_METHOD0(GetMetadata, IMetadataInfo*());
    MOCK_METHOD1(GetPhysicalPath, PCWSTR(DWORD *));
    MOCK_CONST_METHOD1(GetScriptName, PCWSTR(DWORD *));
    MOCK_METHOD1(GetScriptTranslated, PCWSTR(DWORD *));
    MOCK_CONST_METHOD0(GetScriptMap, IScriptMapInfo*());
    MOCK_METHOD0(SetRequestHandled, VOID());
    MOCK_CONST_METHOD0(GetFileInfo, IHttpFileInfo*());
    MOCK_METHOD3(MapPath, HRESULT(PCWSTR pszUrl, PWSTR pszPhysicalPath, DWORD * pcbPhysicalPath));
    MOCK_METHOD2(NotifyCustomNotification, HRESULT(ICustomNotificationProvider * pCustomOutput, BOOL * pfCompletionExpected));
    MOCK_CONST_METHOD0(GetParentContext, IHttpContext*());
    MOCK_CONST_METHOD0(GetRootContext, IHttpContext*());
    MOCK_METHOD2(CloneContext, HRESULT(DWORD dwCloneFlags, IHttpContext ** ppHttpContext));
    MOCK_METHOD0(ReleaseClonedContext, HRESULT());
    MOCK_CONST_METHOD6(GetCurrentExecutionStats, HRESULT(DWORD *, DWORD *, PCWSTR *, DWORD *, DWORD *, DWORD *));
    MOCK_CONST_METHOD0(GetTraceContext, IHttpTraceContext*());
    MOCK_METHOD7(GetServerVarChanges, HRESULT(DWORD dwOldChangeNumber, DWORD * pdwNewChangeNumber, DWORD * pdwVariableSnapshot, PCSTR ** ppVariableNameSnapshot, PCWSTR ** ppVariableValueSnapshot, DWORD * pdwDiffedVariables, DWORD ** ppDiffedVariableIndices));
    MOCK_METHOD0(CancelIo, HRESULT());
    MOCK_METHOD6(MapHandler, HRESULT(DWORD, PCWSTR, PCWSTR, PCSTR, IScriptMapInfo**, BOOL));

    #pragma warning(disable : 4996)
    MOCK_METHOD2(GetExtendedInterface, HRESULT(HTTP_CONTEXT_INTERFACE_VERSION version, PVOID * ppInterface));
};
