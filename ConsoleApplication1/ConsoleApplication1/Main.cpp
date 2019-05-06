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
	char request[BUFF_SIZE];
	memset(request, 0, BUFF_SIZE);
	bool isPersistentConnection = true;
	while (isPersistentConnection) {
		int recvLen = recv(clientSock, request, BUFF_SIZE - 1, 0);
		if (recvLen == SOCKET_ERROR) {
			printf("Can not recive from client !");
			printf("ErrorCode: %d", WSAGetLastError());
			closesocket(clientSock);
			return 0;
		}
		else {
			analyzeHTTPRequest(request, &requestHeader);
			if (isBadRequest(requestHeader)) {
				//handle for bad request
			}
			else {
				switch (getRequestMethod(requestHeader))
				{
				case REQUEST_GET:
					smoothPath(requestHeader.requestURI);
					if (strstr(requestHeader.requestURI, "_FILE") == NULL) {
						//handle for directory
					}
					else {
						//handle for file
					}
					break;
				case REQUEST_POST:
					//handle for POST request
					break;
				}
			}
		}
	}
	closesocket(clientSock);
	return 0;
}