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
	RESPONSE_INFOR responseHeaderInfor;
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
			printf("Can not recive from client !");
			printf("ErrorCode: %d", WSAGetLastError());
			closesocket(clientSock);
			return 0;
		}
		else {
			analyzeHTTPRequest(request, &requestHeader, &body);
			if (isBadRequest(requestHeader)) {

				responseHeaderInfor.versionHTTP = "HTTP/1.1";
				responseHeaderInfor.statusCode = 400;
				responseHeaderInfor.status = "Bad Request";
				responseHeaderInfor.connection = requestHeader.connection;
				responseHeaderInfor.contentType = "application/json";
				responseHeaderInfor.contentLength = 0;

				createHeader(responseHeaderInfor, header);

				int sendLen = sendMessage(clientSock, header, data);
				if (sendLen == SOCKET_ERROR) {
					printf("Can not send response message !");
					printf("ErrorCode: %d", WSAGetLastError());
					closesocket(clientSock);
					return 0;
				}

				if (!strcmp(requestHeader.connection, "close")) {
					isPersistentConnection = false;
				}
			}
			else {
				switch (getRequestMethod(requestHeader))
				{
				case REQUEST_GET:
					smoothPath(requestHeader.requestURI);
					if (strstr(requestHeader.requestURI, "_FILE") != NULL) {

						createResponseDataForDirectory(requestHeader, data);

						responseHeaderInfor.versionHTTP = "HTTP/1.1";
						responseHeaderInfor.statusCode = 200;
						responseHeaderInfor.status = "OK";
						responseHeaderInfor.connection = requestHeader.connection;
						responseHeaderInfor.contentType = "text/html";
						responseHeaderInfor.contentLength = strlen(data);
						
						createHeader(responseHeaderInfor, header);
						int sendLen = sendMessage(clientSock, header, data);

						if (sendLen == SOCKET_ERROR) {
							printf("Can not send response message !");
							printf("ErrorCode: %d", WSAGetLastError());
							closesocket(clientSock);
							return 0;
						}

						if (!strcmp(requestHeader.connection, "close")) {
							isPersistentConnection = false;
						}
					}
					else {
						if (requestHeader.flag[RANGE_FIELD_ID]) {
							RANGE listRange[MAX_NUMBER_OF_RANGE];
							int numRangeRequest = decodeRangeHeaderField(requestHeader.range, listRange);
							for (int i = 0; i < numRangeRequest; i++) {
								printf("%d %d %d\n", listRange[i].firstPos, listRange[i].endPos, listRange[i].suffixLength);
							}
						}
					}
					break;


				case REQUEST_POST:
					char fullPath[BUFF_SIZE];
					memset(fullPath, 0, BUFF_SIZE);
					sprintf_s(fullPath, BUFF_SIZE, "D:%s", requestHeader.requestURI);

					if (isDirectory(fullPath)) {
						sprintf_s(fullPath + strlen(fullPath), BUFF_SIZE - strlen(fullPath), "/unknown.txt");
					}

					FILE* file;

					responseHeaderInfor.versionHTTP = "HTTP/1.1";
					responseHeaderInfor.connection = requestHeader.connection;
					responseHeaderInfor.contentLength = 0;
					responseHeaderInfor.contentType = requestHeader.contentType;

					if (isSupportedContentType(requestHeader.contentType)) {
						if (!strcmp(requestHeader.contentType, "application/x-www-form-urlencoded")) 
							decodeMessageBody(&body);

						int err = fopen_s(&file, fullPath, "ab");
						if (err != OPEN_FILE_SUCCESSFULL) {
							responseHeaderInfor.statusCode = 500;
							responseHeaderInfor.status = "Internal Server Error";
						}
						else {
							fprintf_s(file, "%s\r\n", body);
							responseHeaderInfor.statusCode = 200;
							responseHeaderInfor.status = "OK";
						}
						fclose(file);
					}
					else {
						responseHeaderInfor.statusCode = 415;
						responseHeaderInfor.status = "Unsupported Media Type";
					}

					createHeader(responseHeaderInfor, header);

					int sendLen = sendMessage(clientSock, header, data);
					if (sendLen == SOCKET_ERROR) {
						printf("Can not send response message !");
						printf("ErrorCode: %d", WSAGetLastError());
						closesocket(clientSock);
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