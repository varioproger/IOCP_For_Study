/*
WSASend/WSARecv 함수에 대한 설명에서 본 것처럼 dwFlags 프로토타입을 갖는다.

void CALLBACK CompletionROUTINE
(
    IN DWORD      dwError,
    IN DWORD      cbTransferred,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN DWORD      dwFlags
);

VOID CALLBACK RecvCallback(DWORD dwErrCode, DWORD dwTranBytes, LPOVERLAPPED pOL, DWORD dwFlags)

EchoSverApc 소스 참조




*/
