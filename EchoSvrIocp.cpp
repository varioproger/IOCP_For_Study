#pragma comment(lib, "Ws2_32.lib")
#include "Winsock2.h"
#include<tchar.h>
#include "set"
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


SOCKET g_sockMain = INVALID_SOCKET;
BOOL CtrlHandler(DWORD fdwCtrlType)
{
	if (g_sockMain != INVALID_SOCKET)
		closesocket(g_sockMain);
	return TRUE;
}

#ifndef STATUS_LOCAL_DISCONNECT
#	define STATUS_LOCAL_DISCONNECT	((NTSTATUS)0xC000013BL)
#endif
#ifndef STATUS_REMOTE_DISCONNECT
#	define STATUS_REMOTE_DISCONNECT	((NTSTATUS)0xC000013CL)
#endif
//10.5_WSAGetOverlappedResult 참조

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
typedef std::set<PSOCK_ITEM> SOCK_SET;

struct IOCP_ENV
{
	CRITICAL_SECTION _cs;
	SOCK_SET		 _set;
	HANDLE			 _iocp;
};
typedef IOCP_ENV* PIOCP_ENV;


#define IOKEY_LISTEN	1
#define IOKEY_CHILD		2

DWORD WINAPI IocpSockRecvProc(PVOID pParam)
{
	PIOCP_ENV	pIE = (PIOCP_ENV)pParam;
	PSOCK_ITEM	psi = NULL;
	DWORD		dwTrBytes = 0;
	ULONG_PTR	upDevKey = 0;

	while (true)
	{
		try
		{
			BOOL bIsOK = GetQueuedCompletionStatus
			(
				pIE->_iocp, &dwTrBytes, &upDevKey, (LPOVERLAPPED*)&psi, INFINITE
			);
			if (bIsOK == FALSE)
			{
				if (psi != NULL)
					throw (int)psi->Internal;
				//에러가 발생했으며, 해당 에러가 자식 소켓 관련 에러인 경우는 예외를 던져 catch 블록에서 자식 소켓에 대한 에러 처리를 수행토록 한다.

				int nErrCode = WSAGetLastError();
				if (nErrCode != ERROR_ABANDONED_WAIT_0)
					cout << "GQCS failed: " << nErrCode << endl;
				/*
				IOCP 자체에 관련된 에러인 경우는 에러를 출력하고 루프를 탈출해 스레드를 종료한다. 
				에러 코드 ERROR_ABANDONED_WAIT_0은 IOCP 핸들을 닫았을 때 발생하는 에러며, 
				이는 프로그램 종료 과정에서 메인 스레드가 IOCP를 닫으면서 발생하는 에러이므로 정상 처리에 속한다.
				*/
				break;
			}

			if (upDevKey == IOKEY_LISTEN)
			{
				CreateIoCompletionPort((HANDLE)psi->_sock, pIE->_iocp, IOKEY_CHILD, 0);
				cout << " ==> New client " << psi->_sock << " connected..." << endl;

				EnterCriticalSection(&pIE->_cs);
				pIE->_set.insert(psi);
				LeaveCriticalSection(&pIE->_cs);
			}
			else //자식 소켓이 야기한 비동기 수신에 대한 데이터 수신이 완료되었음을 의미하므로, 수신된 데이터에 대한 처리를 수행한다.
			{
				if (dwTrBytes == 0)
					throw (INT)ERROR_SUCCESS;
				/*
				수신 바이트 수가 0인 경우는 클라이언트가 closesocket을 호출해 정상적으로 접속을 끊었음을 의미하므로, 
				이 경우 역시 예외를 던져 catch 블록에서 처리하도록 한다.
				*/

				psi->_buff[dwTrBytes] = 0;
				cout << " *** Client(" << psi->_sock << ") sent : " << psi->_buff << endl;
				int lSockRet = send(psi->_sock, psi->_buff, dwTrBytes, 0);
				if (lSockRet == SOCKET_ERROR)
					throw WSAGetLastError();
			}

			DWORD dwFlags = 0;
			WSABUF wb;
			wb.buf = psi->_buff, wb.len = sizeof(psi->_buff);
			int nSockRet = WSARecv(psi->_sock, &wb, 1, NULL, &dwFlags, psi, NULL);
			if (nSockRet == SOCKET_ERROR)
			{
				int nErrCode = WSAGetLastError();
				if (nErrCode != WSA_IO_PENDING)
					throw nErrCode;
			}
			/*
			WSARecv를 비동기적으로 호출해 비동기 수신 개시를 알린다. 
			IOKEY_LISTEN인 경우 최초 개시가 되고 IOKEY_CHILD인 경우는 다음의 데이터 수신을 위한 반복 개시가 된다.
			*/
		}
		catch (int ex)
		{
			if (ex != STATUS_LOCAL_DISCONNECT)
			{
				if (ex == ERROR_SUCCESS || ex == STATUS_REMOTE_DISCONNECT)
					cout << " ==> Client " << psi->_sock << " disconnected..." << endl;
				else
					cout << " ==> Client " << psi->_sock << " has error " << ex << endl;
				/*
				STATUS_LOCAL_DISCONNECT는 메인 스레드에서 자식 소켓을 닫은 경우이므로 처리 대상에서 제외된다. 
				STATUS_REMOTE_DISCONNECT는 클라이언트 강제 종료로 인한 접속 해제, 
				ERROR_SUCCESS는 클라이언트의 정상적인 접속 해제에 해당하므로, 
				에러가 아닌 것으로 간주하고 나머지 에러에 대해서는 에러 코드를 출력한다.
				*/
				closesocket(psi->_sock);

				EnterCriticalSection(&pIE->_cs);
				pIE->_set.erase(psi);
				LeaveCriticalSection(&pIE->_cs);
				delete psi;
			}
			else
				cout << " ==> Child socket " << psi->_sock << " closed." << endl;
		}
	}
	return 0;
}

