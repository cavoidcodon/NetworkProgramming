#include <stdio.h>
#include "HandleRequest.h"
#include "ConstValues.h"
#include <WinSock2.h>

/*bool analyzeHTTPRequest(char* request, REQUEST_INFOR* result);
	parameter : char* request[IN] -> stored request header from client
				REQUEST_INFOR* result[OUT] -> pointer to structure will be contained 
											  information about header request (method, request-URI, version...)
*/

void analyzeHTTPRequest(char* request, REQUEST_INFOR* result) {
	memset(result->method, 0, sizeof(result->method));
	memset(result->requestURI, 0, sizeof(result->requestURI));
	memset(result->versionHTTP, 0, sizeof(result->versionHTTP));

	sscanf_s(request, "%s %s %s", result->method, _countof(result->method),
		result->requestURI, _countof(result->requestURI),
		result->versionHTTP, _countof(result->versionHTTP));
}

bool isBadRequest(REQUEST_INFOR request){
	if (strcmp(request.method, "GET")     &&
		strcmp(request.method, "POST")    &&
		strcmp(request.method, "HEAD")    &&
		strcmp(request.method, "OPTIONS") &&
		strcmp(request.method, "PUT")     &&
		strcmp(request.method, "DELETE")  &&
		strcmp(request.method, "TRACE")   &&
		strcmp(request.method, "CONNECT"))
		return false;
	if (strcmp(request.versionHTTP, "HTTP/1.1") &&
		strcmp(request.versionHTTP, "HTTP/1.0"))
		return false;
	return true;
}

int getRequestMethod(REQUEST_INFOR request) {
	if (!strcmp(request.method, "GET")) return REQUEST_GET;
	else if (!strcmp(request.method, "POST")) return REQUEST_POST;
	else if (!strcmp(request.method, "HEAD")) return REQUEST_HEAD;
	else if (!strcmp(request.method, "PUT")) return REQUEST_PUT;
	else if (!strcmp(request.method, "DELETE")) return REQUEST_DELETE;
	else if (!strcmp(request.method, "OPTIONS")) return REQUEST_OPTIONS;
	else if (!strcmp(request.method, "CONNECT")) return REQUEST_CONNECT;
	else if (!strcmp(request.method, "TRACE")) return REQUEST_TRACE;
}