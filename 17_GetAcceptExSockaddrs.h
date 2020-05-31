/*
AcceptEx 함수를 호출하고 난 후 주소를 쉽게 획득할 수 있도록 제공되는 함수가 GetAcceptExSocketaddrs 라는 함수

void PASCAL GetAcceptExSockaddrs
(
    _In_  PVOID      lpOutputBuffer,
    _In_  DWORD      dwReceiveDataLength,
    _In_  DWORD      dwLocalAddressLength,
    _In_  DWORD      dwRemoteAddressLength,
    _Out_  LPSOCKADDR* LocalSockaddr,
    _Out_  LPINT      LocalSockaddrLength,
    _Out_  LPSOCKADDR* RemoteSockaddr,
    _Out_  LPINT      RemoteSockaddrLength
);

상위 4개의 매개변수는 AcceptEx 함수에서 넘겨주는 매개변수와 동일하며, 
실제로 AcceptEx 함수의 매개변수로 넘긴 값 그대로 이 함수의 매개변수로 넘겨줘야 한다.
나머지 하위 4개의 매개변수는 로컬과 리모트 주소를 얻기 위한 SOCK_ADDR 구조체와 길이를 받을 포인터 변수를 지정한다

AcceptEx 함수를 호출한 후 로컬과 리모트 주소를 얻고자 한다면 다음과 같이 사용하면 된다.

BOOL bIsOK = pfnAcceptEx
(
    hListen, hAccept,
    arBuff, 256,
    sizeof(SOCKADDR_IN) + 16,
    sizeof(SOCKADDR_IN) + 16,
    &dwBytes, (LPOVERLAPPED)this
);

PSOCKADDR psaLoc = NULL, psaRem = NULL;
INT nsiLoc = 0, nsiRem = 0;
pfnGetAcceptExSockaddrs
(
    arBuff, 256,
    sizeof(SOCKADDR_IN) + 16,
    sizeof(SOCKADDR_IN) + 16,
    &psaLoc, &nsiLoc, &psaRem, &nsiRem
);
SOCKADDR_IN saLoc = *((PSOCKADDR_IN)psaLoc);
SOCKADDR_IN saRem = *((PSOCKADDR_IN)psaRem);


자세한  AcceptEx와 GetAcceptExSockaddrs 함수의 사용법은 EchoSvrEx 소스를 참조
*/

