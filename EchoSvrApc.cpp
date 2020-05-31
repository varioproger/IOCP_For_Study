#pragma comment(lib, "Ws2_32.lib")
#include<tchar.h>
#include "Winsock2.h"
#include "set"
#include "iostream"
using namespace std;

#define USE_CANCELIO	1
// 확실한 방법은 아래 코드처럼 메인 함수에서 CancelIoEx 함수를 호출하는 것이다.


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


WSAEVENT g_hevExit;
BOOL CtrlHandler(DWORD fdwCtrlType)
{
    if (g_hevExit != NULL)
        SetEvent(g_hevExit);
    return TRUE;
}

struct SOCK_ITEM;
typedef std::set<SOCK_ITEM*> SOCK_SET;
struct SOCK_ITEM : OVERLAPPED
{
    SOCKET		_sock;
    char		_buff[512];
    SOCK_SET* _pSet;

    SOCK_ITEM(SOCKET sock, SOCK_SET* pSet)
    {
        hEvent = NULL;
        Offset = OffsetHigh = 0;
        memset(_buff, 0, sizeof(_buff));
        _sock = sock;
        _pSet = pSet;
    }
};
typedef SOCK_ITEM* PSOCK_ITEM;


VOID CALLBACK RecvSockCallback(DWORD dwErrCode, DWORD dwTranBytes, LPOVERLAPPED pOL, DWORD dwFlags)
{
    PSOCK_ITEM pSI = (PSOCK_ITEM)pOL;

    if (dwTranBytes > 0 && dwErrCode == 0)
    {
        pSI->_buff[dwTranBytes] = 0;
        cout << " *** Client(" << pSI->_sock << ") sent : " << pSI->_buff << endl;

        int lSockRet = send(pSI->_sock, pSI->_buff, dwTranBytes, 0);
        if (lSockRet == SOCKET_ERROR)
        {
            dwErrCode = WSAGetLastError();
            goto $LABEL_CLOSE;
        }

        WSABUF wb;
        wb.buf = pSI->_buff, wb.len = sizeof(pSI->_buff);
        int nSockRet = WSARecv(pSI->_sock, &wb, 1, NULL, &dwFlags, pSI, RecvSockCallback);
        ////WSARecv를 다시 호출해 경보가능 비동기 수신을 개시한다.
        if (nSockRet == SOCKET_ERROR)
        {
            dwErrCode = WSAGetLastError();
            if (dwErrCode != WSA_IO_PENDING)
                goto $LABEL_CLOSE;
        }
        return;
    }

$LABEL_CLOSE:
#if (USE_CANCELIO == 0)
    if (dwErrCode != WSAECONNABORTED)
#else
    if (dwErrCode != ERROR_OPERATION_ABORTED)
#endif
    {
        if (dwErrCode == ERROR_SUCCESS || dwErrCode == WSAECONNRESET)
            cout << " ==> Client " << pSI->_sock << " disconnected..." << endl;
        else
            cout << " ==> Error occurred, code = " << dwErrCode << endl;
        closesocket(pSI->_sock);
        pSI->_pSet->erase(pSI);
        delete pSI;
        /*
        에러가 WSAECONNABORTED가 아닌 경우, 즉 클라이언트가 소켓을 닫거나 통신 대기 중 에러가 발생한 경우에만 소켓을 닫고 
        pSI 인스턴스를 해제한다. WSAECONNABORTED인 경우는 메인 함수의 종료 처리에서 pSI 인스턴스의 해제까지 모두 처리한다.
        */
    }
    else
        cout << " ==> Child socket " << pSI->_sock << " closed." << endl;
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
    g_hevExit = CreateEvent(NULL, TRUE, FALSE, NULL);

    SOCKET hsoListen = GetListenSocket(9001);
    if (hsoListen == INVALID_SOCKET)
    {
        WSACleanup();
        return;
    }
    cout << " ==> Waiting for client's connection......" << endl;

    WSAEVENT hEvent = WSACreateEvent();
    WSAEventSelect(hsoListen, hEvent, FD_ACCEPT);

    SOCK_SET siols;
    HANDLE arSynObjs[2] = { g_hevExit, hEvent };
    while (true)
    {
        DWORD dwWaitRet = WaitForMultipleObjectsEx(2, arSynObjs, FALSE, INFINITE, TRUE);
        /*
        the wait function returns and the completion routine is called only if bAlertable is TRUE
        and the calling thread is the thread that initiated the read or write operation.

        즉 접속 요청을 위한 대기 및 수신 완료 처리를 받기 위해 경보가능 대기 상태로 들어간다.
        */

        if (dwWaitRet == WAIT_FAILED)
        {
            cout << "WaitForMultipleObjectsEx failed : " << GetLastError() << endl;
            break;
        }

        if (dwWaitRet == WAIT_OBJECT_0)
            break;

        if (dwWaitRet == WAIT_IO_COMPLETION)
        {
            //소켓의 입출력 완료 루틴 RecvCallback이 수행되며, 특별히 처리할 것이 없으므로 다시 루프의 첫 부분으로 돌아간다.
            continue;
        }

        WSANETWORKEVENTS ne;
        WSAEnumNetworkEvents(hsoListen, hEvent, &ne);
        if (ne.lNetworkEvents & FD_ACCEPT)
        {
            int nErrCode = ne.iErrorCode[FD_ACCEPT_BIT];
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
                    cout << " ==> Error occurred, code = " << nErrCode << endl;
                break;
            }
            cout << " ==> New client " << sock << " connected" << endl;

            PSOCK_ITEM pSI = new SOCK_ITEM(sock, &siols);

            WSABUF wb;
            wb.buf = pSI->_buff, wb.len = sizeof(pSI->_buff);
            DWORD dwFlags = 0;
            int nSockRet = WSARecv(sock, &wb, 1, NULL, &dwFlags, pSI, RecvSockCallback);
            //최초 경보가능 비동기 수신을 개시한다. 데이터 수신 시에 RecvCallback 함수가 호출된다.
            if (nSockRet == SOCKET_ERROR && (nErrCode = WSAGetLastError()) != WSA_IO_PENDING)
            {
                cout << "ReadFile failed : " << nErrCode << endl;
                closesocket(sock);
                delete pSI;
            }
            else
                siols.insert(pSI);
        }
    }

    if (hsoListen != INVALID_SOCKET)
        closesocket(hsoListen);
    for (SOCK_SET::iterator it = siols.begin(); it != siols.end(); it++)
    {
        PSOCK_ITEM pSI = *it;
#if (USE_CANCELIO == 0)
        closesocket(pSI->_sock);
        SleepEx(INFINITE, TRUE);
#else
        CancelIoEx((HANDLE)pSI->_sock, NULL);
        SleepEx(INFINITE, TRUE);
        closesocket(pSI->_sock);
#endif
        delete pSI;

        /* 원래 코드는 delete pSI;부분이 주석처리 되어 있었다.
        PSOCK_ITEM pSI = *it;
		closesocket(pSI->_sock);
		//delete pSI;
        */
    }
    CloseHandle(hEvent);
    CloseHandle(g_hevExit);

    cout << "==== Server terminates... ==========================" << endl;

    WSACleanup();
}
/*
위에 코드 처럼 변경이 된 이유는 주석 처리된 ‘delete pSI; ’ 라인의 주석을 제거하고 실행하면 
클라이언트가 접속되어 있는 상황에서 서버 프로그램을 종료하면 프로그램은 다운될 것이다.
종료 처리를 위해 자식 소켓에 대해 closesocket 함수를 호출하면 
closesocket 함수는 내부적으로 APC큐에 등록된 APC에 대해 취소 처리를 한다.
하지만 이 취소 처리는 취소 표시만 할 뿐이고, APC큐로부터의 엔트리 제거는 콜백 함수의 수행이 완료된 이후에나 가능하다.
따라서 closesocket 함수는 취소를 요청한 후 APC의 콜백 함수가 실행 완료되기를 기다리지 않고 바로 리턴된다.
따라서 delete의 주석을 풀게 되면 이미 해당 비동기 수신을 식별하는 SOCK_ITEM 구조체의 인스턴스는 메모리로부터 제거된다.
그리고 메인 스레드는 “Server terminates...”라는 종료 메시지를 출력하고 메인 함수를 빠져나갈 것이다.
그 후 시스템은 최종적으로 APC를 제거하기 위해 메인 스레드로 하여금 APC 큐에 등록된 콜백 함수를 실행하도록 만든다.
이 시점에서 호출된 콜백 함수의LPOVERLAPPED 매개변수 pOL은 이미 메모리 상에서 해제된 상태가 되기 때문에, 
결국 콜백 함수에서 pOL을 참조하면서 메모리 접근 위반을 일으켜 프로그램을 다운시키게 된다.
사실 그 전에 이미 dwErrCode 매개변수는 잘못된 매개변수가 입력되었음을 의미하는 WSAEINVAL(10022) 에러 코드를 담게 될 것이다.
*/