void _tmain()
{
	WSADATA	wsd;
	int nIniCode = WSAStartup(MAKEWORD(2, 2), &wsd);
	if (nIniCode)
	{
		cout << "WSAStartup failed with error : " << nIniCode << endl;
		return;
	}

	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		cout << "SetConsoleCtrlHandler failed, code : " << GetLastError() << endl;
		return;
	}

	SOCKET hsoListen = GetListenSocket(9001);
	if (hsoListen == INVALID_SOCKET)
	{
		WSACleanup();
		return;
	}
	g_sockMain = hsoListen;
	cout << " ==> Waiting for client's connection......" << endl;

	IOCP_ENV ie;
	InitializeCriticalSection(&ie._cs);
	ie._iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	HANDLE hTheads[2];
	for (int i = 0; i < 2; i++)
	{
		DWORD dwThreadId;
		hTheads[i] = CreateThread(NULL, 0, IocpSockRecvProc, &ie, 0, &dwThreadId);
	}

	while (true)
	{
		SOCKET sock = accept(hsoListen, NULL, NULL);
		if (sock == INVALID_SOCKET)
		{
			int nErrCode = WSAGetLastError();
			if (nErrCode != WSAEINTR)
				cout << " ==> Accept failed, code = " << nErrCode << endl;
			break;
		}
		PSOCK_ITEM pSI = new SOCK_ITEM(sock);
		PostQueuedCompletionStatus(ie._iocp, 0, IOKEY_LISTEN, pSI);
	}
	CloseHandle(ie._iocp);
	WaitForMultipleObjects(2, hTheads, TRUE, INFINITE);

	for (SOCK_SET::iterator it = ie._set.begin(); it != ie._set.end(); it++)
	{
		PSOCK_ITEM psi = *it;
		closesocket(psi->_sock);
		delete psi;
	}

	DeleteCriticalSection(&ie._cs);
	cout << "==== Server terminates... ==========================" << endl;

	WSACleanup();
}
