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


HWND g_hMsgWnd = NULL;
BOOL CtrlHandler(DWORD fdwCtrlType)
{
	if (g_hMsgWnd != NULL)
		PostMessage(g_hMsgWnd, WM_DESTROY, 0, 0);
	return TRUE;
}

#define WM_ASYNC_SOCKET	WM_USER + 500 //소켓 상태 변경 통지를 받기 위해 유저 정의 메시지를 정의했다.
typedef std::set<SOCKET> SOCK_SET; //자식 소켓을 관리하기 위해 STL set 템플릿 컨테이너를 사용한다.

LRESULT CALLBACK WndSockProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static SOCK_SET* s_pSocks = NULL;

	switch (uMsg)
	{
	case WM_CREATE:
	{
		LPCREATESTRUCT pCS = (LPCREATESTRUCT)lParam;
		s_pSocks = (SOCK_SET*)pCS->lpCreateParams;
		//CreateWindowEx 호출 시 마지막 매개변수로 넘겼던 자식 소켓 관리 세트의 포인터를 획득해서 정적 변수에 저장한다
	}
	/*
	LPCREATESTRUCT :
	Defines the initialization parameters passed to the window procedure of an application. 
	These members are identical to the parameters of the CreateWindowEx function.
	*/
	return 0;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	return 0;

	case WM_ASYNC_SOCKET:
	{
		/*
		지정된 소켓의 상태가 변경되면 시스템은 우리가 정의한 WM_ASYNC_SOCKET 메시지를 전송한다. 
		이 유저 정의 메시지에 대한 핸들러가 이 부분이다. 
		wParam은 상태가 변경된 소켓의 핸들이며, lParam은 에러의 발생 유무와 변경된 상태의 종류를 구분하는 플래그 값을 담고 있다
		*/
		LONG lErrCode = WSAGETSELECTERROR(lParam);//WSAGETSELECTERROR 매크로를 통해 에러를 체크한다. 
		if (lErrCode != 0)	//WSAECONNABORTED
		{
			SOCKET sock = (SOCKET)wParam;
			cout << "~~~ socket " << sock << " failed: " << lErrCode << endl;
			closesocket(sock);
			s_pSocks->erase(sock);
			return 0;
		}

		switch (WSAGETSELECTEVENT(lParam))//WSAGETSELECTEVENT 매크로를 통해 상태 변화의 종류를 획득하고, \
										  종류별 처리를 위해 swtich 문을 이용한다.
		{
		case FD_ACCEPT:
		{
			SOCKET hsoListen = (SOCKET)wParam;
			SOCKET sock = accept(hsoListen, NULL, NULL);
			if (sock == INVALID_SOCKET)
			{
				cout << "accept failed, code : " << WSAGetLastError() << endl;
				break;
			}

			WSAAsyncSelect(sock, hWnd, WM_ASYNC_SOCKET, FD_READ | FD_CLOSE);
			/*
			WSAAsyncSelect는 소켓별로 호출해줘야 한다. 
			accept를 통해 자식 소켓을 수용했다면 자식 소켓의 상태 변경을 통지받기 위해서는 역시 자식 소켓에 대해서도 WSAAsyncSelect를 호출해줘야 한다. 
			물론 자식 소켓에 대해서 관심을 갖는 상태 변화는 데이터 수신과 클라이언트가 접속을 끊었을 때다. 
			따라서 FD_READ| FD_CLOSE 옵션을 지정했다
			*/
			s_pSocks->insert(sock);
			cout << " ==> New client " << sock << " connected" << endl;
		}
		break;

		case FD_READ: 
		{
			SOCKET sock = (SOCKET)wParam;
			char szBuff[512];
			int lSockRet = recv(sock, szBuff, sizeof(szBuff), 0);
			szBuff[lSockRet] = 0;
			cout << " *** Client(" << sock << ") sent : " << szBuff << endl;

			lSockRet = send(sock, szBuff, lSockRet, 0);
			if (lSockRet == SOCKET_ERROR)
				cout << "send failed, code : " << WSAGetLastError() << endl;
		}
		break;

		case FD_CLOSE:
		{
			SOCKET sock = (SOCKET)wParam;
			closesocket(sock);
			s_pSocks->erase(sock);
			cout << " ==> Client " << sock << " disconnected..." << endl;
		}
		break;
		}
	}
	return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
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

	SOCK_SET socks;

	WNDCLASS wcls;
	memset(&wcls, 0, sizeof(wcls));
	wcls.lpfnWndProc = WndSockProc;//call back 함수
	wcls.hInstance = GetModuleHandle(NULL);
	wcls.lpszClassName = _T("WSAAyncSelect");
	if (!RegisterClass(&wcls)) //숨겨진 윈도우 생성을 위한 윈도우 클래스를 등록한다. \
							   WndSockProc 콜백 함수를 지정하고 윈도우 클래스를 등록한다.
	{
		cout << "send failed, code : " << GetLastError() << endl;
		return;
	}

	HWND hWnd = CreateWindowEx
	(
		0, wcls.lpszClassName, NULL, 0, 0, 0, 0, 0,
		HWND_MESSAGE, NULL, wcls.hInstance,
		&socks//SOCK_SET 
	);

	/*
	윈도우를 생성하는 부분이다. Hidden으로 실행시키기 위해 윈도우 스타일과 크기, 시작위치 등은 모두 0으로 설정한다. 
	그리고 메시지 수신 전용 윈도우란 것을 알리기 위해 부모 윈도우 핸들 지정 시에 HWND_MESSAGE를 넘겨주고, 
	제일 마지막 매개변수는 전역 변수의 사용을 피하기 위해 자식 소켓 관리 세트의 포인터를 넘겨준다. 
	이것을 넘겨주면 WndSockProc의 WM_CREATE 메시지에서 이 인스턴스의 포인터를 받을 수 있게 된다.
	*/
	if (!hWnd)
	{
		cout << "send failed, code : " << GetLastError() << endl;
		return;
	}
	g_hMsgWnd = hWnd;
	//CtrlHandler(DWORD fdwCtrlType) 이 함수를 위해
	cout << " ==> Creating hidden window success!!!" << endl;

	SOCKET hsoListen = GetListenSocket(9000);
	if (hsoListen == INVALID_SOCKET)
	{
		WSACleanup();
		return;
	}
	WSAAsyncSelect(hsoListen, hWnd, WM_ASYNC_SOCKET, FD_ACCEPT);
	/*
	리슨 소켓을 감시하기 위해 WSAAsyncSelect로 생성한 윈도우 핸들과 정의한 메시지 값을 넘겨준다.
	이 예제에서도 역시 우리가 원하는 것은 클라이언트의 접속과 데이터 수신, 연결 종료기 때문에, 
	그리고 리슨 소켓은 데이터 수신이 없기 때문에 FD_ACCEPT 비트 플래그만 지정한다.
	*/
	cout << " ==> Waiting for client's connection......" << endl;

	////////////////////////////////////////////////////////////////////////////
	// Message Loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		//이 두 함수는 메시지를 해석하고 목적지 윈도우에 메시지를 전파하는 역할을 한다
	}
	/*
	메시지 루프를 돌다가 소켓의 상태가 변경되면,
	메시지 루프의 GetMessage를 통해 DispatchMessage에 의해 우리가 정의할 윈도우 프로시저 콜백으로 메시지가 전달된다.
	*/
	////////////////////////////////////////////////////////////////////////////

	if (hsoListen != INVALID_SOCKET)
		closesocket(hsoListen);
	for (SOCK_SET::iterator it = socks.begin(); it != socks.end(); it++)
		closesocket(*it);
	cout << "Listen socket closed, program terminates..." << endl;

	WSACleanup();
}
