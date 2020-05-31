/*
OVERLAPPED 구조체를 매개변수로 넘겨 사용할 수 있는 구조다.


BOOL PASCAL ConnectEx
(
    _In_      SOCKET         s,
    _In_      const struct sockaddr* name,
    _In_      int          namelen,
    _In_opt_  PVOID         lpSendBuffer,
    _In_      DWORD         dwSendDataLength,
    _Out_     LPDWORD         lpdwBytesSent,
    _In_      LPOVERLAPPED       lpOverlapped
);
SOCKET s
const struct sockaddr* name
int namelen
이 세 개의 매개변수는 connect 함수의 매개변수와 동일하다.
다만 매개변수로 넘길 소켓 s는미리 바인드된 소켓이어야 한다.

PVOID  lpSendBuffer
DWORD dwSendDataLength
AcceptEx와는 반대로, 접속 후 바로 전송할 데이터가 있으면 이 두 매개변수를 사용한다.

LPDWORD  lpdwBytesSent
LPOVERLAPPED  lpOverlapped
AcceptEx 함수와 마찬가지로, lpdwBytesSent는 전송한 데이터가 있으면 실제로 전송된 바이트 수를 의미하며,
lpOverlapped는 비동기 컨넥션 시에 사용된다.lpOverlapped를 사용할 경우 lpdwBytesSent는 무시되며 NULL로 넘겨주면 된다.
lpOverlapped가 NULL이면 동기함수인 connect 함수와 동일한 기능을 한다.

[반환값] BOOL
AcceptEx 함수와 마찬가지로, FALSE가 리턴되면 WSA_IO_PENDING이나 ERROR_IO_PENDING 여부를 체크해야 한다.
ConnectEx와 connect 함수의 차이점은 세 가지다.
● 첫 번째 매개변수인 소켓 s는 ConnectEx에서 반드시 호출 전에 바인드되어야 한다.
● 연결 확립과 함께 원하는 데이터를 전송할 수 있다.
● 비동기로 움직이며, 따라서 소켓 s를 시그널 객체 자체로 사용하거나 IOCP에 연결할 수 있다.

Client 프로젝트에 ConnectEx 소스 참조
여기서 부터는 소스 먼저 보고 오세요

소스를 통해 알 수 있는 것은 bind 함수를 통해서 먼저 소켓을 바인드를 해줬다는 점과 연결시 타임아웃 처리를 쉽게 할 수 있다는 점이다. 
현재는 ConnectEx 함수를 호출한 후 소켓을 대기함수로 넘겨줬지만, IOCP와 연결시키면 더욱더 유연한 처리도 가능하다.
getsockopt 함수의 매개변수로 SO_CONNECT_TIME 옵션을 전달하여 ConnectEx 호출 후의 경과한 시간을 획득할 수도 있다. 
또한, ConnectEx 호출 성공으로 연결된 소켓은 초기 상태로 있으며, 이전에 설정했던 속성이나 옵션은 활성화되지 않는다. 
따라서 이 속성이나 옵션을 활성화시키기 위해서는 AcceptEx 함수처럼 다음과 같이 SO_UPDATE_CONNECT_CONTEXT 옵션과 함께 
setsockopt 함수를 호출해야 한다.

int iResult =  setsockopt(s, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
*/
