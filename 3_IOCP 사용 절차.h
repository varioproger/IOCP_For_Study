/*
1.  IOCP객채를 생성.	

HANDLE hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
IOCP를 생성 해서 HANDLE을 넘긴다.
핸들 값은 여러 모듈에서 참조하기 때문에 임계구역 잘 생성해야한다.

2. CreateThread 
사용할 만큼의 쓰레드를 생성한다.

HANDLE hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);

	DWORD  dwThrID;
	HANDLE harWorks[2];
	for (int i = 0; i < 2; i++)
		harWorks[i] = CreateThread(NULL, 0, IOCPWorkerProc, hIocp, 0, &dwThrID);

		이런식으로 Thread 엔트리로 IOCP 핸들을 넘긴다.

3. BOOL bIsOK = GetQueuedCompletionStatus

DWORD WINAPI IOCPThreadProc(PVOID pParam)
{
    HANDLE    hIocp = (HANDLE)pParam;
    LPOVERLAPPED  pov = NULL;
    DWORD     dwTrBytes = 0;
    ULONG_PTR   ulKey = 0;

    while (true)
    {
        BOOL bIsOK = GetQueuedCompletionStatus
        (
            hIocp, &dwTrBytes, &ulKey, &pov, INFINITE
        );
        if (bIsOK == TRUE)
        {
            완료 처리 가능 상태
        }
        else
        {
            에러 발생, 에러에 대한 처리
        }
    }
    return 0;
}

쓰레드를 생성하자마자 GetQueuedCompletionStatus 함수를 이용해 스레드를 대기 상태로 만든다.


4. CreateFile

    HANDLE hDstFile = CreateFile
        (
            szDstFile, GENERIC_WRITE, 0, NULL,
            CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL
        );

        FILE_FLAG_OVERLAPPED를 이용해 Handle을 만든다.

5. 생성한 객체를 IOCP랑 연결한다.
    연결하기전에 IOCP객채를 꼭 만들어놔야 한다는걸 잊지 마라.

    CreateIoCompletionPort(hSrcFile, env._hIocp, READ_KEY, 0);

6. 비동기를 지원하는 함수 호출(ReadFile 과 WriteFile)
       
    HANDLE hSrcFile = CreateFile
    (
        pszSrcFile, GENERIC_READ, 0, NULL,
        OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL
    );

    OVERLAPPED ro = { 0 };
	DWORD	dwErrCode = 0;
	BYTE	btBuff[BUFF_SIZE];

	while (true)
	{
		BOOL bIsOK = ReadFile
		(
			hSrcFile, btBuff, sizeof(btBuff), NULL, &ro
		);

*/