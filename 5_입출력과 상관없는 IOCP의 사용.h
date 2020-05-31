/*
BOOL WINAPI PostQueuedCompletionStatus
(
  _In_       HANDLE     CompletionPort,
  _In_       DWORD     dwNumberOfBytesTransferred,
  _In_       ULONG_PTR    dwCompletionKey,
  _In_opt_   LPOVERLAPPED   lpOverlapped
);

GetQueuedCompletionStatus의 경우는 실제 전송 바이트 수나 생성키, OVERLAPPED구조체의 포인터 매개변수가 출력을 위해, 
즉 IOCP의 큐에 저장된 입출력 완료 패킷의 정보를 돌려주기 위해 사용되지만, 
PostQueuedCompletionStatus는 반대로 IOCP에 새로운 입출력 완료 패킷을 추가하기 위해 사용된다는 점이다.

 PostQueuedCompletionStatus는 사용자가 입출력 완료에 관계 없이 의도적으로 스레드 풀에서 대기 중인 스레드 하나를 깨우고자 할 때 유용하다.

 #define BEGIN_KEY  0

 for (int i = 0; i < lChnCnt; i++)
{
	PCOPY_CHUNCK pcc = arChunk[i];
	BOOL bIsOK = ReadFile
	(
		pcc->_hfSrc, pcc->_arBuff, BUFF_SIZE, NULL, pcc
	);
	if (!bIsOK)
	{
		DWORD dwErrCode = GetLastError();
		if (dwErrCode != ERROR_IO_PENDING)
			break;
	}
}

위에 코드를 PostQueuedCompletionStatus 사용해서 코드를 간결하게 만들수 있다.
for (int i = 0; i < lChnCnt; i++)
{
    PCOPY_CHUNCK pcc = arChunk[i];
    PostQueuedCompletionStatus(env._hIocp, 0, BEGIN_KEY, pcc);
}



PostQueuedCompletionStatus를 이용해 정의된 스레드 메시지들을 그대로 생성키로 사용해 데이터의 종류를 식별하고,
또한 LPOVERLAPPED 매개변수를 OVERLAPPED 구조체와 상관없이 사용할 수 있다.

#define TM_NONE     0
#define TM_STR   100
#define TM_POINT   101
#define TM_TIME   102
#define TM_EXIT   200
언급한 대로 스레드 메시지 정의를 그대로 생성키로 사용한다.

LONG  lCmd = TM_NONE, lSize = 0;
PBYTE pData = NULL;
if (_stricmp(szIn, "time") == 0)
{
	lSize = sizeof(SYSTEMTIME), lCmd = TM_TIME;
	pData = new BYTE[lSize];

	SYSTEMTIME st;
	GetLocalTime(&st);
	memcpy(pData, &st, lSize);
}
else if (_stricmp(szIn, "point") == 0)
{
	lSize = sizeof(POINT), lCmd = TM_POINT;
	pData = new BYTE[lSize];

	POINT pt;
	pt.x = rand() % 1000; pt.y = rand() % 1000;
	*((PPOINT)pData) = pt;
}
else
{
	lSize = strlen(szIn), lCmd = TM_STR;
	pData = new BYTE[lSize + 1];
	strcpy((char*)pData, szIn);
}
PostQueuedCompletionStatus(hIocp, (DWORD)lSize, (ULONG_PTR)lCmd, (LPOVERLAPPED)pData);

DWORD		 dwErrCode = 0;
LPOVERLAPPED pov = NULL;
DWORD		 dwTrBytes = 0;
ULONG_PTR	 ulKey;

BOOL bIsOK = GetQueuedCompletionStatus
(
	hIocp, &dwTrBytes, &ulKey, &pov, INFINITE
);

UINT  msg = (UINT)ulKey;
long  lSize = (long)dwTrBytes;
PBYTE pData = (PBYTE)pov;

switch (msg)
{
case TM_STR:
break;

case TM_POINT:

break;

case TM_TIME:

break;
}
delete[] pData;
}


쓰레드 종료

for (int i = 0; i < 2; i++)
	PostQueuedCompletionStatus(hIocp, 0, TM_EXIT, NULL);
스레드 종료를 위해 생성키에 TM_EXIT를 넘겨 PostQueuedCompletionStatus를 호출한다.


UINT  msg = (UINT)ulKey;
if (msg == TM_EXIT)
break;


PostQueuedCompletionStatus를 이용한 쓰래드 풀 생성 하는 방법은 IOCP_THREAD_POOL 소스 참조

*/



