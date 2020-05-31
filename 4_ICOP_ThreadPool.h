/*
 IOCP를 제대로 사용하기 위해서는 실제로 스레드를 생성하여스레드 풀을 구성해야 한다.

여러분이 생성시킨 스레드들은 IOCP와 직접적인 관련이 없다. 
그 스레드가특정 IOCP 커널 객체와 관련을 맺으려면 스레드 엔트리 함수 내에서 
IOCP 커널 객체의 핸들과 함께 GetQueuedCompletionStatus를 호출해야 한다.
이렇게 GetQueuedCompletionStatus를 호출해 대기 상태로 들어간 모든 스레드들은 
생성된 특정 IOCP와 연관된 스레드 풀을 형성하게된다. 
따라서 스레드 풀에 존재하는 모든 스레드는 GetQueuedCompletionStatus를 호출한 스레드로서 
대기 상태가 되어 더 이상 CPU를 소비하지 않는다. 또한 입출력 완료에 의해 깨어난 스레드는 
스레드 풀에서 빠져나와 활성화되어 입출력 완료에 대한 처리를 수행할 수 있게 된다. 그리고
스레드 풀에서 빠져나올 수 있는 최대 스레드의 수가 바로 IOCP 커널 객체 생성 시에 지정한 “동시실행 가능한 최대 스레드 수”가 된다.

IOCP가 하는 역할은 스레드 풀로부터 꺼낼 수 있는 스레드의 수를 IOCP 생성 시에 지정한 “동시 실행 가능한 스레드 수”만큼 조절하는 일이다.
스레드 풀에서 빠져나온 스레드는 동시에 실행된다.

IOCP와 관련된 스레드의 엔트리 포인트 함수의 작성은 다음과 같은 전형적인 패턴을 지닌다.

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

스레드는 생성된 후 즉시 GetQueuedCompletionStatus를 호출함으로써 스스로 대기 상태로, 즉 스레드 풀로 들어간다. 
그러다가 입출력이 완료되면 스레드는 GetQueuedCompletionStatus로부터 리턴되어
(다시 말해 스레드 풀에서 빠져나와) 리턴 결과가 TRUE면 입출력 완료에 대한 결과 처리를 수행한 후 루프를 돌고 
다시 GetQueuedCompletionStatus를 호출해 또 대기 상태로 들어가는, 이러한 과정을 무한히 반복한다. 
이렇게 형성된 스레드 풀의 활용을 극대화시키기 위해서는 대량의 장치들을 IOCP에 연결시켰을 때 
스레드 풀에서 빠져나와 결과 처리를 수행하는 간격을 가능하면 짧게 하는 것이 중요하다.

스레드 풀을 구성하는 스레드 엔트리 함수를 작성할 때, GetQueuedCompletionStatus에 대한 에러 체크는 반드시 해야 한다.
실제 에러 발생 여부는 OVERLAPPED 구조체의 Internal 필드를 통해 확인한다. 
GetQueuedCompletionStatus는 장치에서 에러가 발생하면 FALSE를 리턴한다. 
대신 *lpOverlapped가 NULL인 경우는GetQueuedCompletionStatus 자체의 에러고, 
*lpOverlapped가 NULL이 아닌 경우는 장치에서 에러가 발생했음을 판단할 수 있다.

● ● ● *lpOverlapped == NULL
   GetQueuedCompletionStatus 리턴 결과, 입출력 완료 큐로부터 입출력 완료 패킷을 데큐하지 않았다. 
   이 경우lpNumberOfBytes, lpCompletionKey 매개변수 역시 정의되지 않는다. 
   함수 자체에 대한 에러로, 예를 들어 타임아웃 발생(WAIT_TIMEOUT)이나 IOCP 핸들을 닫았을 경우(ERROR_ABANDONED_WAIT_0)에 발생한다.

● ● ● *lpOverlapped != NULL
   IOCP와 연계된 장치 자체에서 입출력 과정에 에러가 발생했으며, 입출력 완료 큐로부터 해당 입출력 완료 패킷이 데큐된 상태다. 
   따라서 lpNumberOfBytes, lpCompletionKey 매개변수 모두 적절한 값으로 채워진다. 
   lpOverlapped의 Internal 필드는 장치의 상태 코드를 담고 있으며, GetLastError를 호출하면 
   그 상태 코드에 매치되는 Win32 에러 코드를 획득할 수 있다.

 GetQueuedCompletionStatus 호출 결과, 리턴값이 FALSE인 경우에 대한 에러 체크

 BOOL bIsOK = GetQueuedCompletionStatus
(
    hIocp, &dwTrBytes, &ulKey, &pov, 5000
);
if (bIsOK)
{
    성공 ⇒ 완료된 입출력 요구에 대한 처리를 행할 수 있다.
}
else
{
    에러 발생 ⇒ 다음의 패턴을 따른다.
        DWORD dwErrCode = GetLastError();
    if (pov != NULL)
    {
        입출력 완료 요구 처리 시 에러가 발생했다.pov의 Internal 필드에 NTSTATUS 상태 코드가 설정되어 있으며,
        dwErrCode는 이 상태 코드를 윈도우 에러 코드로 변환된 값을 갖는다.
    }
    else
    {
        GetQueuedCompletionStatus 자체의 에러
            if (dwErrCode == WAIT_TIMEOUT)
            {
                여러분이 지정한 dwMilliseconds만큼의 시간이 경과했다.
            }
            else
            {
                GetQueuedCompletionStatus 호출 자체에 문제가 있다.
            }
    }
}

IOCP는 하나의 장치와 연결된 것이 아니라 여러 장치와 연결되어 있다.
따라서 위와 같이 에러 발생한 후 pov의 체크 없이 단순히 루프를 탈출하게 되면, 
스레드를 종료하게 될 뿐만 아니라 하나의 장치에서의 에러로 인해 
연결된 다른 여러 장치로부터 더 이상의 입출력 완료 통지를 받아들일수 없게 된다.

pov가 NULL이 아닌 경우에는 해당 장치 핸들에 대한 에러 처리만을 수행한 후 
IOCP에 연결된 다른 장치의 입출력을 받아들이기 위해 계속 루프를 돌면서 GetQueuedCompletionStatus를 호출해야 한다.

IOCP에 대해 대기 중인 스레드들은 GetQueuedCompletionStatus 호출 시 넘겨지는 IOCP 핸들의 상태에 따라 깨어나든지 대기할 것이다.

대기 상태에서 깨어난 스레드는 입출력 완료가 발생한 장치에 대한 식별을
CreateIoCompletionPort를 통해서 장치를 IOCP와 연계시킬 때 정의한 생성키를 통해서 한다.

IOCP는 큐로부터 패킷을 FIFO 방식으로 추출한 후, 대기하고 있는 스레드에게 하나씩 처리를 맡긴다.
입출력 완료 패킷의 각 필드값을 GetQueuedCompletionStatus의 출력 매개변수로 넘어온 매개변수들에 설정한 후 
GetQueuedCompletionStatus를 리턴시킨다. 패킷의 필드 중 생성키 필드도 
GetQueuedCompletionStatus의 lpCompletionKey 매개변수에 설정되며, 
따라서 이 매개변수를 통해 장치를 식별할 수 있는 것이다.

스레드 풀을 구성할 스레드들의 생성 시점은 개개의 장치를 열기 전에 생성시키는 것이 좋다. 
즉 IOCP 커널 객체를 생성시킨 후, 이 객체의 핸들에 대해서 GetQueuedCompletionStatus를 호출하는 스레드들을 미리 생성시켜 두라.


스레드는 무한 루프를 돌면서 반복적으로 GetQueuedCompletionStatus를 호출하는 것을 볼수 있다.
추가해서 언급할 내용은 일단 GetQueuedCompletionStatus에서 깨어나 입출력 완료에 대한 처리를 마쳤다면,
해당 스레드가 계속해서 비동기 입출력 완료에 대한 처리를 하기 위해서는 다시 한 번 더 비동기 입출력을 수행하는 함수,
즉 ReadFile/WriteFile을 호출한 후 루프를 돌아 GetQueuedCompletionStatus의 대기 상태로 들어가야 한다.

DWORD WINAPI IOCPThreadProc(LPVOID pParam)
{
    ⋮
        while (true)
        {
            GetQueuedCompletionStatus(...);
            IOCP 커널 객체에 대해 GetQueuedCompletionStatus를 호출해 대기 상태로 들어간다.

                ☞ 완료 처리 가능 상태
                GetQueuedCompletionStatus로부터 성공적으로 리턴될 경우에는 입출력 처리가 완료되었음을 의미하므로, 그것에 대한 적절한 처
                리 작업을 수행한다.그 “적절한 처리 작업”을 가능한 짧은 시간 내에 완료하는 것이 IOCP 사용의 중요한 요소가 된다.
                ⋮
                ReadFile(...);
            완료 처리를 마친 후 다시 비동기 입출력을 수행한다.즉 ReadFile / WriteFile을 비동기적으로 호출한다.이 함수들은 비동기적이기 때문
                에 블록되지 않고 바로 리턴되어 루프를 돌아 다시 GetQueuedCompletionStatus를 호출한다.다음 입출력 완료가 있을 때까지 대기
                상태로 머물러 있을 것이다.
        }
    return 0;
}
이렇게 스레드를 생성했다면 이 스레드를 우선 실행시켜 대기 상태로 만들어 놓은 후 
메인 스레드나다른 스레드 등에서 최초의 비동기 입출력을 수행하는 것이 중요하다

#define READ_KEY	1
#define WROTE_KEY	2

struct COPY_CHUNCK : OVERLAPPED
{
    HANDLE	_hfSrc, _hfDst; 읽기 쓰기 용
    BYTE	_arBuff[BUFF_SIZE];

    COPY_CHUNCK(HANDLE hfSrc, HANDLE hfDst)
    {
        memset(this, 0, sizeof(*this));
        _hfSrc = hfSrc, _hfDst = hfDst;
    }
};

typedef COPY_CHUNCK* PCOPY_CHUNCK;

struct COPY_ENV
{
    HANDLE	_hIocp;
    LONG	_nCpCnt;
    HANDLE	_hevEnd;
};
typedef COPY_ENV* PCOPY_ENV;

if (ulKey == READ_KEY) //다 읽었으면
{
    printf(" => Thr %d Read bytes : %d\n", dwThrId, pcc->Offset);

    bIsOK = WriteFile //쓰기 시작
    (
        pcc->_hfDst, pcc->_arBuff, dwTrBytes, NULL, pcc
    );
}
else //다 썼으면
{
    pcc->Offset += dwTrBytes;
    printf(" <= Thr %d Wrote bytes : %d\n", dwThrId, pcc->Offset);

    bIsOK = ReadFile //읽기 시작
    (
        pcc->_hfSrc, pcc->_arBuff, BUFF_SIZE, NULL, pcc
    );
}

env._hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);
//IOCP 생성

HANDLE hSrcFile = CreateFile(...)
HANDLE hDstFile = CreateFile(...)
//비동기 핸들 생성

CreateIoCompletionPort(hSrcFile, env._hIocp, READ_KEY, 0);
CreateIoCompletionPort(hDstFile, env._hIocp, WROTE_KEY, 0);
// 핸들, IOCP 연결

PCOPY_CHUNCK pcc = new COPY_CHUNCK(hSrcFile, hDstFile);

HANDLE harWorks[2];
for (int i = 0; i < 2; i++)
    harWorks[i] = CreateThread(NULL, 0, IOCPCopyProc, &env, 0, &dwThrID);
//쓰래드 생성 및 IOCP 연결

BOOL bIsOK = ReadFile
(
    pcc->_hfSrc, pcc->_arBuff, BUFF_SIZE, NULL, pcc
);
//처음은 read 먼저 시작하기 때문에 함수 호출

이 코드에 의미는 read를 다하면 write 쓰레드를 깨우고 write를 다하면 read를 깨운다.

IOCP 에러를 처리 하는 방식
if (!bIsOK)
{
    DWORD dwErrCode = GetLastError();
    if (pov == NULL)
    {
        if (dwErrCode != ERROR_ABANDONED_WAIT_0)
            cout << "GQCS failed: " << dwErrCode << endl;
        break;
       
       정상
            적인 종료임을 판별할 수 있다.
    }
    else
    {
        cout << "Internal error: " << dwErrCode << endl;
        continue;
        비동기 감시 중 해당 장치에서 에러가 발생했다.
    }
}





*/

