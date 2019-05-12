#include <WinSock2.h>
#include <stdio.h>
#include <conio.h>
#include "ConstValues.h"
#include <WS2tcpip.h>
#include "HandleRequest.h"

DWORD WINAPI serveClient(LPVOID);

int main() {
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data)) {
		printf("Version is not supported !");
		_getch();
		return 0;
	}

	SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	if (!inet_pton(AF_INET, SERVER_ADDR, (PVOID)&serverAddr.sin_addr.S_un.S_addr)) {
		printf("Error! Invalid server address.\n");
		printf("Error Code: %d", WSAGetLastError());
		_getch();
		return 0;
	}

	if (bind(listenSock, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR))) {
		printf("Can not bind address %s \n", SERVER_ADDR);
		_getch();
		return 0;
	}
	
	if (listen(listenSock, SOMAXCONN)) {
		printf("Can not listen !\n");
		_getch();
		return 0;
	}
	printf("##### SERVER STARTED #####\n");

	while (WORKING_STATE) {
		SOCKADDR_IN clientAddr;
		int length = sizeof(SOCKADDR);
		SOCKET clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &length);
		HANDLE hdle = CreateThread(NULL, 0, serveClient, (LPVOID)clientSock, 0, NULL);
	}
	closesocket(listenSock);
	WSACleanup();
	return 0;
}

DWORD WINAPI serveClient(LPVOID arg) {

	SOCKET clientSock = (SOCKET)arg;
	REQUEST_INFOR requestHeader;
	char* request = (char*)calloc(BUFF_SIZE, sizeof(char));
	char* data = (char*)calloc(DATA_SIZE, sizeof(char));
	char* header = (char*)calloc(HEADER_SIZE, sizeof(char));
	char* body = (char*)calloc(BODY_SIZE, sizeof(char));
	bool isPersistentConnection = true;

	while (isPersistentConnection) {

		memset(request, 0, BUFF_SIZE);
		memset(data, 0, DATA_SIZE);
		memset(header, 0, HEADER_SIZE);
		memset(body, 0, BODY_SIZE);
		
		int recvLen = recv(clientSock, request, BUFF_SIZE - 1, 0);
		
		if (recvLen == SOCKET_ERROR) {
			closeConnectionWithError(clientSock);
			return 0;
		}
		else {
			analyzeHTTPRequest(request, &requestHeader, &body);

//// BAD REQUEST //////////////////////////////////////////////////////

			if (isBadRequest(requestHeader)) {
				createHeader(header, "HTTP/1.1", 400, "Bad Request", "close", 0, NULL);

				int sendLen = sendMessage(clientSock, header, data);
				if (sendLen == SOCKET_ERROR) {
					closeConnectionWithError(clientSock);
					return 0;
				}
				isPersistentConnection = false;
			}
//==========================================================================
// VALID REQUEST ////////////////////////////////////////////////////////
			else {
				switch (getRequestMethod(requestHeader))
				{
				//== HANDLE GET REQUEST =================================
				case REQUEST_GET:
					smoothPath(requestHeader.requestURI);

					//== Directory ==
					if (strstr(requestHeader.requestURI, "FILE_") == NULL) {

						createResponseDataForDirectory(requestHeader, data);
						createHeader(header, "HTTP/1.1", 200, "OK", requestHeader.connection, strlen(data), "text/html");

						int sendLen = sendMessage(clientSock, header, data);
						if (sendLen == SOCKET_ERROR) {
							closeConnectionWithError(clientSock);
							return 0;
						}

						isPersistentConnection = !strcmp(requestHeader.connection, "close") ? false : true;
					}

					//== File With Range Request ===
					else if (requestHeader.flag[RANGE_FIELD_ID]) {
						RANGE listRange[MAX_NUMBER_OF_RANGE];
						int numRangeRequest = decodeRangeHeaderField(requestHeader.range, listRange);

					}

					//== File Without Range Request ===
					else {
						int statusCode;
						char* status = NULL;

						int err = createResponseDataForFile(requestHeader, data);
						if (err == OPEN_FILE_SUCCESSFULL) {
							statusCode = 200;
							status = "OK";
						}
						else {
							statusCode = 500;
							status = "Internal Server Error";
						}

						char* contentType = getContentType(requestHeader.requestURI);
						createHeader(header, "HTTP/1.1", statusCode, status, requestHeader.connection, strlen(data), contentType);

						int sendLen = sendMessage(clientSock, header, data);
						if (sendLen == SOCKET_ERROR) {
							closeConnectionWithError(clientSock);
							return 0;
						}

						isPersistentConnection = !strcmp(requestHeader.connection, "close") ? false : true;
					}

					break;

				//== HANLDE POST REQUEST ================================
				case REQUEST_POST:
					char fullPath[BUFF_SIZE];
					memset(fullPath, 0, BUFF_SIZE);
					sprintf_s(fullPath, BUFF_SIZE, "D:%s", requestHeader.requestURI);

					if (isDirectory(fullPath)) {
						sprintf_s(fullPath + strlen(fullPath), BUFF_SIZE - strlen(fullPath), "/unknown.txt");
					}

					FILE* file;
					int statusCode;
					char* status;

					if (isSupportedContentType(requestHeader.contentType)) {
						if (!strcmp(requestHeader.contentType, "application/x-www-form-urlencoded")) 
							decodeMessageBody(&body);

						int err = fopen_s(&file, fullPath, "ab");
						if (err != OPEN_FILE_SUCCESSFULL) {
							statusCode = 500;
							status = "Internal Server Error";
						}
						else {
							fprintf_s(file, "%s\r\n", body);
							statusCode = 200;
							status = "OK";
						}
						fclose(file);
					}
					else {
						statusCode = 415;
						status = "Unsupported Media Type";
					}

					createHeader(header, "HTTP/1.1", statusCode, status, "close", 0, NULL);

					int sendLen = sendMessage(clientSock, header, data);
					if (sendLen == SOCKET_ERROR) {
						closeConnectionWithError(clientSock);
						return 0;
					}

					isPersistentConnection = false;
					break;
				}
			}
		}
	}

	free(data);
	free(header);
	closesocket(clientSock);
	return 0;
}