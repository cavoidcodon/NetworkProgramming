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
	if (strcmp(request.method, "GET") &&
		strcmp(request.method, "POST") &&
		strcmp(request.method, "HEAD") &&
		strcmp(request.method, "OPTIONS") &&
		strcmp(request.method, "PUT") &&
		strcmp(request.method, "DELETE") &&
		strcmp(request.method, "TRACE") &&
		strcmp(request.method, "CONNECT"))
		return true;
	if (strcmp(request.versionHTTP, "HTTP/1.1") &&
		strcmp(request.versionHTTP, "HTTP/1.0"))
		return true;
	return false;
}

int getRequestMethod(REQUEST_INFOR request) {
	if (!strcmp(request.method, "GET")) return REQUEST_GET;
	else if (!strcmp(request.method, "POST")) return REQUEST_POST;
	else if (!strcmp(request.method, "HEAD")) return REQUEST_HEAD;
	else if (!strcmp(request.method, "PUT")) return REQUEST_PUT;
	else if (!strcmp(request.method, "DELETE")) return REQUEST_DELETE;
	else if (!strcmp(request.method, "OPTIONS")) return REQUEST_OPTIONS;
	else if (!strcmp(request.method, "CONNECT")) return REQUEST_CONNECT;
	else return REQUEST_TRACE;
}

int firstIndexOf(char* string, const char* pattern) {
	unsigned int index;
	for (index = 0; index < strlen(string); index++) {
		unsigned int i = 0;
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

void smoothPath(char* path) {
	while (strstr(path, "%20") != NULL) {
		char* space = strstr(path, "%20");
		int offset = space - path;
		path[offset] = ' ';
		offset++;
		for (unsigned int i = 3; i<strlen(space); i++, offset++) {
			path[offset] = space[i];
		}
		path[offset] = 0;
	}
}

void createResponseDataForDirectory(REQUEST_INFOR request, char* data) {
	sprintf_s(data, DATA_SIZE, "<html><H>DIRECTORY</H><br>");
	WIN32_FIND_DATAA FDATA;
	char full_path[BUFF_SIZE];
	memset(full_path, 0, BUFF_SIZE);
	sprintf_s(full_path, BUFF_SIZE, "C:%s*.*", request.requestURI);
	HANDLE hFind = FindFirstFileA(full_path, &FDATA);
	do {
		if (FDATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			sprintf_s(data + strlen(data), DATA_SIZE - strlen(data),
				"<a href=\"%s%s/\">%s</a><br>", request.requestURI, FDATA.cFileName, FDATA.cFileName);
		}
		else {
			sprintf_s(data + strlen(data), DATA_SIZE - strlen(data),
				"<b><a href=\"FILE_%s%s/\">%s</a></b><br>", request.requestURI, FDATA.cFileName, FDATA.cFileName);
		}
	} while (FindNextFileA(hFind, &FDATA));
	sprintf_s(data + strlen(data), DATA_SIZE - strlen(data), "</html>");
}

void createHeader(RESPONSE_INFOR infor, char* header) {
	sprintf_s(header, HEADER_SIZE, "%s %d %s\r\n"
								"Connection: %s\r\n"
								"Content-Type: %s\r\n", infor.versionHTTP, infor.statusCode, infor.status,
														infor.connection,
														infor.contentType);

	if (infor.contentLength != 0) {
		sprintf_s(header + strlen(header), HEADER_SIZE - strlen(header), "Content-Length: %d\r\n", infor.contentLength);
	}
}
 
int sendMessage(SOCKET clientSock, char* header, char* data) {
	const long SIZE = HEADER_SIZE + DATA_SIZE;
	char* message = (char*)calloc(SIZE, sizeof(char));
	memset(message, 0, SIZE);
	sprintf_s(message, SIZE, "%s\r\n%s", header, data);
	int sendLen = send(clientSock, message, strlen(message), 0);
	return sendLen;
}