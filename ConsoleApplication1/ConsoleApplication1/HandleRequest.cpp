#include <stdio.h>
#include "HandleRequest.h"
#include "ConstValues.h"
#include <WinSock2.h>
#include <sys/types.h>


/*bool analyzeHTTPRequest(char* request, REQUEST_INFOR* result);
	parameter : char* request[IN] -> stored request header from client
				REQUEST_INFOR* result[OUT] -> pointer to structure will be contained 
											  information about header request (method, request-URI, version...)
*/

void analyzeHTTPRequest(char* request, REQUEST_INFOR* result, char** body) {

	memset(result->method, 0, sizeof(result->method));
	memset(result->requestURI, 0, sizeof(result->requestURI));
	memset(result->versionHTTP, 0, sizeof(result->versionHTTP));
	memset(result->connection, 0, sizeof(result->connection));
	memset(result->range, 0, sizeof(result->range));
	memset(result->contentType, 0, sizeof(result->contentType));

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
		else if (keyID == CONTENTTYPE_FILED_ID) {
			memcpy(result->contentType, value, sizeof(value));
		}

		headerLine += (index + 2);

	} while (index != 0);
	*body = headerLine;
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

/*
	return request's method code
*/
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

/*
	return the first of 'pattern' in 'string'
*/
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
	toUpperCase(&key);
	if (!strcmp(key, "CONNECTION")) return 0;
	if (!strcmp(key, "RANGE")) return 1;
	if (!strcmp(key, "CONTENT-LENGTH")) return 2;
	if (!strcmp(key, "CONTENT-TYPE")) return 3;
	return -1;
}

/*
	replace all character '%20' [%'ASSCII_CODE'] in path by ' '
*/
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

	sprintf_s(full_path, BUFF_SIZE, "D:%s*.*", request.requestURI);
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

void toUpperCase(char** string) {
	for (unsigned int i = 0; i < strlen(*string); i++) {
		(*string)[i] = toupper((*string)[i]);
	}
}

void decodeMessageBody(char** body) {
	char* firstMath = strstr(*body, "&");
	while (firstMath != NULL){
		*firstMath = '\t';
		firstMath = strstr(*body, "&");
	}

	firstMath = strstr(*body, "=");
	while (firstMath != NULL)
	{
		*firstMath = ':';
		firstMath = strstr(*body, "=");
	}
}

int isDirectory(const char* path) {

	DWORD fileAttributes = GetFileAttributesA(path);
	return fileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

bool isSupportedContentType(char* contentType) {

	return (!strcmp(contentType, "application/x-www-form-urlencoded")) ||
		(!strcmp(contentType, "text/plain"));
}

RANGE parseRange(char* str) {
	char first[20];
	char end[20];
	memset(first, 0, 20);
	memset(end, 0, 20);

	char* seperator = strstr(str, "-");
	int distance = seperator - str;
	memcpy(first, str, distance);
	memcpy(end, str + distance + 1, strlen(str) - distance - 1);

	RANGE result;
	if (strlen(first) != 0 && strlen(end) !=0) {
		result.suffixLength = 0;
		result.firstPos = atoi(first);
		result.endPos = atoi(end);
	}
	else if (strlen(first) != 0) {
		result.firstPos = atoi(first);
		result.endPos = MAX_SIZE;
	}
	else if (strlen(end) != 0) {
		result.suffixLength = atoi(end);
	}

	return result;
}

int decodeRangeHeaderField(char* rangeField, RANGE* result) {
	int count = 0;
	char* startPoint = strstr(rangeField, "=") + 1;
	char* delimeter = strstr(startPoint, ",");
	char range[BUFF_SIZE];
	while (delimeter != NULL) {
		memset(range, 0, BUFF_SIZE);
		int size = delimeter - startPoint;
		memcpy(range, startPoint, size);
		result[count] = parseRange(range);
		count++;
		startPoint += (size + 2);
		delimeter = strstr(startPoint, ",");
	}
	result[count++] = parseRange(startPoint);

	return count;
}