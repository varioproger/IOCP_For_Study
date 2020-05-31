/*
AcceptEx 함수와 accept 함수의 가장 큰 차이는 중첩된 입출력의 수행 가능 여부다. 
그 외의 또 다른 중요한 차이는 AcceptEx 함수의 경우는 호출자가 두 개의 소켓, 
즉 리슨할 소켓과 접속을 수용할 소켓을 미리 준비해야 한다는 것이다.

BOOL PASCAL AcceptEx
(
  _In_   SOCKET     sListenSocket,
  _In_   SOCKET     sAcceptSocket,
  _In_   PVOID     lpOutputBuffer,
  _In_   DWORD     dwReceiveDataLength,
  _In_   DWORD     dwLocalAddressLength,
  _In_   DWORD     dwRemoteAddressLength,
  _Out_    LPDWORD     lpdwBytesReceived,
  _In_   LPOVERLAPPED  lpOverlapped
);

SOCKET sListenSocket
  socket 또는 WSASocket으로 생성한 리슨용 소켓 핸들이다.

SOCKET sAcceptSocket
   들어오는 접속을 수용할 소켓이다. 
   내부적으로 접속을 수용해서 소켓을 생성시켜 리턴해주는 accept 함수와는 달리, 
   AcceptEx 함수의 경우는 미리 접속을 수용할 소켓을 준비해야 한다. 
   따라서, 이 소켓은 바인드되거나 연결된 소켓이어서는 안 된다.

PVOID  lpOutputBuffer
DWORD dwReceiveDataLength
DWORD dwLocalAddressLength
DWORD dwRemoteAddressLength
   accept 함수와는 다르게 버퍼를 위한 포인터를 전달해줘야 한다. 
   lpOutputBuffer 매개변수는 로컬 주소와 리모트 주소를 담기 위한 버퍼를 의미하며 NULL이 될 수 없다. 
   accept 함수의 경우, 접속을 요구한 리모트의 주소를 담기 위해 매개변수로 SOCKADDR 구조체의 포인터를 요구하지만 
   NULL로 넘기면 리모트 주소 없이 자식 소켓을 획득할 수 있다. 
   하지만 AcceptEx 함수에서의 lpOutputBuffer 매개변수는 NULL이 될 수 없다. 
   주소 획득을 위한 버퍼의 크기를 알려주기 위해 dwLocalAddressLength는 로컬 주소의 길이를, 
   dwRemoteAddressLength는 리모트 주소의 길이를 지정한다. 
   다음으로, lpOutputBuffer 매개변수는 또 다른 용도가 있다. 
   accept 함수처럼 주소 획득이 목적이라면 타입이 SOCKADDR 구조체의 포인터여야 하겠지만, 
   AcceptEx 함수의 경우 그 타입은 void 형의 포인터다. lpOutputBuffer 매개변수는 주소 획득뿐만 아니라 리모트가 접속 후 
   최초로 보내는 데이터가 있다면 접속 수용과 더불어 그 데이터까지 받을 수 있도록 한다. 
   그리고 접속 후 전송되는 최초 데이터를 수신하기 위한 버퍼의 크기를 dwReceiveDataLength 매개변수로 지정하게 된다. 
   따라서 lpOutputBuffer 매개변수가 가리키는 버퍼의 실제 크기는 dwReceiveDataLength 와 dwLocalAddressLength, 
   그리고 dwRemoteAddressLength의 값을 합한 것보다 무조건 큰 값이어야 한다.

LPDWORD  lpdwBytesReceived
   AcceptEx 함수 호출 후 실제로 받은 데이터의 크기를 돌려준다. 만약 lpOverlapped 매개변수를 사용하면 이 매개변수는 무시된다. 
   따라서 이 경우엔 NULL로 넘겨주면 된다.

LPOVERLAPPED  lpOverlapped
   OVERLAPPED 구조체의 포인터다. 이 매개변수가 NULL이면 AcceptEx 함수는 동기적으로 움직이며, accept 함수를 사용하는 것과 동일해진다.

[반환값] BOOL
   다른 비동기 입출력 함수와 마찬가지로, FALSE가 리턴되면 WSA_IO_PENDING 이나 ERROR_IO_PENDING 여부를 체크해야 한다.

AcceptEx나 ConnectEx 함수를 호출해 접속 대기 중인 상태에서 이 함수로 넘겨준 소켓 핸들을 닫게 되면, 
Internal 필드의 상태 코드는 ERROR_OPERATION_ABORTED 에 해당하는 STATUS_CANCELLED(0xC0000120 )로 설정된다.

ReadFile/WSARecv 또는 WriteFile/WSASend 함수의 경우는 이미 연결된 소켓에 대해 closesocket 함수를 호출한다. 
closesocket 함수는 내부적으로 입출력 중이라면 취소를 요청하고 연결 상태면 연결을 끊는다. 
따라서 연결된 소켓의 경우 로컬에서 의도적으로 연결을 끊었으므로 최종적으로 STATUS _LOCAL _DISCONNECT가 상태 코드로 지정되지만, 
AcceptEx 함수의 경우는 연결된 소켓이 아니라 연결을 대기하는 소켓이므로 closesocket 함수가 연결 대기를 취소하게 되며, 
최종적으로 상태 코드는 입출력 취소를 의미하는 STATUS_CANCELLED가 된다. 
그러므로 GetQueuedCompletedStatus 또는 GetOverlappedResult 대기 상태에서 깨어났을 때의 상태 코드 체크는 
확장 API를 사용했을 경우에는 STATUS_CANCELLED 상태 코드에 대한 체크도 함께 해줘야 한다.

#define STATUS_CANCELLED   ((NTSTATUS)0xC0000120L)
Internal 필드의 상태 코드 체크 결과값이 STATUS_LOCAL_DISCONNECT와 STATUS_CANCELLED인 경우에는 
두 상태 코드 모두 로컬에서 직접 연결된 또는 연결 대기 중인 자식 소켓을 닫는 경우로 간주할 수 있으므로 이 점을 염두에 두기 바란다

AcceptEx 함수는 여러 개의 소켓 함수를 단 한 번의 커널 전환을 요구하는 하나의 함수로 결합한다. 
호출에 성공했을 경우, AcceptEx 함수는 다음 세 가지 작업을 수행한다.

  ●   새로운 연결을 수용한다.
  ●   새로 맺어진 연결에 대한 로컬 주소와 리모트 주소를 돌려준다.
  ●   리모트에서 전송된 첫 번째 데이터 블록을 수신한다.

dwLocalAddressLength와 dwRemoteAddresssLength는 주소를 위한 버퍼의 크기를 지정하기 위한 매개변수며, 
TCP 기반의 주소라면 SOCKADDR_IN 구조체의 크기 이상이어야 한다. 
정확히 말하자면, 사용한 소켓 주소 구조체 크기에 16을 더한 값이어야만 하며, 다음과 같이 넘겨줘야 한다.
dwAddressLength ← sizeof(SOCKADDR_IN) + 16

dwReceiveDataLength는 접속과 동시에 전송될 수도 있을 데이터를 받기 위한 버퍼의 크기를 의미한다.
일반적으로 C/S 구조로 솔루션을 구현하다 보면 리모트 인증이나 초기 정보 전달 등의 다양한 이유로 
클라이언트가 접속과 동시에 최초의 데이터를 보내는 경우가 많다. 이럴 경우를 대비해 접속과 동시에 그 데이터를 받을 목적으로 이용된다. 
반대로 ConnectEx 함수에서는 접속 성공과  더불어 전송할 데이터를 지정하는 길이 매개변수가 존재한다. 
물론 그럴 필요가 없다면 이 매개변수를 0으로 넘기면 된다. 이런 기능은 편리하기는 하지만 악의적으로 접속만 일으켜서 
서버에 부하를 주려는 공격에 취약할 수 있다. 
dwReceiveDataLength가 0이 아니면 시스템은 접속 후 부가적인 데이터를 받을 때까지 기다리며, 
실제 데이터가 수신되어야만 리슨 소켓은 시그널 상태가 된다. 그러므로 이 옵션을 사용할 때는 신중해야 한다.

넘겨줄 버퍼의 실제 크기는 다음의 조건을 충족해야 하며, 그렇지 않으면 에러가 발생한다.

실제 버퍼 크기 ≥ dwLocalAddressLength + dwRemoteAddresssLength + dwReceiveDataLength;



실제 사용 방법은 17_GetAcceptExSockaddrs 를 읽고난 후에 EchoSvrEx 소스를 참조하세요.
EchoSvrEx소스를 참조 했다면 알겠지만 

클라이언트를 실행하면 클라이언트가 connect 함수를 호출하게 되고,
서버는 곧바로 대기 상태에서 풀려 접속 메시지와 로컬 및 리모트 주소를 출력할 것이다.

이것에 의미는 AcceptEx 함수를 사용하면 리슨 소켓까지도 바로 장치 시그널링용으로 사용 가능하다

DWORD   dwRecvLen =  256 주석을 풀어서 실행을 할 경우 :
실행 후 클라이언트를 실행만 시킨 상태에서 가만히 있으면 서버 콘솔에서는 클라이언트의 접속을알리는 메시지가 출력되지 않을 것이다.
시간이 좀 지나서 클라이언트 콘솔에 문자열을 입력해 전송하면, 그제서야 클라이언트 접속 메시지 및 로컬/리모트 주소,
그리고 입력한 문자열이 서버 콘솔에 표시된다.

AcceptEx의 네 번째 매개변수인 dwReceiveDataLength에 0이 아닌 그 이상의 값을 넘겨주면, 
비록 접속을 했더라도 클라이언트로부터 어떤 데이터가 전송되기 전까지 대기 상태에서 계속 머물러있게 된다는 것을 의미한다. 
‘dwReceiveDataLength > 0’의 의미는 접속과 동시에 클라이언트로부터 최초의 데이터를 수신하겠다는 것이며,
따라서 입출력 완료의 의미는 접속 후 데이터가 도착해야만 한다는 것이다.
그렇다면 ‘dwReceiveDataLength > 0’인 상황에서, 클라이언트를 실행 후 계속 아무런 데이터도 보내지 않는다면 아마도 계속 대기 상태로 머무를 것이다.
어떤 악의적인 의도를 가지고 접속만 한 상태에서 데이터를 보내지 않고 있는 대량의 클라이언트가 있다면 위의 상황은 문제가 된다. 
이런 상황을 체크하기 위해 다음의 수단을 제공한다. AcceptEx 함수를 호출한 후 일정 시간이 지나도록 계속 대기 상태에 머물러 있을 경우,
SO_CONNECT_TIME 옵션과 함께 getsockopt 함수를 호출하면 AcceptEx 함수의 자식 소켓의 상태를 확인할 수 있다.

getsockopt(sock, SOL_SOCKET, SO_CONNECT_TIME, (char*)&dwSecs, (PINT)&nBytes);

위와 같이 getsockopt 함수를 호출하고 그 호출이 성공했을 경우, dwSecs 매개변수는 다음의 정보를 담고 있다.
  ● dwSecs == 0xFFFFFFFF : 클라이언트가 아직 접속하지 않았다.
  ● dwSecs > 0 : 클라이언트가 접속한 후 dwSecs 초만큼 경과했다.

이때 dwSecs에 담긴 값은 초 단위의 경과 시간을 의미한다. 위와 같이 호출해 접속한 후, 
몇 초 이상 경과하도록 데이터 전송이 없다면 소켓을 강제로 닫아버림으로써 리소스 낭비 및 악의적인 접속에 대응할 수 있다.

EchoSvrEx_2 소스를 참고 하세요

 */

/*
accept 함수의 경우, 접속을 요구한 리모트의 주소를 담기 위해 매개변수로 SOCKADDR 구조체의 포인터를 요구하지만
NULL로 넘기면 리모트 주소 없이 자식 소켓을 획득할 수 있다.
이부분이 신경쓰여서 진짜로 해보니까 된다....
TEST_0 폴더에 있는 소스를 확인해보세요
*/