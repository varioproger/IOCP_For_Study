#pragma comment(lib, "Ws2_32.lib")
#include<tchar.h>
#include "Winsock2.h"
#include "Mswsock.h"
#include "set"
#include "iostream"
using namespace std;


PVOID GetSockExtAPI(SOCKET sock, GUID guidFn)
{
	PVOID pfnEx = NULL;
	GUID guid = guidFn;
	DWORD dwBytes = 0;
	LONG lRet = ::WSAIoctl
	(
		sock, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid, sizeof(guid), &pfnEx,
		sizeof(pfnEx), &dwBytes, NULL, NULL
	);
	if (lRet == SOCKET_ERROR)
	{
		cout << "WSAIoctl failed, code : " << WSAGetLastError() << endl;
		return NULL;
	}
	return pfnEx;
}

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


#ifndef STATUS_LOCAL_DISCONNECT
#	define STATUS_LOCAL_DISCONNECT	((NTSTATUS)0xC000013BL)	//ERROR_NETNAME_DELETED
#endif
#ifndef STATUS_REMOTE_DISCONNECT
#	define STATUS_REMOTE_DISCONNECT	((NTSTATUS)0xC000013CL)	//ERROR_NETNAME_DELETED
#endif
#ifndef STATUS_CONNECTION_RESET
#	define STATUS_CONNECTION_RESET	((NTSTATUS)0xC000020DL)	//ERROR_NETNAME_DELETED
#endif
#ifndef STATUS_CANCELLED
#	define STATUS_CANCELLED			((NTSTATUS)0xC0000120L)	//ERROR_OPERATION_ABORTED
#endif

#define IOKEY_LISTEN	1
#define IOKEY_CHILD		2

#define TM_PROG_EXIT			WM_USER + 1
#define TM_SOCK_CONNECTED		WM_USER + 2
#define TM_SOCK_DISCONNECTED	WM_USER + 3
//스레드 메시지 처리를 위한 정의다. 소켓 풀 관리를 스레드 메시지를 통해 메인 스레드에서 전적으로 관리하도록 함으로써 동기화 처리를 피한다.

#define	POOL_MAX_SIZE	32
#define	POOL_MIN_SIZE	4
#define PERMIT_INC_CNT	4
//풀 관리를 위한 상수를 정의한다. 풀의 최대/최소 크기와 풀 갱신 시 추가될 소켓의 수를 정의한다.


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
	HANDLE	 _iocp;
	DWORD	 _thrid;
	//스레드에 메시지를 전송하기 위한 메인 스레드 ID를 담을 필드를 추가했다
};
typedef IOCP_ENV* PIOCP_ENV;


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

				int nErrCode = WSAGetLastError();
				if (nErrCode != ERROR_ABANDONED_WAIT_0)
					cout << "GQCS failed: " << nErrCode << endl;
				break;
			}

			if (upDevKey == IOKEY_LISTEN)
			{
				CreateIoCompletionPort((HANDLE)psi->_sock, pIE->_iocp, IOKEY_CHILD, 0);
				cout << " ==> New client " << psi->_sock << " connected..." << endl;
				PostThreadMessage(pIE->_thrid, TM_SOCK_CONNECTED, 0, (LPARAM)psi);
				//PostThreadMessage 함수를 사용해 클라이언트 접속 및 해제 시의 처리를 메인 스레드가 한다. \
				스레드 풀 관리 및 접속 소켓 관리를 위해 TM_SOCK_CONNECTED 메시지를 메인 스레드로 부친다.

			}
			else
			{
				if (dwTrBytes == 0)
					throw (INT)ERROR_SUCCESS;

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
		}
		catch (int ex)
		{
			if (ex == STATUS_LOCAL_DISCONNECT || ex == STATUS_CANCELLED)
			{
				cout << " ==> Child socket closed." << endl;
				continue;
			}
			if (ex == ERROR_SUCCESS || ex == STATUS_REMOTE_DISCONNECT)
				cout << " ==> Client " << psi->_sock << " disconnected..." << endl;
			else if (ex == STATUS_CONNECTION_RESET)
				cout << " ==> Pending Client " << psi->_sock << " disconnected..." << endl;
			else
				cout << " ==> Client " << psi->_sock << " has error " << ex << endl;

			PostThreadMessage(pIE->_thrid, TM_SOCK_DISCONNECTED, ex, (LPARAM)psi);
		}
	}
	return 0;
}

