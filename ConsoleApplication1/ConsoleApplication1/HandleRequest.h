#pragma once

#ifndef _handle_request_
#define _handle_request_
#include "ConstValues.h"
#include <WinSock2.h>

typedef struct requestInfor 
{
	int flag[3] = { 0, 0, 0 }; // {connection, range, contentlength}
	char method[20];
	char requestURI[BUFF_SIZE];
	char versionHTTP[20];
	char range[BUFF_SIZE];
	char connection[BUFF_SIZE];
	char contentType[BUFF_SIZE];
	int contentLength = 0; // apply for POST request
}REQUEST_INFOR;

typedef struct responseInfor 
{
	char* versionHTTP;
	int statusCode;
	char* status;
	int contentLength;
	char* connection;
	char* contentType;
}RESPONSE_INFOR;

void analyzeHTTPRequest(char* request, REQUEST_INFOR* result, char** body);
bool isBadRequest(REQUEST_INFOR request);
int getRequestMethod(REQUEST_INFOR request);
int firstIndexOf(char*, const char*);
int isMatchedKey(char*);
void smoothPath(char*);
void createResponseDataForDirectory(REQUEST_INFOR, char*);
void createHeader(RESPONSE_INFOR, char*);
int sendMessage(SOCKET, char* , char*);
void toUpperCase(char**);
#endif // !_handle_request

