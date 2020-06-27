#pragma comment(lib, "Ws2_32.lib")
#include "Winsock2.h"
#include "list"
#include "iostream"
using namespace std;


/*
TrySubmitThreadpoolCallback 함수는 특정 작업을 위해 계속 실행되어야 하는 전용 스레드를 대체할 목적으로 사용 가능하다. 
이 함수는 비록 TP_WORK 객체 카테고리에서 설명되고 있지만,
사실은 어떤 카테고리에도 종속되지 않기 때문에 TP_XXX 객체 없이 단순히 지정된 특정 스레드풀에서 
콜백 함수가 실행되도록 만들 수 있는 유용성을 갖고 있다
*/
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


SOCKET g_sockMain = INVALID_SOCKET;
BOOL CtrlHandler(DWORD fdwCtrlType)
{
	if (g_sockMain != INVALID_SOCKET)
		closesocket(g_sockMain);
	return TRUE;
}


VOID WINAPI ChildSockProc(PTP_CALLBACK_INSTANCE pinst, PVOID pctx)
{
	SOCKET sock = (SOCKET)pctx;
	char szBuff[512];

	while (true)
	{
		int lSockRet = recv(sock, szBuff, sizeof(szBuff), 0);
		if (lSockRet == SOCKET_ERROR)
		{
			cout << "recv failed : " << WSAGetLastError() << endl;
			break;
		}
		if (lSockRet == 0)
		{
			cout << " ==> Client " << sock << " disconnected..." << endl;
			break;
		}

		szBuff[lSockRet] = 0;
		cout << " *** Client(" << sock << ") sent : " << szBuff << endl;
		Sleep(10);

		lSockRet = send(sock, szBuff, lSockRet, 0);
		if (lSockRet == SOCKET_ERROR)
		{
			cout << "send failed : " << WSAGetLastError() << endl;
			break;
		}
	}
	closesocket(sock);
}


typedef std::list<SOCKET> WORK_LIST;
void main()
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

	SOCKET hsoListen = GetListenSocket(9000);
	if (hsoListen == INVALID_SOCKET)
	{
		WSACleanup();
		return;
	}
	g_sockMain = hsoListen;
	cout << " ==> Waiting for client's connection......" << endl;

	WORK_LIST works;
	while (true)
	{
		SOCKET sock = accept(hsoListen, NULL, NULL);
		if (sock == INVALID_SOCKET)
		{
			cout << "accept failed, code : " << WSAGetLastError() << endl;
			break;
		}
		cout << " ==> New client " << sock << " connected..." << endl;

		if (!TrySubmitThreadpoolCallback(ChildSockProc, (PVOID)sock, NULL))
		{
			closesocket(sock);
			cout << "TrySubmitThreadpoolCallback failed : " << GetLastError() << endl;
		}
		works.push_back(sock);
	}
	for (WORK_LIST::iterator it = works.begin(); it != works.end(); it++)
		closesocket(*it);

	cout << "==== Server terminates... ==========================" << endl;
	WSACleanup();
}
