#pragma once

#ifndef _handle_request_
#define _handle_request_
#include "ConstValues.h"

typedef struct requestInfor 
{
	int flag[3] = { 0, 0, 0 }; // {connection, range, contentlength}
	char method[20];
	char requestURI[BUFF_SIZE];
	char versionHTTP[20];
	char range[BUFF_SIZE];
	char connection[BUFF_SIZE];
	int contentLength; // apply for POST request
}REQUEST_INFOR;

void analyzeHTTPRequest(char* request, REQUEST_INFOR* result);
bool isBadRequest(REQUEST_INFOR request);
int getRequestMethod(REQUEST_INFOR request);
int firstIndexOf(char*, const char*);
int isMatchedKey(char*);
void smoothPath(char*);
#endif // !_handle_request

