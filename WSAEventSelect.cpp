#pragma comment(lib, "Ws2_32.lib")
#include <Winsock2.h>
#include <iostream>
#include<tchar.h>
using namespace std;

SOCKET GetListenSocket(short shPortNo, int nBacklog = SOMAXCONN)
{
	SOCKET hsoListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hsoListen == INVALID_SOCKET)
	{
		cout << "socket failed, code : " << WSAGetLastError() << endl;
		return INVALID_SOCKET;
	}

	SOCKADDR_IN	sa;
	memset(&sa, 0, sizeof(SOCKADDR_IN));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(shPortNo);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	LONG lSockRet = bind(hsoListen, (PSOCKADDR)&sa, sizeof(SOCKADDR_IN));
	if (lSockRet == SOCKET_ERROR)
	{
		cout << "bind failed, code : " << WSAGetLastError() << endl;
		closesocket(hsoListen);
		return INVALID_SOCKET;
	}

	lSockRet = listen(hsoListen, nBacklog);
	if (lSockRet == SOCKET_ERROR)
	{
		cout << "listen failed, code : " << WSAGetLastError() << endl;
		closesocket(hsoListen);
		return INVALID_SOCKET;
	}

	return hsoListen;
}

DWORD g_dwTheadId = 0;
BOOL CtrlHandler(DWORD fdwCtrlType)
{
	PostThreadMessage(g_dwTheadId, WM_QUIT, 0, 0);
	printf("g_dwTheadId = %d", g_dwTheadId);
	return TRUE;
}


