#pragma comment(lib, "Ws2_32.lib")
#include "Winsock2.h"
#include <tchar.h>
#include "NTSecApi.h"
#include "iostream"
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
	return TRUE;
}



struct SOCK_ITEM : OVERLAPPED
{
	SOCKET	_sock;
	char	_buff[512];

	SOCK_ITEM(SOCKET sock)
	{
		memset(this, 0, sizeof(*this));
		_sock = sock;
	}
};
typedef SOCK_ITEM* PSOCK_ITEM;


#define MAX_CLI_CNT	63
void _tmain()
{
	WSADATA	wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd))
	{
		cout << "WSAStartup failed..." << endl;
		return;
	}
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		cout << "SetConsoleCtrlHandler failed, code : " << GetLastError() << endl;
		return;
	}
	g_dwTheadId = GetCurrentThreadId();

	SOCKET hsoListen = GetListenSocket(9001, MAX_CLI_CNT);
	if (hsoListen == INVALID_SOCKET)
	{
		WSACleanup();
		return;
	}
	cout << " ==> Waiting for client's connection......" << endl;

	WSAEVENT hevListen = WSACreateEvent();
	WSAEventSelect(hsoListen, hevListen, FD_ACCEPT);

	int			nSockCnt = 0;
	SOCKET		arSocks[MAX_CLI_CNT + 1];
	PSOCK_ITEM	arItems[MAX_CLI_CNT + 1];
	memset(arSocks, 0xFF, (MAX_CLI_CNT + 1) * sizeof(SOCKET));
	memset(arItems, 0, (MAX_CLI_CNT + 1) * sizeof(PSOCK_ITEM));
	arSocks[0] = (SOCKET)hevListen;
	/*
	
	이벤트 핸들을 위한 배열이 아니라 소켓 핸들을 담을크기가 64인 배열을 준비하고, 
	그 배열의 첫 번째는 리슨 소켓을 위한 이벤트 핸들을 지정하고 나머지 63개의 공간은 접속할 클라이언트의 자식 소켓 핸들 자체를 지정하도록 한다. 
	그리고 PSOCK_ITEM을 위한 크기 64인 배열을 준비해 인덱스 1부터 63개의 공간을 자식 소켓의 비동기 수신 처리를 위해 예약한다.
	*/
	int nSockRet = 0;
	while (true)
	{
		DWORD dwWaitRet = MsgWaitForMultipleObjects
		(
			nSockCnt + 1, (PHANDLE)arSocks, FALSE, INFINITE, QS_ALLPOSTMESSAGE
			
		);
		if (dwWaitRet == WAIT_FAILED)
		{
			cout << "WaitForMultipleObjectsEx failed : " << GetLastError() << endl;
			break;
		}

		if (dwWaitRet == WAIT_OBJECT_0 + nSockCnt + 1)
			break;

		DWORD		dwFlags = 0;
		LONG		nErrCode = ERROR_SUCCESS;
		PSOCK_ITEM	pSI = NULL;
		if (dwWaitRet == WAIT_OBJECT_0)//리슨 소켓의 상태 변화가 발생했다.
		{
			WSANETWORKEVENTS ne;
			WSAEnumNetworkEvents(hsoListen, hevListen, &ne);
			if (ne.lNetworkEvents & FD_ACCEPT)
			{
				nErrCode = ne.iErrorCode[FD_ACCEPT_BIT];
				if (nErrCode != 0)
				{
					cout << " ==> Error occurred, code = " << nErrCode << endl;
					break;
				}

				SOCKET sock = accept(hsoListen, NULL, NULL);
				if (sock == INVALID_SOCKET)
				{
					nErrCode = WSAGetLastError();
					if (nErrCode != WSAEINTR)
						/*
						WSAEINTR 에러 코드는 리슨 소켓에 대해 closesocket을 호출했음을 의미하므로, 
						에러가 아닌 종료를 위한 처리기 때문에 에러 발생 메시지 출력 없이 루프를 빠져나간다
						*/
						cout << " ==> Error occurred, code = " << nErrCode << endl;
					break;
				}

				pSI = new SOCK_ITEM(sock);
				arSocks[nSockCnt + 1] = sock;
				arItems[nSockCnt + 1] = pSI;
				nSockCnt++;
				cout << " ==> New client " << sock << " connected" << endl;
			}
		}
		else //자식 소켓들 중 하나를 통해 데이터 수신이 완료되었다
		{
			int nIndex = (int)(dwWaitRet - WAIT_OBJECT_0);
			SOCKET sock = arSocks[nIndex];
			pSI = arItems[nIndex];

			DWORD dwTrBytes = 0;
			if (!WSAGetOverlappedResult(sock, pSI, &dwTrBytes, FALSE, &dwFlags))
			{ //정확한 에러 코드를 획득하기 위해 fWait를 FALSE로 지정해 WSAGetOverlappedResult를 호출한다.
				nErrCode = WSAGetLastError();
				goto $LABEL_CLOSE;
			}
			else
			{
				if (dwTrBytes == 0)//클라이언트가 closesocket을 호출하면 소켓 종료 처리를 위해 $LABEL_CLOSE 루틴으로 점프한다.
					goto $LABEL_CLOSE;

				pSI->_buff[dwTrBytes] = 0;
				cout << " *** Client(" << pSI->_sock << ") sent : " << pSI->_buff << endl;

				int lSockRet = send(pSI->_sock, pSI->_buff, dwTrBytes, 0);
				if (lSockRet == SOCKET_ERROR)
				{
					nErrCode = WSAGetLastError();
					goto $LABEL_CLOSE;
				}
			}
		}

		WSABUF wb;
		wb.buf = pSI->_buff, wb.len = sizeof(pSI->_buff);
		nSockRet = WSARecv(pSI->_sock, &wb, 1, NULL, &dwFlags, pSI, NULL);
		if (nSockRet == SOCKET_ERROR)
		{
			nErrCode = WSAGetLastError();
			if (nErrCode != WSA_IO_PENDING)
				goto $LABEL_CLOSE;
		}
		continue;

	$LABEL_CLOSE:
		int nIndex = (int)(dwWaitRet - WAIT_OBJECT_0);
		for (int i = nIndex; i < nSockCnt; i++)
		{
			arSocks[i] = arSocks[i + 1];
			arItems[i] = arItems[i + 1];
		}
		arSocks[nSockCnt] = INVALID_SOCKET;
		arItems[nSockCnt] = NULL;
		nSockCnt--;

		if (nErrCode != WSAECONNABORTED)
		{
			/*
			서버 측에서 closesocket을 통해 자식 소켓을 닫았을 경우는 의도적인 행위이므로, 
			이 소켓에 대해 closesocket을 호출해준다던지 하는 별도의 처리는 필요 없다.
			*/
			if (nErrCode == ERROR_SUCCESS || nErrCode == WSAECONNRESET)
				//클라이언트가 closesocket을 호출했거나 클라이언트 강제 종료인 경우는 연결 해제가 되었다는 메시지를 출력한다.
				cout << " ==> Client " << pSI->_sock << " disconnected..." << endl;
			else//이외의 경우는 알 수 없는 에러이므로, 에러 코드를 출력한다.
				cout << " ==> Error occurred, code = " << nErrCode << endl;
			closesocket(pSI->_sock);
			delete pSI;
		}
	}

	if (hsoListen != INVALID_SOCKET)
		closesocket(hsoListen);
	WSACloseEvent(hevListen);
	for (int i = 1; i < nSockCnt + 1; i++)
	{
		if (arSocks[i] != INVALID_SOCKET)
			closesocket(arSocks[i]);
		if (arItems[i] != NULL)
			delete arItems[i];
	}

	cout << "==== Server terminates... ==========================" << endl;

	WSACleanup();
}
