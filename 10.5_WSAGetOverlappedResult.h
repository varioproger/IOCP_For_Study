/*
전송 바이트 수와 에러 발생 시 에러 코드를 쉽게 얻을 수 있는 전용 대기 함수
BOOL WINAPI GetOverlappedResult
(
  _In_    HANDLE     hFile,
  _In_    LPOVERLAPPED   lpOverlapped,
  _Out_   LPDWORD    lpNumberOfBytesTransferred,
  _In_    BOOL      bWait
);

HANDLE hFile
CreateFile로 연 장치의 핸들값이나 OVERLAPPED 구조체의 hEvent 필드값이다.
물론 이 장치는 FILE_FLAG_OVERLAPPED로 열린 장치여야 한다.

LPOVERLAPPED  lpOverlapped
ReadFile/WriteFile의 마지막 매개변수로 넘겨준 OVERLAPPED 구조체의 포인터값이다.

LPDWORD  lpNumberOfBytesTransferred
입출력 과정에서 실제로 유저의 버퍼로 읽어들이거나 장치로 쓴 바이트 수다. 
이 값은 OVERLAPPED 구조체의 InternalHigh 값과 동일하다.

BOOL bWait
입출력이 진행 중일 때, 입출력이 완료될 때까지 대기할 것인지의 여부를 지정하는 플래그다. 
이 매개변수가 TRUE면 입출력이 완료될 때까지 대기하고, FALSE면 입출력 중이라도 바로 리턴한다. 
이때 GetLastError의 값은 ERROR_IO_INCOMPLETE(996)가 된다.

[반환값] BOOL
성공이면 TRUE를 리턴하고 실패면 FALSE를 리턴한다. 
bWait 매개변수를 FALSE로 지정했을 때 만약 입출력이 진행 중이라면 FALSE가 리턴되고, 
이때 GetLastError의 값은 ERROR_IO_INCOMPLETE(996L)가 된다. 
따라서 에러 코드를 체크함으로써 현재 입출력의 진행 여부를 판단할 수 있다.

사용 예
HANDLE hSrcFile = CreateFile
(
   argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING,
  FILE_FLAG_OVERLAPPED, NULL
);

OVERLAPPED ro =  { 0, };
ro.Offset =  ro.OffsetHigh = 0;
ro.hEvent = NULL;
OVERLAPPED 구조체를 정의한 후 내부 필드를 전부 0으로 초기화한다. 
여기서는 오프셋 관련 필드와 이벤트 필드를 반드시 초기화시켜야 한다는 것을 강조하기위해 
ro의 Offset과 OffsetHigh, 그리고 hEvent 필드를 한 번 더 0으로 초기화시켜 준다.

DWORD dwReadBytes = 0;
bIsOK =  GetOverlappedResult(hSrcFile, &ro, &dwReadBytes, TRUE);
if (!bIsOK)
{
   dwErrCode =  GetLastError();
   if (dwErrCode == ERROR_HANDLE_EOF)
    dwErrCode =  0;
   break;
}
ro.Offset += dwReadBytes;

GetOverlappedResult 함수의 마지막 매개변수 bWait를 FALSE로 넘겨서 단순히 입출력이 현재 진행 중인지의 여부를 간단하게 판단할 수 있다. 
매개변수 bWait를 FALSE로 지정하여 GetOverlappedResult를 호출한 후 리턴값과 그 에러 코드를 체크하면 된다
DWORD dwReadBytes = 0;
BOOL bIsOK = GetOverlappedResult(hSrcFile, &ro, &dwReadBytes, FALSE);
if (!bIsOK)
{
    dwErrCode = GetLastError();
    if (dwErrCode == ERROR_IO_INCOMPLETE);
    {
        현재 입출력이 완료되지 않았다.
    }
    else
    {
        다른 문제에 따른 에러가 발생했다.
    }
}
else
{
    입출력 작업이 완료되었다.
}


윈속에서는 WSAGetOverlappedResult 함수가 제공된다.
BOOL WSAAPI WSAGetOverlappedResult
(
  _In_   SOCKET      s,
  _In_   LPWSAOVERLAPPED  lpOverlapped,
  _Out_   LPDWORD     lpcbTransfer,
  _In_   BOOL       fWait,
  _Out_   LPDWORD     lpdwFlags
);
마지막 매개변수 lpdwFlags가 추가된 것을 제외하면 매개변수의 구성은 GetOverlappedResult함수와 동일하다. 
lpdwFlags는 WSASend나 WSARecv 함수 호출 시에 매개변수로 전달하는 dwFlags 값을 받아온다.

GetOverlappedResult 함수와 WSAGetOverlappedResult 차이점은 에러처리에 있다.

GetOverlappedResult는 동일하게 ERROR_NETNAME_DELETED 나온다.

WSAGetOverlappedResult는 에러코드는 2가지 상태 코드로 놔뉜다.
“NTStatus.h”에 정의되어 있다.
#define STATUS_LOCAL_DISCONNECT   ((NTSTATUS)0xC000013BL)
#define STATUS_REMOTE_DISCONNECT    ((NTSTATUS)0xC000013CL)

① 서버에서 소켓 닫음 STATUS_LOCAL_DISCONNECT WSAECONNABORTED (10053)
② 클라이언트 강제 종료 STATUS_REMOTE_DISCONNECT WSAECONNRESET (10054)

#ifndef STATUS_LOCAL_DISCONNECT
#  define STATUS_LOCAL_DISCONNECT   ((NTSTATUS)0xC000013BL)
#endif
#ifndef STATUS_REMOTE_DISCONNECT
#  define STATUS_REMOTE_DISCONNECT   ((NTSTATUS)0xC000013CL)
#endif

이렇게 정의를 미리 해두는 편이 좋다.

WSAEventSelect_Upgrade 소스 참조
*/
