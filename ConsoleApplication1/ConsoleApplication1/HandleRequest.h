#pragma once

#ifndef _handle_request_
#define _handle_request_
#include "ConstValues.h"

typedef struct requestInfor {
	char method[20];
	char requestURI[BUFF_SIZE];
	char versionHTTP[20];
}REQUEST_INFOR;

inline bool isOverflow(int _count) {
	return !(_count % WSA_MAXIMUM_WAIT_EVENTS);
}

void analyzeHTTPRequest(char* request, REQUEST_INFOR* result);
bool isBadRequest(REQUEST_INFOR request);
int getRequestMethod(REQUEST_INFOR request);
#endif // !_handle_request

