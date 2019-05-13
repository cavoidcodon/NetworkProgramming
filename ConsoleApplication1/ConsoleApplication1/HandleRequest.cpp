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

	while(0 == 0) {
		memset(buff, 0, BUFF_SIZE);
		memset(key, 0, BUFF_SIZE);
		memset(value, 0, BUFF_SIZE);

		index = strstr(headerLine, "\r\n") - headerLine;
		memcpy(buff, headerLine, index);
		headerLine += (index + 2);

		if (index == 0) break;

		int indexOfColon = strstr(buff, ":") - buff;
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
	}
	*body = headerLine;
}

//=========================================================================//

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

//=========================================================================//

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

//=========================================================================//

int isMatchedKey(char* key) {
	toUpperCase(&key);
	if (!strcmp(key, "CONNECTION")) return CONNECTION_FIELD_ID;
	if (!strcmp(key, "RANGE")) return RANGE_FIELD_ID;
	if (!strcmp(key, "CONTENT-LENGTH")) return CONTENTLENGTH_FIELD_ID;
	if (!strcmp(key, "CONTENT-TYPE")) return CONTENTTYPE_FILED_ID;
	return -1;
}

//=========================================================================//

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

//=========================================================================//

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

//=========================================================================//

void createHeader
	(char* header, char* versionHTTP, int statusCode, char* status, 
				   char* connection, int contentLength, char* contentType, char* contentRange) 
{
	sprintf_s(header, HEADER_SIZE, "%s %d %s\r\n"
								   "Connection: %s\r\n", versionHTTP, statusCode, status,
														connection);
	if (contentType != NULL) {
		sprintf_s(header + strlen(header), HEADER_SIZE - strlen(header), "Content-Type: %s\r\n", contentType);
	}

	if (contentLength != 0) {
		sprintf_s(header + strlen(header), HEADER_SIZE - strlen(header), "Content-Length: %d\r\n", contentLength);
	}

	if (contentRange != NULL) {
		sprintf_s(header + strlen(header), HEADER_SIZE - strlen(header), "Content-Range: %s\r\n", contentRange);
	}
}

//=========================================================================//
 
int sendMessage(SOCKET clientSock, char* header, char* data) {

	const long SIZE = HEADER_SIZE + DATA_SIZE;
	char* message = (char*)calloc(SIZE, sizeof(char));

	memset(message, 0, SIZE);

	sprintf_s(message, SIZE, "%s\r\n%s", header, data);

	int sendLen = send(clientSock, message, strlen(message), 0);

	return sendLen;
}

//=========================================================================//

void toUpperCase(char** string) {
	for (unsigned int i = 0; i < strlen(*string); i++) {
		(*string)[i] = toupper((*string)[i]);
	}
}

//=========================================================================//

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

//=========================================================================//

int isDirectory(const char* path) {

	DWORD fileAttributes = GetFileAttributesA(path);
	return fileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

//=========================================================================//

bool isSupportedContentType(char* contentType) {

	return (!strcmp(contentType, "application/x-www-form-urlencoded")) ||
		(!strcmp(contentType, "text/plain"));
}

//=========================================================================//

RANGE parseRange(char* str, int fileSize) {
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
		result.firstPos = atoi(first);
		result.endPos = atoi(end) >= fileSize ? fileSize - 1 : atoi(end);
	}
	else if (strlen(first) != 0) {
		result.firstPos = atoi(first);
		result.endPos = fileSize - 1;
	}
	else if (strlen(end) != 0) {
		result.suffixLength = atoi(end);
		result.firstPos = fileSize - result.suffixLength;
		result.endPos = fileSize - 1;
	}

	return result;
}

//=========================================================================//

int decodeRangeHeaderField(char* rangeField, RANGE* result, int fileSize) {
	int count = 0;
	char* startPoint = strstr(rangeField, "=") + 1;
	char* delimeter = strstr(startPoint, ",");
	char range[BUFF_SIZE];
	while (delimeter != NULL) {
		memset(range, 0, BUFF_SIZE);
		int size = delimeter - startPoint;
		memcpy(range, startPoint, size);
		result[count] = parseRange(range, fileSize);
		count++;
		startPoint += (size + 2);
		delimeter = strstr(startPoint, ",");
	}
	result[count++] = parseRange(startPoint, fileSize);

	return count;
}

//=========================================================================//

bool isSatisfiableRangeHeader(RANGE* list, int* numb, int fileSize) {
	bool flag = false;
	int cout = 0;
	for (int i = 0; i < *numb; i++) {
		if ((list[i].suffixLength > 0) ||
			((list[i].suffixLength == -1) && (list[i].firstPos < fileSize) && (list[i].firstPos <= list[i].endPos))) {
			list[i].isSatisfiable = true;
			flag = true;
			cout++;
		}
	}

	if (cout > MAX_NUMBER_OF_RANGE) flag = false;

	*numb = cout;

	return flag;
}

//=========================================================================//

int createResponseDataForFile(REQUEST_INFOR request, char* data) {
	char* full_path = (char*)calloc(BUFF_SIZE, sizeof(CHAR));
	memset(full_path, 0, BUFF_SIZE);

	char* sub_path = strstr(request.requestURI, "FILE_");
	sprintf_s(full_path, BUFF_SIZE, "D:%s", sub_path + 5);
	full_path[strlen(full_path) - 1] = 0;

	FILE* file;
	long file_size;

	int err = fopen_s(&file, full_path, "rb");
	if (err == 0) {
		fseek(file, 0, SEEK_END);
		file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		fread(data, 1, file_size, file);
		fclose(file);
	}
	return err;
}

//=========================================================================//

void closeConnectionWithError(SOCKET sock) {
	printf("ErrorCode: %d", WSAGetLastError());
	closesocket(sock);
}

//=========================================================================//

char* getContentType(char* path) {
	char* extension = strstr(path, ".");
	char* result = NULL;

	if (!strcmp(extension, ".c/") ||
		!strcmp(extension, ".cpp/") ||
		!strcmp(extension, ".bas/") ||
		!strcmp(extension, ".h/") ||
		!strcmp(extension, ".txt/")) result = "text/plain";

	else if (!strcmp(extension, ".css/")) result = "text/css";

	else if (!strcmp(extension, ".htm/") ||
			!strcmp(extension, ".html/") ||
			!strcmp(extension, ".stm/")) result = "text/html";
	else result = "application/octet-stream";  // default content-type (according RFC)

	return result;
}

//=========================================================================//

void getContentRange(char** contentRange, int firstPos, int endPos, int contentLength) {
	*contentRange = (char*)calloc(50, sizeof(char));
	memset(*contentRange, 0, 50);
	if (firstPos == -1 &&
		endPos == -1) {
		sprintf_s(*contentRange, 50, "bytes */%d", contentLength);
	}
	else {
		sprintf_s(*contentRange, 50, "bytes %d-%d/%d", firstPos,
			endPos,
			contentLength);
	}
}

//=========================================================================//

void createSubData(char** rData, char* data, int firstPos, int endPos) {
	*rData = (char*)calloc(DATA_SIZE, sizeof(char));
	memset(*rData, 0, DATA_SIZE);

	memcpy(*rData, data + firstPos,
		endPos - firstPos + 1);
}

//=========================================================================//