#define MAX_CLI_CNT	63 //접속 가능한 최대 클라이언트 수를 정의한다. 
void _tmain()
{
	WSADATA	wsd;
	int nErrCode = WSAStartup(MAKEWORD(2, 2), &wsd);
	if (nErrCode)
	{
		cout << "WSAStartup failed with error : " << nErrCode << endl;
		return;
	}
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		cout << "SetConsoleCtrlHandler failed, code : " << GetLastError() << endl;
		return;
	}
	g_dwTheadId = GetCurrentThreadId();

	SOCKET hsoListen = GetListenSocket(9000, MAX_CLI_CNT);
	if (hsoListen == INVALID_SOCKET)
	{
		WSACleanup();
		return;
	}
	cout << " ==> Waiting for client's connection......" << endl;

	WSAEVENT hEvent = WSACreateEvent();
	WSAEventSelect(hsoListen, hEvent, FD_ACCEPT);//이벤트를 생성하고 WSAEventSelect를 호출해 리슨 소켓과 연결한다. FD_ACCEPT를 지정한다.

	int		 nSockCnt = 0;
	WSAEVENT arEvents[MAX_CLI_CNT + 1];
	SOCKET	 arSocks[MAX_CLI_CNT + 1];
	/*
	배열의 크기는 64이다.  WaitForMultipleObjects의 수용할 수 있는 최대 크기가 64이기 때문이다.
	수용 가능한 클라이언트 접속은 63개고, 나머지 하나는 리슨 소켓용
	*/
	memset(arEvents, 0, (MAX_CLI_CNT + 1) * sizeof(HANDLE));
	memset(arSocks, 0xFF, (MAX_CLI_CNT + 1) * sizeof(SOCKET));
	//이벤트 핸들을 담을 배열과 소켓 핸들을 담을 배열을 선언하고 초기화한다.
	arEvents[0] = hEvent;
	arSocks[0] = hsoListen;
	//각 배열의 첫번째 원소는 리슨 소켓과 이에 대한 이벤트 핸들을 설정한다.

	while (true)
	{
		DWORD dwWaitRet = MsgWaitForMultipleObjects
		(
			nSockCnt + 1, arEvents, FALSE, INFINITE, QS_POSTMESSAGE
		);//대기 상태로 들어가 소켓들의 상태 변경을 기다린다. 종료 처리를 위해 메시지 전송에 대해서도 대기한다.
		if (dwWaitRet == WAIT_FAILED)
		{
			cout << "MsgWaitForMultipleObjects failed: " << GetLastError() << endl;
			break;
		}
		if (dwWaitRet == WAIT_OBJECT_0 + nSockCnt + 1)
		{
			printf("nSockCnt + 1 = %d", WAIT_OBJECT_0 + nSockCnt + 1);
			printf("ctrl\n");
			break;
			/*
			소켓에 의한 종료가 아닌 PostThreadMessage(g_dwTheadId, WM_QUIT, 0, 0) 함수에 의한 메세지가 큐잉이 되면
			MsgWaitForMultipleObjects 는 현재 접속중인 소켓의 수보다 + 1 높은수가 리턴된다.			
			그럴경우는 그냥 종료 한다.
			*/

		}


		int nIndex = (int)(dwWaitRet - WAIT_OBJECT_0);
		SOCKET sock = arSocks[nIndex];
		//리턴값에 따르는 배열상의 인덱스를 통해서 시그널된 소켓을 획득한다.
		int nErrCode = 0;
		WSANETWORKEVENTS ne;
		WSAEnumNetworkEvents(sock, arEvents[nIndex], &ne);
		//해당 소켓의 상태 변경사항을 획득한다.

		if (ne.lNetworkEvents & FD_ACCEPT)
		{
			nErrCode = ne.iErrorCode[FD_ACCEPT_BIT];
			if (nErrCode != 0)
			{
				cout << " ==> Error occurred, code = " << nErrCode << endl;
				break;
			}

			SOCKET hsoChild = accept(sock, NULL, NULL);
			if (hsoChild == INVALID_SOCKET)
			{
				cout << "accept failed, code : " << WSAGetLastError() << endl;
				break;
			}

			hEvent = WSACreateEvent();
			WSAEventSelect(hsoChild, hEvent, FD_READ | FD_CLOSE);
			// 물론 자식 소켓에 대해서 관심을 갖는 상태 변화는 데이터 수신과 클라이언트가 접속을 끊었을 때이므로, 따라서 FD_READ | FD_CLOSE 옵션을 지정해줘야 한다.

			arEvents[nSockCnt + 1] = hEvent;
			arSocks[nSockCnt + 1] = hsoChild;
			//0번은 리슨 소켓과 그에 해당하는 event이기 때문에 +1 씩 해준다.
			nSockCnt++;
			//수용한 자식 소켓의 상태 변경을 통지받기 위해 관리 배열에 소켓과 이벤트 핸들을 설정한다.
			cout << " ==> New client " << hsoChild << " connected" << endl;
			continue;
		}

		if (ne.lNetworkEvents & FD_READ)
		{
			nErrCode = ne.iErrorCode[FD_READ_BIT];
			if (nErrCode != 0)
				goto $LABEL_CLOSE;

			char szBuff[512];
			int lSockRet = recv(sock, szBuff, sizeof(szBuff), 0);
			if (lSockRet == SOCKET_ERROR)
			{
				nErrCode = WSAGetLastError();
				goto $LABEL_CLOSE;
			}

			szBuff[lSockRet] = 0;
			cout << " *** Client(" << sock << ") sent : " << szBuff << endl;
			Sleep(10);

			lSockRet = send(sock, szBuff, lSockRet, 0);
			if (lSockRet == SOCKET_ERROR)
			{
				nErrCode = WSAGetLastError();
				goto $LABEL_CLOSE;
			}
		}

		if (ne.lNetworkEvents & FD_CLOSE)
		{
			nErrCode = ne.iErrorCode[FD_CLOSE_BIT];
			if (nErrCode != 0)
			{
				if (nErrCode == WSAECONNABORTED)
					nErrCode = 0;
			}
			goto $LABEL_CLOSE;
		}
		continue;

	$LABEL_CLOSE:
		closesocket(sock);
		CloseHandle(arEvents[nIndex]);

		for (int i = nIndex; i < nSockCnt; i++)
		{
			arEvents[i] = arEvents[i + 1];
			arSocks[i] = arSocks[i + 1];
		}
		arEvents[nSockCnt] = NULL;
		arSocks[nSockCnt] = INVALID_SOCKET;
		nSockCnt--;
		if (nErrCode == 0)
			cout << " ==> Client " << sock << " disconnected..." << endl;
		else
			cout << " ==> Error occurred, code = " << nErrCode << endl;
	}

	for (int i = 0; i < nSockCnt + 1; i++)
	{
		if (arEvents[i] != NULL)
			CloseHandle(arEvents[i]);
		if (arSocks[i] != INVALID_SOCKET)
			closesocket(arSocks[i]);
	}

	cout << "socket closed, program terminates..." << endl;
	WSACleanup();
}