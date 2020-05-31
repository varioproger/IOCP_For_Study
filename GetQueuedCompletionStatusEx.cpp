#pragma warning(disable:4996)
#include<tchar.h>
#include "Windows.h"
#include "list"
#include "iostream"
using namespace std;


#define TM_NONE		  0
#define TM_STR		100
#define TM_POINT	101
#define TM_TIME		102
#define TM_EXIT		200

PCSTR APC_STRS[] =
{
	"INDEX 0 >>> QueueUserAPC 00000\n",
	"INDEX 1 >>> QueueUserAPC 11111\n"
};

void WINAPI APCCallback(ULONG_PTR dwData)
{
	DWORD dwThrId = GetCurrentThreadId();
	printf("  <== APC %d called : %s\n", dwThrId, (PSTR)dwData);
}
DWORD WINAPI IOCPWorkerProc(PVOID pParam)
{
	HANDLE hIocp = (HANDLE)pParam;
	DWORD dwThrId = GetCurrentThreadId();

	while (true)
	{
		OVERLAPPED_ENTRY oves[4];
		memset(oves, 0, sizeof(OVERLAPPED_ENTRY) * 4);
		ULONG uRemCnt = 0;
		BOOL bIsOK = GetQueuedCompletionStatusEx
		(
			hIocp,
			oves, 4, &uRemCnt,
			INFINITE,
			TRUE
		);
		if (!bIsOK)
		{
			DWORD dwErrCode = GetLastError();
			if (dwErrCode == WAIT_IO_COMPLETION)
			{ // WAIT_IO_COMPLETION이면 APC 콜백 함수 호출의 완료를 의미한다. \
			    QueueUserAPC(APCCallback, harWorks[nThrIdx], (ULONG_PTR)APC_STRS[nThrIdx]); 함수 호출로 인해 \
			    스레드가 실행 및 완료 되었을때 확인 할 수있다.
				printf("  ======> %d APC completed by Thread %d\n", uRemCnt, dwThrId);
				continue;
			}
			break;
		}
		printf("uRemCnt = %d\n", uRemCnt);
		//for 문으로 실행할 횟수 \
		즉 입출력 완료 큐의 수를 가져온다. \
		PostQueuedCompletionStatus 소스코드랑 가장 큰 다른건 입출력 완료 큐를 한번에 1개씩 가져오는게 아니라 한번에 여러개를 가져올수 있다.

		bool bIsExit = false;
		for (ULONG i = 0; i < uRemCnt; i++)//그걸 for문으로 처리 하니까 입출력이 완료이 될떄까지 기다리는것보단 빨리 처리 할수있다.
		{
			LPOVERLAPPED_ENTRY poe = &oves[i];
			long  lSize = (long)poe->dwNumberOfBytesTransferred;
			PBYTE pData = (PBYTE)poe->lpOverlapped;
			UINT msg = (UINT)poe->lpCompletionKey;

			if (msg == TM_EXIT)
			{
				bIsExit = true;
				continue;
			}

			switch (msg)
			{
			case TM_STR:
			{
				PSTR pstr = (PSTR)pData;
				printf("  <== R-TH %d read STR : %s\n", dwThrId, pstr);
			}
			break;

			case TM_POINT:
			{
				PPOINT ppt = (PPOINT)pData;
				printf("  <== R-TH %d read POINT : (%d, %d)\n", dwThrId, ppt->x, ppt->y);
			}
			break;

			case TM_TIME:
			{
				PSYSTEMTIME pst = (PSYSTEMTIME)pData;
				printf("  <== R-TH %d read TIME : %04d-%02d-%02d %02d:%02d:%02d+%03d\n",
					dwThrId, pst->wYear, pst->wMonth, pst->wDay, pst->wHour,
					pst->wMinute, pst->wSecond, pst->wMilliseconds);
			}
			break;
			}
			delete[] pData;
		}
		printf("  ======> %d IOCP Packets dequeued by Thread %d\n", uRemCnt, dwThrId);
		if (bIsExit)
			break;
	}
	printf(" *** WorkerProc Thread %d exits\n", dwThrId);

	return 0;
}



void _tmain()
{
	cout << "======= Start MsgNotify Test ========\n" << endl;

	HANDLE hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);

	DWORD  dwThrID;
	HANDLE harWorks[2];
	for (int i = 0; i < 2; i++)
		harWorks[i] = CreateThread(NULL, 0, IOCPWorkerProc, hIocp, 0, &dwThrID);

	char szIn[512];
	while (true)
	{
		cin >> szIn;
		if (_stricmp(szIn, "quit") == 0)
			break;

		if (_strnicmp(szIn, "apc", 3) == 0)
		{
			int len = strlen(szIn);
			int nThrIdx = 0;
			if (len > 3)
			{
				if (szIn[3] == '0')//apc에서 문자가 끝났을 경우
					nThrIdx = 0;
				else //아닌 경우
					nThrIdx = 1;
			}
			printf("nThrIdx = %d\n", nThrIdx);
			QueueUserAPC(APCCallback, harWorks[nThrIdx], (ULONG_PTR)APC_STRS[nThrIdx]);
			continue;//이 부분은 QueueUserAPC를 이용해 \
			         문자열이 apc로 시작하면 QueueUserAPC를 호출해 스레드 풀에 대기 중인 스레드로 하여금 APC 콜백 함수를 수행하도록 한다.
		}

		//이부분은 PostQueuedCompletionStatus 소스코드랑 똑같다.
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
	}
	CloseHandle(hIocp);
	WaitForMultipleObjects(2, harWorks, TRUE, INFINITE);

	cout << "======= End MsgNotify Test ==========" << endl;
}
