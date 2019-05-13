#pragma once

#ifndef _handle_request_
#define _handle_request_
#include "ConstValues.h"
#include <WinSock2.h>

typedef struct range
{
	int firstPos = -1;
	int endPos = -1;
	int suffixLength = -1;
	bool isSatisfiable = false;
}RANGE;

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

void analyzeHTTPRequest(char* request, REQUEST_INFOR* header, char** body);
bool isBadRequest(REQUEST_INFOR request);
int getRequestMethod(REQUEST_INFOR request);
int isMatchedKey(char* key);
void smoothPath(char* path);
void createResponseDataForDirectory(REQUEST_INFOR requestHeader, char* data);
void createHeader(char* header, char* versionHTTP, int statusCode, char* status, 
								char* connection, int contentLength, char* contentType, char* contentRange);
int sendMessage(SOCKET sock, char* header, char* data);
void toUpperCase(char** string);
void decodeMessageBody(char** body);
int isDirectory(const char* path);
bool isSupportedContentType(char* contentType);
bool isSatisfiableRangeHeader(RANGE* listRange, int* size, int fileSize);
int decodeRangeHeaderField(char* range, RANGE* listRange, int fileSize);
int createResponseDataForFile(REQUEST_INFOR requestHeader, char* data);
char* getContentType(char* path);
void getContentRange(char** contentRange, int firstPos, int endPos, int contentLength);
void closeConnectionWithError(SOCKET sock);
void createSubData(char** subData, char* data, int firstPos, int endPos);
#endif // !_handle_request

