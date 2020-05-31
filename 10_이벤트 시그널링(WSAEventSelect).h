/*
이벤트 커널 객체를 통하여 소켓의 상태 변화를 통지하는 수단이 바로 WSAEventSelect 함수다

int WSAEventSelect
(
  _In_ SOCKET    s,
  _In_ WSAEVENT   hEventObject,
  _In_ long     lNetworkEvents
);

SOCKET s
  WSAAsyncSelect 함수에서와 마찬가지로 상태 체크를 수행할 대상 소켓이다.

WSAEVENT hEventObject
   상태 변경을 통지받을 이벤트 커널 객체의 핸들이다. CreateEvent나 WSACreateEvent 함수를 통해서 생성한 핸들값을 넘겨준다.

long  lNetworkEvents
  WSAAsyncSelect 함수의 마지막 매개변수인 lEvent와 동일한 의미를 지닌다.

상태 변경에 대한 통지를 받을 이벤트 커널 객체 생성을 위해 MS는 별도로 WSACreateEvent라는함수를 제공하며, 
이벤트와 관련해서는 WSA로 시작하는 전용 함수들을 제공한다. 
물론 기존의 이벤트 관련 함수들을 사용하더라도 전혀 상관없다.

WSAEVENT WSACreateEvent(void);
BOOL WSACloseEvent(_In_ WSAEVENT hEvent);
BOOL WSASetEvent(_In_ WSAEVENT hEvent);
BOOL WSAResetEvent(_In_ WSAEVENT hEvent);

WSACreateEvent 함수는 이름이 없으며, 초기 상태가 넌시그널 상태인 수동 리셋 이벤트를 생성한다. 
따라서 시그널 상태로 바뀌었을 때 다시 대기 상태로 들어가기 전에 리셋시켜줘야 한다.
그리고 이벤트 관련 함수뿐만 아니라 대기 함수도 WSA로 시작하는 유사한 함수들을 제공한다.

DWORD WSAWaitForMultipleEvents
(
  _In_ DWORD      cEvents,
  _In_ const  WSAEVENT*  lphEvents,
  _In_ BOOL      fWaitAll,
  _In_ DWORD      dwTimeout,
  _In_ BOOL      fAlertable
);
물론 이 함수들에 대해서는 별도의 설명이 필요 없을 것이다. 이미 우리가 익히 알고 있는, WSA 접두사가 없는 기존 함수들을 사용해도 전혀 무방하다.

이벤트커널 객체 시그널링은 그 변경의 통지를 유저가 생성한 이벤트 커널 객체를 시그널 상태로 만들어 통지를 한다.
이벤트에 대해 통지를 받기 위해 대기하는 부분이 필요하다.
DWORD dwWaitRet = WSAWaitForMultipleEvents
(
    nSockCnt, arrEvents, FALSE, INFINITE, FALSE
);
소켓의 상태가 변경되면 이 함수를 호출한 스레드는 깨어나 소켓의 변경된 상태에 대한 체크를 수행할 수 있다. 
(dwWaitRet – WAIT_OBJECT_0)의 값에 해당하는 인덱스 상태가 변경된 소켓의 인덱스임을 의미한다.


메시지 방식의 LPARAM에 해당하는것, 즉 상태 변경의 종류나 에러 발생 여부를 판단하기 위해서는 다음의 함수를 호출해야 한다.

int WSAEnumNetworkEvents
(
    _In_   SOCKET       s,
    _In_   WSAEVENT      hEventObject,
    _Out_  LPWSANETWORKEVENTS   lpNetworkEvents
);
SOCKET s
WSAEVENT hEventObject
  변경 통지를 받은 소켓의 핸들과 이벤트 커널 객체의 핸들을 넘겨준다.

LPWSANETWORKEVENTS  lpNetworkEvents 
   변경된 상태의 종류를 식별하는 내용과 에러 발생 시의 에러 코드를 담고 있다. 
   WSANET WORKEVENTS 구조체는 다음과 같다.

typedef struct _WSANETWORKEVENTS
{
  long  lNetworkEvents;
  int  iErrorCode[FD_MAX_EVENTS];
} WSANETWORKEVENTS, FAR * LPWSANETWORKEVENTS;

long  lNetworkEvents
  윈도우 메시지 방식에서의 WSAGETSELECTEVENT (lParam) 값과 동일하다.

int  iErrorCode[FD_MAX_EVENTS]
   윈도우 메시지 방식에서의 WSAGETSELECTERROR (lParam)과 비슷한 의미로, 에러가 발생하면 그 에러 코드를 담고 있다. 
   에러의 식별은 각 상태 변경 식별 플래그별로 FD_XXX_BIT 매크로가 제공되어, 이 값이 iErrorCode 배열의 인덱스를 가리킨다. 
   예를 들어 FD_ACCEPT 플래그인 경우 iErrorCode[FD_ACCEPT_BIT]에 해당 에러 코드를 담게 된다.

WSAEnumNetworkEvents 함수는 매개변수로 넘겨진 소켓 핸들에 대해 변경사항을 체크하고 내부 네트워크 이벤트 레코드를 클리어한다. 
그리고 또 중요한 역할은 매개변수로 넘겨진 이벤트 커널 객체를 리셋시켜 넌시그널 상태로 만든다.


WSAEventSelect 함수를 호출할 때 lNetworkEvents 매개변수를 ‘FD_READ|FD_CLOSE’로지정했을 경우, 
WSAEnumNetworkEvents 함수를 통해서 획득한 WSANETWORKEVENTS 구조체의 lNetworkEvents 필드와 iErrorCode 필드를 체크하는 방식은 다음과 같다

WSANETWORKEVENTS ne;
WSAEnumNetworkEvents(sock, hEvent, &ne);

if (ne.lNetworkEvents & FD_READ)
{
    클라이언트로부터 데이터가 전송되었다.
        if (ne.iErrorCode[FD_READ_BIT] != 0)
            수신 과정에서 에러가 발생했다.
        else
            recv를 호출해 전송된 데이터를 수신할 수 있다.
}
if (ne.lNetworkEvents & FD_CLOSE)
{
    클라이언트가 접속을 끊었다.
        if (ne.iErrorCode[FD_CLOSE_BIT] != 0)
            접속 해제 과정에서 에러가 발생했다.
        else
            closesocket을 호출해 자식 소켓을 닫을 수 있다.
}
⋮
주의할 것은 위의 코드처럼 자신이 지정한 플래그는 모두 체크해야 한다. 
“if ~ elseif ” 패턴으로 처리하지 말기 바란다.
*/

