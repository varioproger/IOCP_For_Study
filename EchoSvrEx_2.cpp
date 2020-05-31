#pragma comment(lib, "Ws2_32.lib")
#include <tchar.h>
#include "Winsock2.h"
#include "Mswsock.h"
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

	char		szBuff[4096];
	OVERLAPPED	ov;
	memset(&ov, 0, sizeof(ov));

	LPFN_ACCEPTEX pfnAcceptEx = (LPFN_ACCEPTEX)GetSockExtAPI(hsoListen, WSAID_ACCEPTEX);
	BOOL bIsOK = pfnAcceptEx
	(
		hsoListen, sock, szBuff, 256,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		NULL, &ov
	);
	//AcceptEx 호출 시 접속 후 바로 데이터를 받기 위해 버퍼 사이즈를 256바이트로 지정했다.

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

	bool isValid = true;
	while (true)
	{
		DWORD dwWaitRet = WaitForSingleObject((HANDLE)hsoListen, 1000);//체크를 위해서 타임아웃을 지정한다. 체크 타임을 1초로 지정했다.
		if (dwWaitRet == WAIT_FAILED)
		{
			cout << "WaitForMultipleObjectsEx failed : " << GetLastError() << endl;
			return;
		}

		if (dwWaitRet != WAIT_TIMEOUT)
			break;

		//타임아웃이 발생되면 실제 클라이언트 접속 여부 및 접속 후의 경과 시간을 체크한다.
		DWORD dwSecs;
		INT nBytes = sizeof(dwSecs);
		int nResult = 0;

		nResult = getsockopt(sock, SOL_SOCKET, SO_CONNECT_TIME, (char*)&dwSecs, (PINT)&nBytes);
		//SO_CONNECT_TIME을 옵션으로 넘겨서 미리 생성한 자식 소켓의 접속 경과 시간을 획득한다.
		if (nResult != NO_ERROR)
		{
			printf("getsockopt(SO_CONNECT_TIME) failed: %u\n", WSAGetLastError());
			return;
		}
		if (dwSecs == 0xFFFFFFFF)//dwSecs가 0xFFFFFFFF이면 아직 접속하지 않았음을 의미하므로, 다시 대기 상태로 돌아간다. 
			continue;
		if (dwSecs > 5)
		{
			cout << " ==> Socket " << sock << " connected, but does not send data..." << endl;
			isValid = false;
			/*
			dwSecs가 5초 이상이면 클라이언트가 접속한 후 5초가 경과할 동안 아무런 데이터도 전송하지 않았음을 의미하므로, 
			유효하지 않은 소켓으로 간주하고 isValid 변수를 false로 설정한 후 루프를 탈출한다. 
			*/
			break;
		}
	}

	if (isValid)//체크 상태가 유효하면 작업을 진행하고, 그렇지 않으면 바로 소켓을 닫고 종료한다.
	{
		PSOCKADDR psaLoc = NULL, psaRem = NULL;
		INT nsiLoc = 0, nsiRem = 0;

		LPFN_GETACCEPTEXSOCKADDRS pfnGetSockAddr = (LPFN_GETACCEPTEXSOCKADDRS)
			GetSockExtAPI(hsoListen, WSAID_GETACCEPTEXSOCKADDRS);
		pfnGetSockAddr
		(
			szBuff, 0,
			sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16,
			&psaLoc, &nsiLoc, &psaRem, &nsiRem
		);
		SOCKADDR_IN saLoc = *((PSOCKADDR_IN)psaLoc);
		SOCKADDR_IN saRem = *((PSOCKADDR_IN)psaRem);

		printf(" ==> New client %d  connected...\n", sock);
		printf("     Client %s:%d -> Server%s:%d\n",
			inet_ntoa(saRem.sin_addr), htons(saRem.sin_port),
			inet_ntoa(saLoc.sin_addr), htons(saLoc.sin_port));
		szBuff[ov.InternalHigh] = 0;
		printf("     Recv data : %s\n", szBuff);
	}
	closesocket(sock);
	closesocket(hsoListen);

	cout << "==== Server terminates... ==========================" << endl;

	WSACleanup();
}
