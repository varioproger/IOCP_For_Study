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
	LONG lRet = WSAIoctl
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


struct SOCK_ITEM : OVERLAPPED
{
	SOCKET	_sock;
	PTP_IO	_ptpIo; //// 자식 소켓과 연결될 TP_IO 객체의 포인터 필드
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
	SOCK_SET	_conn;
	SOCKET   _listen;   // 리슨 소켓 핸들
	WSAEVENT  _event;   // 접속 요청을 통지받을 리슨 소켓과 연결된 이벤트
};
typedef IOCP_ENV* PIOCP_ENV;



void WINAPI Handler_SockChild(PTP_CALLBACK_INSTANCE pInst,
	PVOID pCtx, PVOID pov, ULONG dwErrCode, ULONG_PTR dwTrBytes, PTP_IO ptpIo)
{
	PIOCP_ENV  pie = (PIOCP_ENV)pCtx;
	PSOCK_ITEM psi = (PSOCK_ITEM)pov;

	if (dwTrBytes > 0 && dwErrCode == NO_ERROR)
	{//전송 바이트 수가 0보다 크고 에러가 없는 경우, 데이터 수신 처리를 한다.
		if ((int)dwTrBytes > 0)
		{
			/*
			형 변환 결과, 전송 바이트가 양수인 경우에는 에코 처리를 수행한다.
			접속 수용 콜백에서 WSARecv 최초 호출을 위해 이 콜백 함수를 의도적으로 호출하기 때문에 음수인 경우에는
			에코 처리를 건너뛰어야 한다.
			*/
			psi->_buff[dwTrBytes] = 0;
			cout << " *** Client(" << psi->_sock << ") sent : " << psi->_buff << endl;

			int lSockRet = send(psi->_sock, psi->_buff, dwTrBytes, 0);
			if (lSockRet == SOCKET_ERROR)
			{
				dwErrCode = WSAGetLastError();
				goto $LABEL_CLOSE;
			}
		}

		StartThreadpoolIo(ptpIo);
		WSABUF wb; DWORD dwFlags = 0;
		wb.buf = psi->_buff, wb.len = sizeof(psi->_buff);
		int nSockRet = WSARecv(psi->_sock, &wb, 1, NULL, &dwFlags, psi, NULL);
		//먼저 StartThreadpoolIo를 호출한 후 WSARecv를 비동기적으로 호출한다.
		if (nSockRet == SOCKET_ERROR)
		{
			dwErrCode = WSAGetLastError();
			if (dwErrCode != WSA_IO_PENDING)
			{
				CancelThreadpoolIo(ptpIo);
				//에러가 발생했을 경우 CancelThreadpoolIo를 호출한다.
				goto $LABEL_CLOSE;
			}
		}
		return;
	}

$LABEL_CLOSE:
	if (dwErrCode == ERROR_OPERATION_ABORTED)
		return;
	/*
	ERROR_OPERATION_ABORTED는 프로그램 종료를 위해 CancelIoEx를 호출한 경우고,
	자식 소켓 관련 리소스는 메인 스레드에 서 해제하기 때문에 바로 리턴한다.
	*/
	if (dwErrCode == ERROR_SUCCESS || dwErrCode == ERROR_NETNAME_DELETED)
		cout << " ==> Client " << psi->_sock << " disconnected..." << endl;
	else
		cout << " ==> Error occurred, code = " << dwErrCode << endl;
	EnterCriticalSection(&pie->_cs);
	pie->_conn.erase(psi);
	LeaveCriticalSection(&pie->_cs);

	closesocket(psi->_sock);
	delete psi;
	CloseThreadpoolIo(ptpIo);
	/*
	자식 소켓 관련 리소스를 해제하고 TP_IO 객체를 해제한다.
	이 콜백 함수 내에서 CloseThreadpoolIo를 호출할 수 있는 근거는 코드 상에서 중첩 입출력을 발생시키지 않았기에
	현재 실행 중인 콜백은 본 콜백 함수밖에 없기 때문이다.
	*/
}

