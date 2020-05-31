#pragma comment(lib, "Ws2_32.lib")
#include <tchar.h>
#include "Winsock2.h"
#include "Mswsock.h"
#include "iostream"
using namespace std;

//이 소스에 대한 이해가 끝났다면 16.AcceptEx 밑부분을 다시 확인 해보세요

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

void _tmain()
{
	WSADATA	wsd;
	int nErrCode = WSAStartup(MAKEWORD(2, 2), &wsd);
	if (nErrCode)
	{
		cout << "WSAStartup failed with error : " << nErrCode << endl;
		return;
	}

	SOCKET hsoListen = GetListenSocket(9000, 1);
	if (hsoListen == INVALID_SOCKET)
	{
		WSACleanup();
		return;
	}
	cout << " ==> Waiting for client's connection......" << endl;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		WSACleanup();
		return;
	}

	int nSize = (sizeof(SOCKADDR_IN) + 16) * 2;
	OVERLAPPED	ov;
	char		szBuff[4096];
	DWORD		dwRecvLen = 0;
	//DWORD   dwRecvLen =  256
	memset(&ov, 0, sizeof(ov));
	/*
	중첩 구조체를 초기화하고 수신할 데이터 및 로컬/리모트 주소를 담을 수 있는 충분한 크기의 버퍼를 준비한다. 
	dwRecvLen은 접속 후 바로 수신할 데이터 크기를 의미하며, 여기서는 0으로 설정해 접속과 동시에 데이터를 수신하지 않겠다는 것을 알린다.
	*/
	LPFN_ACCEPTEX pfnAcceptEx = (LPFN_ACCEPTEX)GetSockExtAPI(hsoListen, WSAID_ACCEPTEX);
	//확장 API AcceptEx의 함수 포인터를 획득한다.

	BOOL bIsOK = pfnAcceptEx
	(
		hsoListen,        // 리슨 소켓 핸들
		sock,          // 미리 생성한 자식 소켓 핸들
		szBuff,          // 
		dwRecvLen,        // 접속 시 수신할 데이터 크기
		sizeof(SOCKADDR_IN) + 16,  // 
		sizeof(SOCKADDR_IN) + 16,  //
		NULL,          // 
		&ov          // 비동기 accept를 위한 OVERLAPPED 구조체
	);
	/*
	AcceptEx를 호출한다. 리슨 소켓 핸들과 클라이언트 접속 시 할당을 위해 미리 생성한 소켓의 핸들을 넘겨준다. 
	dwRecvLen의 값을 여기서는 0으로 설정해 접속 시 수신 데이터는 받지 않겠다는 것을 지시한다. 
	로컬과 리모트 주소를 위해 SOCKADDR_IN 구조체 크기에 16을 더해 크기를 지정하고, 
	비동기 접속 수용을 위해 중첩 구조체의 포인터를 넘겨준다.
	*/
	if (!bIsOK)
	{
		int nErrCode = WSAGetLastError();
		if (nErrCode != WSA_IO_PENDING)
		{
			cout << "AcceptEx failed, code : " << WSAGetLastError() << endl;
			closesocket(sock);
			closesocket(hsoListen);
			WSACleanup();
			return;
		}
	}

	DWORD dwWaitRet = WaitForSingleObject((HANDLE)hsoListen, INFINITE);
	//클라이언트로부터 접속이 들어오면 대기 함수에서 리턴되어 다음 작업을 수행할 수 있다
	if (dwWaitRet == WAIT_FAILED)
	{
		cout << "WaitForMultipleObjectsEx failed : " << GetLastError() << endl;
		return;
	}


	PSOCKADDR psaLoc = NULL, psaRem = NULL;
	INT nsiLoc = 0, nsiRem = 0;
	WSAEFAULT;
	LPFN_GETACCEPTEXSOCKADDRS pfnGetSockAddr = (LPFN_GETACCEPTEXSOCKADDRS)
		GetSockExtAPI(hsoListen, WSAID_GETACCEPTEXSOCKADDRS);
	//확장 API가 GetAcceptExSockaddrs의 함수 포인터를 획득한다.
	pfnGetSockAddr
	(
		szBuff, 
		dwRecvLen,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&psaLoc, 
		&nsiLoc, 
		&psaRem, 
		&nsiRem
	);
	/*
	접속이 되면 버퍼로부터 로컬 및 리모트 주소를 획득한다. 
	AcceptEx 호출 시 넘겨줬던 버퍼, dwRecvLen, 그리고 로컬/리모트 주소 크기를 이 함수 호출 시 그대로 넘겨줘야 한다.
	*/
	SOCKADDR_IN saLoc = *((PSOCKADDR_IN)psaLoc);
	SOCKADDR_IN saRem = *((PSOCKADDR_IN)psaRem);

	printf(" ==> New client %d  connected...\n", sock);
	printf("     Client %s:%d -> Server%s:%d\n",
		inet_ntoa(saRem.sin_addr), htons(saRem.sin_port),
		inet_ntoa(saLoc.sin_addr), htons(saLoc.sin_port));

	if (dwRecvLen > 0)
	{
		szBuff[ov.InternalHigh] = 0;
		printf("     Recv data : %s\n", szBuff);
	}

	closesocket(sock);
	closesocket(hsoListen);

	cout << "==== Server terminates... ==========================" << endl;

	WSACleanup();

}