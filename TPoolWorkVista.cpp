#include "Windows.h"
#include<stdio.h>
#define SAMPLE_NO	0
/*
QueueUserWorkItem의 예를 비스타 작업 함수들로 대체한 것이다.

새로운 스레드 풀 메커니즘에서는 작업 항목을 큐에 등록해야만 콜백 함수가 실행된다.
큐에 작업 항목을 등록하기 위해서는 SubmitThreadpoolWork를 호출해줘야 한다.

새로운 스레드 풀에서는 작업 항목 객체와 큐를 분리했기 때문에 동일한 작업 항목에 대해 콜백 함수를 반복해서 실행되도록 만들 수 있으며, 
SubmitThreadpoolWork를 호출할 때마다 콜백 함수가 실행된다.
*/
//콜백 함수 WorkCallback의 정의
VOID CALLBACK ThreadPoolWorkProc(PTP_CALLBACK_INSTANCE pInst, PVOID pCtx, PTP_WORK ptpWork)
{
	/*
	작업에 대한 콜백 함수를 정의한다. 매개변수 pParam을 통해 작업의 ID를 받아서 그 값만큼 초 단위로 대기한 후 종료한다. 
	시작과 종료 시의 시간과 스레드 ID를 출력하도록 했다.
	*/
	int nWorkId = (int)pCtx;
	SYSTEMTIME st;

	GetLocalTime(&st);
	printf("=> WorkItem %d(ID : %d) started at : %02d:%02d:%02d.%03d\n",
		nWorkId, GetCurrentThreadId(),
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	Sleep(nWorkId * 1000);

	GetLocalTime(&st);
	printf("...WorkItem %d(ID : %d) ended at : %02d:%02d:%02d.%03d\n",
		nWorkId, GetCurrentThreadId(),
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

void main()
{
	printf("===== MainThread(ID : %d) started...\n\n", GetCurrentThreadId());

#if (SAMPLE_NO == 0)//일반적인 사용 방법
	PTP_WORK arptpWorks[5];
	for (int i = 0; i < 5; i++)
		arptpWorks[i] = CreateThreadpoolWork(ThreadPoolWorkProc, (PVOID)i, NULL);
	//10개의 TP_WORK 작업 항목을 생성한다.

	for (int i = 0; i < 5; i++)
		SubmitThreadpoolWork(arptpWorks[i]);
	//10개의 작업 항목을 개시하도록 SubmitThreadpoolWork 함수를 호출해 스레드 풀 큐에 항목을 추가한다.

	for (int i = 0; i < 5; i++)
		WaitForThreadpoolWorkCallbacks(arptpWorks[i], FALSE);
	//수행 중인 10개의 작업 항목의 콜백 함수가 종료될 때까지 대기한다.

	for (int i = 0; i < 5; i++)
		CloseThreadpoolWork(arptpWorks[i]);
	//큐에 등록된 작업 항목을 모두 제거한다.

#elif (SAMPLE_NO == 1) // TP_WORK 작업 항목을 하나만 생성하고 SubmitThreadpoolWork를 반복해서 호출한 예다.
	PTP_WORK ptpWork = CreateThreadpoolWork(ThreadPoolWorkProc, (PVOID)3, NULL);
	for (int i = 0; i < 3; i++)
		SubmitThreadpoolWork(ptpWork);
	/*
	앞서 실행된 콜백이 종료되지 않더라도 여러 번(MAXULONG 값의 횟수까지) 작업 객체에 대해 콜백 수행이 가능하다
	이럴 경우 스레드 풀은 효율성의 증대를 위해 해당 스레드를 억제할 수도 있다
	*/
	WaitForThreadpoolWorkCallbacks(ptpWork, FALSE);
	//WaitForThreadpoolWorkCallbacks 함수를 통해서 콜백 수행이 완료될 때까지 대기할 수 있는 수단을 제공한다.

	CloseThreadpoolWork(ptpWork);

#elif (SAMPLE_NO == 2) 
	//코드는 3개의 작업에 대해 순차적으로 큐에 추가한 후 하나의 콜백 수행이 완료되면 다시 다음 작업을 추가하도록 처리하여 동기적으로 작업이 처리되도록 한 예다. 
	PTP_WORK ptpWork = CreateThreadpoolWork(ThreadPoolWorkProc, (PVOID)3, NULL);
	for (int i = 0; i < 3; i++)
	{
		SubmitThreadpoolWork(ptpWork);
		WaitForThreadpoolWorkCallbacks(ptpWork, FALSE);
	}
	CloseThreadpoolWork(ptpWork);
#elif (SAMPLE_NO == 3) 
	PTP_WORK ptpWork = CreateThreadpoolWork(ThreadPoolWorkProc, (PVOID)5, NULL);
	SubmitThreadpoolWork(ptpWork);
	Sleep(4000);
	//WaitForThreadpoolWorkCallbacks(ptpWork, TRUE);
	//CloseThreadpoolWork(ptpWork);
	getchar();
#endif

	printf("\n===== MainThread(ID : %d) terminated...\n", GetCurrentThreadId());
}