VOID WINAPI Handler_SockListen(PTP_CALLBACK_INSTANCE pInst, PVOID pCtx, PTP_WAIT ptpWait, TP_WAIT_RESULT)
{
	PIOCP_ENV pie = (PIOCP_ENV)pCtx;
	//IOCP_ENV 인스턴스를 획득한다.
	WSANETWORKEVENTS ne;
	WSAEnumNetworkEvents(pie->_listen, pie->_event, &ne);
	if (!(ne.lNetworkEvents & FD_ACCEPT))
		return;

	int nErrCode = ne.iErrorCode[FD_ACCEPT_BIT];
	if (nErrCode != 0)
	{
		cout << " ==> Listen failed, code = " << nErrCode << endl;
		return;
	}

	SOCKET sock = accept(pie->_listen, NULL, NULL);
	if (sock == INVALID_SOCKET)
	{
		nErrCode = WSAGetLastError();
		if (nErrCode != WSAEINTR)
			cout << " ==> Listen failed, code = " << nErrCode << endl;
		return;
	}
	cout << " ==> New client " << sock << " connected..." << endl;

	PSOCK_ITEM psi = new SOCK_ITEM(sock);
	psi->_ptpIo = CreateThreadpoolIo((HANDLE)psi->_sock, Handler_SockChild, pie, NULL);
	//TP_IO 객체를 생성하고 접속이 수용된 자식 소켓과 연결한다. 컨텍스트 매개변수로 IOCP_ENV 구조체의 포인터를 전달한다.
	EnterCriticalSection(&pie->_cs);
	pie->_conn.insert(psi);
	LeaveCriticalSection(&pie->_cs);

	Handler_SockChild(pInst, pie, psi, NO_ERROR, -1, psi->_ptpIo);
	//데이터 수신 개시를 위해 TP_IO용 콜백 함수를 직접 호출한다.
	SetThreadpoolWait(ptpWait, pie->_event, NULL);
	//리슨용 이벤트의 시그널을 계속 받기 위해 SetThreadpoolWait를 호출한다.
}


void _tmain()
{
	WSADATA	wsd;
	int nInitCode = WSAStartup(MAKEWORD(2, 2), &wsd);
	if (nInitCode)
	{
		cout << "WSAStartup failed with error : " << nInitCode << endl;
		return;
	}

	PTP_WAIT psoWait = NULL;
	IOCP_ENV ie;
	InitializeCriticalSection(&ie._cs);
	try
	{
		ie._listen = GetListenSocket(9000);
		if (ie._listen == INVALID_SOCKET)
			throw WSAGetLastError();
		cout << " ==> Waiting for client's connection......" << endl;

		psoWait = CreateThreadpoolWait(Handler_SockListen, &ie, NULL);//PTP_WAIT 객체를 생성한다. 매개변수로 IOCP_ENV 포인터를 전달한다.
		if (psoWait == NULL)
			throw (int)GetLastError();

		ie._event = WSACreateEvent();
		if (ie._event == NULL)
			throw WSAGetLastError();
		SetThreadpoolWait(psoWait, ie._event, NULL);
		//이벤트의 시그널 상태를 잡기 위해 리슨용 이벤트를 생성하고 PTP_WAIT 객체와 연결한다.
		WSAEventSelect(ie._listen, ie._event, FD_ACCEPT);

		getchar();
		//종료를 위해 키 입력을 대기한다.
	}
	catch (int ex)
	{
		cout << "Error occurred in main, " << ex << endl;
	}

	if (ie._listen != INVALID_SOCKET)
		closesocket(ie._listen);
	if (psoWait != NULL)
	{
		WaitForThreadpoolWaitCallbacks(psoWait, TRUE);
		CloseThreadpoolWait(psoWait);
		//접속 수용 콜백 함수의 실행 완료를 대기하고 TP_WAIT 객체를 해제한다.
	}
	if (ie._event != NULL)
		CloseHandle(ie._event);

	for (SOCK_SET::iterator it = ie._conn.begin(); it != ie._conn.end(); it++)
	{
		PSOCK_ITEM psi = *it;
		CancelIoEx((HANDLE)psi->_sock, NULL);
		WaitForThreadpoolIoCallbacks(psi->_ptpIo, TRUE);
		//연결된 자식 소켓 해제를 위해 우선 대기 중인 입출력을 취소하고, 콜백 항목의 취소 및 완료를 기다린다.
		closesocket(psi->_sock);
		CloseThreadpoolIo(psi->_ptpIo);
		//자식 소켓, TP_IO 객체 및 관련 리소스를 해제한다.
		delete psi;
	}
	DeleteCriticalSection(&ie._cs);

	cout << "==== Server terminates... ==========================" << endl;
	WSACleanup();
}