int IncreaseAcceptSockets(SOCKET hsoListen, int nIncCnt, SOCK_SET& pool)
{
	// 지정된 수만큼 소켓을 생성하여 AcceptEx 함수를 호출한 후 소켓 풀에 추가한다
	LPFN_ACCEPTEX pfnAcceptEx = (LPFN_ACCEPTEX)
		GetSockExtAPI(hsoListen, WSAID_ACCEPTEX);

	int nPooledCnt = 0;
	for (; nPooledCnt < nIncCnt; nPooledCnt++)
	{
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//접속을 수용할 자식 소켓을 생성한다.
		if (sock == INVALID_SOCKET)
			break;

		PSOCK_ITEM psi = new SOCK_ITEM(sock);
		BOOL bIsOK = pfnAcceptEx
		(
			hsoListen, sock, psi->_buff, 0,
			sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
			NULL, (LPOVERLAPPED)psi
		);
		if (bIsOK == FALSE)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				cout << "AcceptEx failed : " << WSAGetLastError() << endl;
				closesocket(psi->_sock);
				delete psi;
				break;
			}
		}
		pool.insert(psi);//소켓 풀 컨테이너 pool에 SOCK_ITEM 구조체의 인스턴스를 추가한다.
	}
	return nPooledCnt;
}


