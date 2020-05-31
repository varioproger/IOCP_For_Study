/*
다음의 함수를 이용해 소켓을 닫는 게 아닌 접속만 끊고 추후에 재사용 가능한 형태로 만들 수 있다.

BOOL PASCAL DisconnectEx
(
  _In_  SOCKET    hSocket,
  _In_  LPOVERLAPPED  lpOverlapped,
  _In_  DWORD     dwFlags,
  _In_  DWORD     reserved
);

SOCKET hSocket
  AcceptEx나 ConnectEx 함수를 통해 클라이언트와 연결된 소켓이다.

LPOVERLAPPED  lpOverlapped
   OVERLAPPED 구조체에 대한 포인터다. 이 말은 DisconnectEx 함수 역시 비동기로 수행할수 있음을 의미한다. 
   이 매개변수를 NULL로 넘겨주면 동기 함수로 실행된다.

DWORD dwFlags
   소켓 hSocket을 재사용하기 위해서는 TF_REUSE _SOCKET 플래그를 사용하라. 
   TF_REUSE_SOCKET의 의미는 소켓 연결 해제 작업이 완료된 후, 나중에 이 소켓을 AcceptEx나ConnectEx 함수 호출 시 다시 사용하겠다는 의미다. 
   그런 목적이 아닌 경우에는 0을 넘겨주면 된다.

DWORD  reserved
    예약된 매개변수다. 반드시 0으로 넘겨줘야 하며, 그렇지 않으면 WSAEINVAL 에러가 리턴된다.

EchoSvrExIocp 소스를 참조하세요
*/
