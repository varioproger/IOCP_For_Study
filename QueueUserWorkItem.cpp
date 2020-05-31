#include "Windows.h"
#include<tchar.h>
#include "iostream"
using namespace std;

DWORD WINAPI ThreadPoolWorkProc(PVOID pParam)
{
	int nEvtIdx = (int)pParam;
	cout << " => Thread " << nEvtIdx << "("
		<< GetCurrentThreadId() << ") Signaled..." << endl;
	Sleep((nEvtIdx + 1) * 1000);
	return 0;
}

void _tmain()
{
	for (int i = 0; i < 10; i++)
	{
		QueueUserWorkItem(ThreadPoolWorkProc, (PVOID)i, WT_EXECUTEDEFAULT);
	}
	getchar();
}
//콜백 함수를 비동기적으로 10회 실행하는 예