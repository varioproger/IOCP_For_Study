/*
int WSAIoctl
(
    _In_   SOCKET      s,
    _In_   DWORD      dwIoControlCode,
    _In_   LPVOID      lpvInBuffer,
    _In_   DWORD      cbInBuffer,
    _Out_   LPVOID      lpvOutBuffer,
    _In_   DWORD      cbOutBuffer,
    _Out_   LPDWORD     lpcbBytesReturned,
    _In_   LPWSAOVERLAPPED  lpOverlapped,
    _In_   LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

많은 매개변수 중 눈여겨볼 매개변수는 dwIoControlCode다. 
우리의 목적은 connect나 accept등의 동기 함수에 대응하는 비동기 연결 및 접속 수용 함수의 포인터를 획득하는 것이다. 
이 목적을 위해 dwIoControlCode 매개변수로 전달해야 할 컨트롤 코드는 SIO_GET_EXTENSION_FUNCTION_POINTER 다.

lpvInBuffer 매개변수는 원하는 확장 API의 함수 포인터를 식별하는 GUID를 지정하고,
cbInBuffer는 이 GUID의 크기인 sizeof (GUID)를 지정한다. 
성공하면 lpvOutbuffer에 해당 함수의 포인터를 돌려준다. 

다음은 얻고자 하는 함수와 관련된 GUID를 정리하였다.
GUID(lpvInBuffer)              API                      함수 포인터(lpvOutBuffer)
WSAID_ACCEPTEX                 AcceptEx                 LPFN_ACCEPTEX
WSAID_CONNECTEX                ConnectEx                LPFN_CONNECTEX
WSAID_DISCONNECTEX             DisconnectEx             LPFN_DISCONNECTEX
WSAID_GETACCEPTEXSOCKADDRS     GetAcceptExSockaddrs     LPFN_GETACCEPTEXSOCKADDRS
WSAID_TRANSMITFILE             TransmitFile             LPFN_TRANSMITFILE
WSAID_TRANSMITPACKETS          TransmitPackets          LPFN_TRANSMITPACKETS
WSAID_WSARECVMSG               WSARecvMsg               LPFN_WSARECVMSG
WSAID_WSASENDMSG               WSASendMsg               LPFN_WSASENDMSG


다음과 같이 WSAIoctl을 호출하면 소켓 확장 API의 함수 포인터를 획득할 수 있다.
다음 코드는 비동기 accept 함수에 해당하는 AcceptEx 함수의 함수 포인터를 획득하는 예다.

그전에 GetSockExtAPI 함수를 정의해야한다.
이 함수는 소켓이랑 GUID를 WSAIoctl함수를 이용해 합쳐서 리턴한다.

PVOID GetSockExtAPI(SOCKET sock, GUID guidFn)
{
  PVOID   pfnEx =  NULL;
  GUID   guid =  guidFn;
  DWORD   dwBytes =  0;
  LONG lRet =  WSAIoctl
  (
   sock,
   SIO_GET_EXTENSION_FUNCTION_POINTER,
   &guid, sizeof(guid),
   &pfnEx, sizeof(pfnEx),
   &dwBytes, NULL, NULL
  );
  if (lRet == SOCKET_ERROR)
   return NULL;
  return pfnEx;
}

사용방법은 : 
LPFN_ACCEPTEX pfnAcceptEx =  (LPFN_ACCEPTEX)GetSockExtAPI(sock, WSAID_ACCEPTEX);

*/
