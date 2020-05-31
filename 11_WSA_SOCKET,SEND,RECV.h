/*
SOCKET WSASocket
(
    _In_ int        af,
    _In_ int        type,
    _In_ int        protocol,
    _In_ LPWSAPROTOCOL_INFO   lpProtocolInfo,
    _In_ GROUP        g,
    _In_ DWORD        dwFlags
);
WSASocket은 중첩 속성 여부를 지정할 수 있다.

int af
int  type
int protocol
   위의 세 매개변수는 socket 함수의 매개변수와 동일하다. 
   각각 주소 패밀리와 소켓 타입, 그리고 프로토콜을 나타낸다. 
   socket 함수 설명 시에도 그랬던 것처럼 현재 우리의 주된 논의는 IPv4 기반의 TCP 기반이므로, 
   각각 af ← AF_INET, type ← SOCK_STREAM, protocol ← IPPROTO_TCP 또는 0을 지정하는 것으로 하자.
LPWSAPROTOCOL_INFO  lpProtocolInfo
GROUP g
   각각 상세 지정 정보를 설정하지만, 이 책의 주제와는 상관이 없으므로 무시한다. 
   lpProtocolInfo 매개변수는 NULL로, g 매개변수는 0으로 지정한다.
DWORD dwFlags
   가장 중요한 요소가 될 것이다. 
   소켓 자체를 비동기 입출력의 대상으로 생성하기 위해서는 WSA_FLAG_OVERLAPPED 플래그를 지정하면 된다. 

SOCKET sock = WSASocket
(
    AF_INET, SOCK_STREAM, IPPROTO_TCP,
    NULL, 0, WSA_FLAG_OVERLAPPED
);
↕↕
SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


int WSARecv
(
    _In_    SOCKET      s,
    _Inout_ LPWSABUF     lpBuffers,
    _In_    DWORD      dwBufferCount,
    _Out_   LPDWORD     lpNumberOfBytesRecvd,
    _Inout_ LPDWORD     lpFlags,
    _In_    LPWSAOVERLAPPED lpOverlapped,
    _In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);
int WSASend
(
    _In_   SOCKET      s,
    _In_   LPWSABUF      lpBuffers,
    _In_   DWORD       dwBufferCount,
    _Out_   LPDWORD      lpNumberOfBytesSent,
    _In_   DWORD       dwFlags,
    _In_   LPWSAOVERLAPPED   lpOverlapped,
    _In_   LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

SOCKET s
송수신에 사용할 소켓 핸들이다.

LPWSABUF  lpBuffers
DWORD dwBufferCount
ReadFile / WriteFile 함수와는 달리 소켓 버퍼 구조를 별도로 가져간다.
WSABUF 구조체는 다음과 같다.
typedef struct __WSABUF
{
    u_long     len;
    char FAR* buf;
} WSABUF, * LPWSABUF;
buf는 전송할 데이터를 담고 있는 바이트 스트림이며, 
len은 그 길이를 바이트 단위로 담는 변수다. 
그리고 lpBuffers 매개변수가 WSABUF 구조체의 포인터임을 확인하기 바란다. 
즉 이는 WSABUF 구조체를 하나만 보낼 수 있는 것이 아니라 WSABUF 구조체의 배열로, 즉 여러 데이터 조각을 보낼 수 있다는 것을 의미한다. 
그리고 이때 WSABUF 구조체 배열의 원소의 개수를 의미하는 매개변수가 dwBufferCount가 된다.

LPDWORD  lpNumberOfBytesRecvd /  lpNumberOfBytesSent
데이터를 송신했거나 전송받은 실제 길이를 바이트 수로 담아오는 매개변수다. 
이 매개변수는 ReadFile/WriteFile 함수에서와 마찬가지로, 중첩 입출력을 사용할 경우 NULL로 지정할 수 있다.

LPDWORD  lpFlags / DWORD dwFlags
send/recv 함수의 플래그 매개변수에 몇 가지 더 추가된 옵션을 담고 있다.

LPWSAOVERLAPPED  lpOverlapped
OVERLAPPED 구조체의 포인터다. WSAOVERLAPPED라는 별도의 구조체가 정의되어 있지만 OVERLAPPED 구조체와 그 내용이 동일하다. 
이 매개변수가 NULL이 아니면 비동기 입출력을 이용하겠다는 의미며, 
이 경우 lpNumberOfBytesRecvd/lpNumberOfBytesSent 매개변수는 NULL로 지정할 수 있다.

LPWSAOVERLAPPED_COMPLETION_ROUTINE  lpCompletionRoutine
경보가능 입출력, 즉 APC 큐에 삽입되어 실행될 입출력 완료 루틴 콜백 함수의 포인터를 지정한다. 
dwFlags는 WSASend/WSARecv 함수 호출 시에 매개변수로 전달했던 dwFlags/lpFlags 플래그다. 

그 선언은 다음과 같다.
void CALLBACK CompletionROUTINE
(
  IN DWORD      dwError,
  IN DWORD      cbTransferred,
  IN LPWSAOVERLAPPED lpOverlapped,
  IN DWORD      dwFlags
);
WSASend/WSARecv 함수 호출 시 lpOverlapped 매개변수와 lpCompletionRoutine 매개변수를 모두 NULL로 넘겨주면, 

각각 send/recv 함수를 호출한 것과 동일한 효과를 갖게 된다.
*/
