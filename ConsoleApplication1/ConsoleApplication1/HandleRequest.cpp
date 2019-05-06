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
	memset(result->connection, 0, sizeof(result->connection));
	memset(result->range, 0, sizeof(result->range));

	sscanf_s(request, "%s %s %s", result->method, _countof(result->method),
								result->requestURI, _countof(result->requestURI),
								result->versionHTTP, _countof(result->versionHTTP));
	char* headerLine = request + strlen(result->method) + strlen(result->requestURI) + strlen(result->versionHTTP) + 4;
	int index;
	char buff[BUFF_SIZE];
	char key[BUFF_SIZE];
	char value[BUFF_SIZE];
	do {
		memset(buff, 0, BUFF_SIZE);
		memset(key, 0, BUFF_SIZE);
		memset(value, 0, BUFF_SIZE);
		index = firstIndexOf(headerLine, "\r\n");
		memcpy(buff, headerLine, index);
		int indexOfColon = firstIndexOf(buff, ":");
		memcpy(key, buff, indexOfColon);
		memcpy(value, buff + indexOfColon + 2, sizeof(buff) - indexOfColon - 1);
		int keyID = isMatchedKey(key);
		if (keyID == CONNECTION_FIELD_ID) {
			memcpy(result->connection, value, sizeof(value));
			result->flag[CONNECTION_FIELD_ID] = 1;
		}
		else if (keyID == RANGE_FIELD_ID) {
			memcpy(result->range, value, sizeof(value));
			result->flag[RANGE_FIELD_ID] = 1;
		}
		else if (keyID == CONTENTLENGTH_FIELD_ID) {
			result->contentLength = atoi(value);
			result->flag[CONTENTLENGTH_FIELD_ID] = 1;
		}
		headerLine += (index + 2);
	} while (index != 0);
}

bool isBadRequest(REQUEST_INFOR request){
	return true;
}

int getRequestMethod(REQUEST_INFOR request) {
	return 0;
}

int firstIndexOf(char* string, const char* pattern) {
	int index;
	for (index = 0; index < strlen(string); index++) {
		int i = 0;
		for (; i < strlen(pattern); i++) {
			if (string[index + i] != pattern[i])
				break;
		}
		if (i == strlen(pattern))
			break;
	}
	return index;
}

int isMatchedKey(char* key) {
	if (!strcmp(key, "Connection")) return 0;
	if (!strcmp(key, "Range")) return 1;
	if (!strcmp(key, "Content-Length")) return 2;
	return -1;
}