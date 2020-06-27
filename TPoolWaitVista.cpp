#include "Windows.h"
#include "time.h"
#include "iostream"
using namespace std;

#define SAMPLE_NO	0


VOID CALLBACK ThreadPoolWaitProc(PTP_CALLBACK_INSTANCE, PVOID pCtx, PTP_WAIT ptpWait, TP_WAIT_RESULT)
{
	DWORD dwDelay = (DWORD)pCtx;
	DWORD dwThrId = GetCurrentThreadId();
	SYSTEMTIME st;

	GetLocalTime(&st);
	printf("=> Work(ThrId : %d) started at : %02d:%02d:%02d.%03d\n",
		dwThrId, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	Sleep(dwDelay * 1000);

	GetLocalTime(&st);
	printf("<= Work(ThrId : %d) ended at   : %02d:%02d:%02d.%03d\n",
		dwThrId, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

void main()
{
	printf("===== MainThread(ID : %d) started...\n\n", GetCurrentThreadId());

#if (SAMPLE_NO == 0)
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	PTP_WAIT ptpWait = CreateThreadpoolWait(ThreadPoolWaitProc, (PVOID)3, NULL);
	//TP_WAIT 객체를 생성한다. 매개변수로 지연 시간을 초 단위로 넘겨준다.

	SetThreadpoolWait(ptpWait, hEvent, NULL);
	//TP_WAIT 객체를 스레드 풀 큐에 추가하고, 이벤트 핸들을 넘겨준다.
	getchar();
	SetEvent(hEvent);
	//키 입력이 들어오면 이벤트를 시그널 상태로 만든다. 그러면 콜백 함수가 호출된다.
	WaitForThreadpoolWaitCallbacks(ptpWait, TRUE);
	CloseThreadpoolWait(ptpWait);

	
#elif (SAMPLE_NO == 1)
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	PTP_WAIT ptpWait = CreateThreadpoolWait(ThreadPoolWaitProc, (PVOID)3, NULL);


	SetThreadpoolWait(ptpWait, hEvent, NULL);
	getchar();
	SetEvent(hEvent);// 첫 번째 시그널링

	SetThreadpoolWait(ptpWait, hEvent, NULL);
	getchar();
	SetEvent(hEvent);// 두 번째 시그널링
	//계속 재사용하려면 반드시 다음과 같이 명시적으로
	//SetThreadpoolWait를 호출해 반복적으로 대기 핸들을 TP_WAIT 객체와 연결시켜줘야 한다.
	WaitForThreadpoolWaitCallbacks(ptpWait, TRUE);
	CloseThreadpoolWait(ptpWait);

#elif (SAMPLE_NO == 2)
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	PTP_WAIT ptpWait = CreateThreadpoolWait(ThreadPoolWaitProc, (PVOID)3, NULL);


	SetThreadpoolWait(ptpWait, hEvent, NULL);

	getchar();
	SetEvent(hEvent);// 첫 번째 시그널링

	getchar();
	SetEvent(hEvent);// 두 번째 시그널링
	//이와 같이 처리하게 되면 첫 번째 시그널링에 대해서는 콜백 함수가 호출되지만, \
	두 번째 시그널링에 대해서는 반응을 하지 않는다.
	WaitForThreadpoolWaitCallbacks(ptpWait, TRUE);
	CloseThreadpoolWait(ptpWait);
	/*
	 새로운 스레드 풀 역시 대기 스레드가 존재하고, 이 스레드에서 동기화 객체가 시그널 상태가 되면
	 큐에 콜백 항목을 추가하는 것은 마찬가지다. 하지만 콜백 항목을 스레드 풀의 큐에 추가하고 난 후
	 대기 항목에서 이 이벤트 핸들을 제거하기 때문에, 두 번째 시그널링에 의해 비록 이벤트가 또다시 시그널 상태가 되더라도
	 콜백 함수는 호출되지 않는다. 따라서 앞의 코드에서 두 번째 SetEvent 호출 후 다시 SetThreadpoolWait를 호출하면
	 그제서야 콜백 함수가 호출될 것이다.
	*/
#elif (SAMPLE_NO == 3) 
	PTP_WAIT ptpWait = CreateThreadpoolWait(ThreadPoolWaitProc, (PVOID)3, NULL);
	HANDLE hTimer = CreateWaitableTimer(NULL, FALSE, NULL);

	SetThreadpoolWait(ptpWait, hTimer, NULL);

	getchar();
	LARGE_INTEGER ll; ll.QuadPart = -30000000LL;
	SetWaitableTimer(hTimer, &ll, 2000, NULL, NULL, FALSE);

	WaitForThreadpoolWaitCallbacks(ptpWait, FALSE);
	CloseThreadpoolWait(ptpWait);
#endif

	printf("\n===== MainThread(ID : %d) terminated...\n", GetCurrentThreadId());
}
