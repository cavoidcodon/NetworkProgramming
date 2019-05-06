#include <WinSock2.h>
#include <stdio.h>
#include <conio.h>
#include "ConstValues.h"
#include <WS2tcpip.h>
#include "HandleRequest.h"

SOCKET* listClient = NULL;
HANDLE* listEvent = NULL;
int clientCount = 0;
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
		listClient = (SOCKET*)realloc(listClient, (clientCount + 1) * sizeof(SOCKET));
		listEvent = (HANDLE*)realloc(listEvent, (clientCount + 1) * sizeof(HANDLE));
		listClient[clientCount] = clientSock;
		listEvent[clientCount] = WSACreateEvent();
		WSAEventSelect(listClient[clientCount], listEvent[clientCount], FD_READ);
		if (isOverflow(clientCount)) {
			int startIndex = clientCount;
			HANDLE hdle = CreateThread(NULL, 0, serveClient, (LPVOID)&startIndex, 0, NULL);
		}
		clientCount++;
	}
}

DWORD WINAPI serveClient(LPVOID arg) {
	int startIndex = *(int*)arg;

	while (WORKING_STATE) {
		int endIndex = startIndex + WSA_MAXIMUM_WAIT_EVENTS - 1;
		int numbOfExaminedClient = (clientCount > (endIndex + 1)) ? (endIndex - startIndex + 1) : (clientCount - startIndex);
		int signaledEventIndex = WSAWaitForMultipleEvents(numbOfExaminedClient, listEvent + startIndex, FALSE, 1000, TRUE);
		if (signaledEventIndex == WSA_WAIT_FAILED || signaledEventIndex == WSA_WAIT_TIMEOUT) continue;
		int firstSignaledEventIndex = startIndex + signaledEventIndex - WSA_WAIT_EVENT_0;
		for (int i = firstSignaledEventIndex; i < clientCount && i <= endIndex; i++) {
			int unlockEventIndex = WSAWaitForMultipleEvents(1, listEvent + i, FALSE, 100, TRUE);
			if (unlockEventIndex == WSA_WAIT_FAILED || unlockEventIndex == WSA_WAIT_TIMEOUT) {
				//handle for locked events
			}
			else {
				char requestBuffer[BUFF_SIZE];
				REQUEST_INFOR requestHeader;
				memset(requestBuffer, 0, BUFF_SIZE);
				int recvLen = recv(listClient[i], requestBuffer, BUFF_SIZE - 1, 0);
				if (recvLen == SOCKET_ERROR) {
					printf("Can not receive request from client !");
					printf("Error code: %d", WSAGetLastError());
					continue;
				}
				else {
					analyzeHTTPRequest(requestBuffer, &requestHeader);
					if (isBadRequest(requestHeader)) {
						//hnadle for bad request
					}
					else {
						switch (getRequestMethod(requestHeader))
						{
						case REQUEST_GET:
							
							break;
						case REQUEST_POST:

							break;
						default:
							break;
						}
					}
				}
			}
		}
	}
	return 0;
}