DWORD g_dwMainThrId = 0;
BOOL CtrlHandler(DWORD fdwCtrlType)
{
	PostThreadMessage(g_dwMainThrId, TM_PROG_EXIT, 0, 0);
	return TRUE;
}

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

	SOCKET hsoListen = GetListenSocket(9000);
	if (hsoListen == INVALID_SOCKET)
	{
		WSACleanup();
		return;
	}
	cout << " ==> Waiting for client's connection......" << endl;

	IOCP_ENV ie;
	ie._thrid = g_dwMainThrId = GetCurrentThreadId();
	ie._iocp = CreateIoCompletionPort((HANDLE)hsoListen, NULL, 1, 2);

	HANDLE hTheads[2];
	for (int i = 0; i < 2; i++)
	{
		DWORD dwThrId;
		hTheads[i] = CreateThread(NULL, 0, IocpSockRecvProc, &ie, 0, &dwThrId);
	}

	SOCK_SET pool, conn;
	IncreaseAcceptSockets(hsoListen, PERMIT_INC_CNT, pool);//POOL_UPT_SIZE 수만큼 미리 소켓 풀에 소켓을 생성하여 추가한다.

	WSAEVENT hEvent = WSACreateEvent();
	WSAEventSelect(hsoListen, hEvent, FD_ACCEPT);//WSAEventSelect용 이벤트를 생성하고 WSAEventSelect를 통해 리슨 소켓과 연결한다.


	while (true)
	{
		DWORD dwWaitRet = MsgWaitForMultipleObjectsEx
		(
			1, &hEvent, 2000, QS_POSTMESSAGE, MWMO_INPUTAVAILABLE
		);
		/*
		스레드 메시지를 이용해 pool과 conn 두 컨테이너를 관리하면서 리슨 소켓과 연결된 이벤트에 대해서도 대기해야 하므로, 
		MsgWaitForMultipleObjectsEx 함수를 호출해 대기한다. 또한 이 함수 호출 시에 타임아웃 값을 지정한 점도 눈여겨보기 바란다.
		리슨 소켓과 연결된 이벤트가 시그널될 때와 타임아웃이 발생했을 때, 
		pool 소켓 풀의 원소를 추가 및 제거하는 아주 원초적인 풀 관리 기능을 구현하고 있다.
		풀 관리에서 타임아웃 발생 여부는 중요하다. 클라이언트 접속 및 해제가 빈번하게 이루어진다면 
		대기 함수는 메시지 처리로 계속 바쁜 상태가 되어 타임아웃이 발생되지 않는다. 
		하지만 타임아웃이 발생했다는 의미는 그만큼 해당 스레드가 한가하다는 의미며, 이는 클라이언트 접속 및 해제가 별로 발생되지 않았다는 것을
		의미한다. 따라서 타임아웃 발생 시에 소켓 풀에 소켓이 너무 많이 추가되어 있을 경우에는 리소스 낭비를 막기 위해 
		여분의 소켓을 닫고 그 인스턴스를 해제할 기회를 갖는다.
		*/
		if (dwWaitRet == WAIT_FAILED)
			break;

		if (dwWaitRet == WAIT_TIMEOUT)
		{//타임아웃이 발생했으며, 이는 소켓 풀의 크기를 감소시킬 수 있는 기회다.
			if (pool.size() > POOL_MIN_SIZE)
			{
				SOCK_SET::iterator it = pool.begin();
				PSOCK_ITEM psi = *it;
				pool.erase(it);
				closesocket(psi->_sock);
				delete psi;

				printf("...Timeout expired, pool=%d, conn=%d\n",
					pool.size(), conn.size());
			}
			continue;
		}

		if (dwWaitRet == 1)//메인 스레드의 메시지 큐에 메시지가 도착했다.
		{
			MSG msg;
			if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				continue;

			if (msg.message == TM_PROG_EXIT)
				//콘솔 컨트롤 핸들러로부터 종료 메시지가 온 경우에는 루프를 탈출하고 프로그램 종료 처리를 수행한다.
				break;

			PSOCK_ITEM psi = (PSOCK_ITEM)msg.lParam;
			if (msg.message == TM_SOCK_CONNECTED)
			{//클라이언트가 접속했으며, 따라서 소켓 풀의 접속된 소켓을 접속 중인 소켓 관리 컨테이너로 이동시켜 준다.
				pool.erase(psi);
				conn.insert(psi);
				printf("...Connection established, pool=%d, conn=%d\n",
					pool.size(), conn.size());
			}
			else if (msg.message == TM_SOCK_DISCONNECTED)
			{//접속이 해제됐으며, 해당 소켓을 우선 접속 중인 소켓 관리 컨테이너로부터 제거한다.
				conn.erase(psi);
				if (pool.size() > POOL_MIN_SIZE)
				{
					closesocket(psi->_sock);
					delete psi;
				}
				else
				{//소켓 풀의 크기가 풀 최소 크기보다 작거나 같을 경우에는 접속 대기 소켓이 여전히 필요한 상황이기 때문에 소켓을 닫으면 안 된다.
					LPFN_DISCONNECTEX pfnDisconnectEx = (LPFN_DISCONNECTEX)
						GetSockExtAPI(psi->_sock, WSAID_DISCONNECTEX);
					pfnDisconnectEx(psi->_sock, NULL, TF_REUSE_SOCKET, 0);
					//소켓의 연결을 끊어주고 TF_REUSE_SOCKET 플래그를 지정하여 재사용이 가능하도록 한다.
					LPFN_ACCEPTEX pfnAcceptEx = (LPFN_ACCEPTEX)
						GetSockExtAPI(hsoListen, WSAID_ACCEPTEX);
					BOOL bIsOK = pfnAcceptEx
					(
						hsoListen, psi->_sock, psi->_buff, 0,
						sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
						NULL, (LPOVERLAPPED)psi
					);//연결을 끊은 소켓을 재사용하기 위해 다시 AcceptEx 함수를 호출해 접속 대기 상태로 만든다.
					if (bIsOK == FALSE)
					{
						int nErrCode = WSAGetLastError();
						if (nErrCode != WSA_IO_PENDING)
						{
							cout << "AcceptEx failed : " << WSAGetLastError() << endl;
							closesocket(psi->_sock);
							delete psi;
							continue;
						}
					}
					pool.insert(psi);//AcceptEx 함수를 호출함으로써 접속 대기 상태가 되었기 때문에 소켓 풀에 소켓을 추가한다.
				}
				printf("...Connection released, pool=%d, conn=%d\n",
					pool.size(), conn.size());
			}
		}
		else
		{
			/*
			WSACreateEvent 호출을 통하여 리슨 소켓과 연결된 이벤트는 시그널 상태가 된다. 
			소켓 풀이 비어있는 상태기 때문에 소켓을 추가해 야 한다.
			*/
			WSANETWORKEVENTS ne;
			WSAEnumNetworkEvents(hsoListen, hEvent, &ne);
			if (ne.lNetworkEvents & FD_ACCEPT)
			{
				if (pool.size() < POOL_MAX_SIZE)
					IncreaseAcceptSockets(hsoListen, PERMIT_INC_CNT, pool);
				//소켓 풀에 소켓을 새롭게 추가하기 위해 IncreaseAcceptSockets 함수를 호출한다.
				printf("...Listen event signaled, pool=%d, conn=%d\n",
					pool.size(), conn.size());
			}
		}
	}
	CloseHandle(ie._iocp);
	WaitForMultipleObjects(2, hTheads, TRUE, INFINITE);

	closesocket(hsoListen);
	CloseHandle(hEvent);
	for (SOCK_SET::iterator it = conn.begin(); it != conn.end(); it++)
	{
		PSOCK_ITEM psi = *it;
		closesocket(psi->_sock);
		delete psi;
	}
	for (SOCK_SET::iterator it = pool.begin(); it != pool.end(); it++)
	{
		PSOCK_ITEM psi = *it;
		closesocket(psi->_sock);
		delete psi;
	}

	cout << "==== Server terminates... ==========================" << endl;
	WSACleanup();
}
