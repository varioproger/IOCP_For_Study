#include "Windows.h"
#include "iostream"
#include<tchar.h>
using namespace std;


VOID CALLBACK ThreadPoolIoProc(PTP_CALLBACK_INSTANCE pnst,
	PVOID pctx, PVOID pol, ULONG ior, ULONG_PTR dwTrBytes, PTP_IO pio)
{
	LPOVERLAPPED pov = (LPOVERLAPPED)pol;
	DWORD dwThrId = GetCurrentThreadId();

	if (ior != 0)
	{
		printf(" => Thread %d reads %d error occurred: %d\n", dwThrId, ior);
		return;
	}

	LARGE_INTEGER ll;
	ll.LowPart = pov->Offset, ll.HighPart = pov->OffsetHigh;
	printf(" => Thread %d reads %d bytes, offset=%I64d\n",
		dwThrId, dwTrBytes, ll.QuadPart);
}

void _tmain()
{
	HANDLE hFile = CreateFile(_T("D:\\Game_Server\\IOCP 공부용\\1.txt"),
		GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	LARGE_INTEGER llSize;
	GetFileSizeEx(hFile, &llSize);

	PTP_IO ptpIo = CreateThreadpoolIo(hFile, ThreadPoolIoProc, NULL, NULL);
	//BindIoCompletionCallback 호출 대신 파일 핸들 hFile을 전달하여 TP_IO 객체를 생성한다.
	OVERLAPPED ov;
	ov.Offset = ov.OffsetHigh = 0;
	ov.hEvent = NULL;
	while (llSize.QuadPart > 0)
	{
		BYTE arrBuff[4096];

		StartThreadpoolIo(ptpIo);
		//비동기 입출력 개시 이전에 미리 StartThreadpoolIo를 호출한다.
		if (!ReadFile(hFile, arrBuff, sizeof(arrBuff), NULL, &ov))
		{
			int nErrCode = GetLastError();
			if (nErrCode != ERROR_IO_PENDING)
			{
				cout << "MainThread -> Error occurred : " << nErrCode << endl;
				CancelThreadpoolIo(ptpIo);
				/*
				에러가 발생했을 경우 앞서 StartThreadpoolIo를 호출했으므로,
				그에 대한 취소 처리를 위해 CancelThreadpoolIo 함수를 호출해야 한다.
				*/
				break;
			}
		}
		printf("before\n");
		WaitForThreadpoolIoCallbacks(ptpIo, FALSE);
		/*
		입출력 완료를 기다리기 위해 TP_IO 객체를 사용할 경우에는 WaitForThreadpoolIoCallbacks 함수를 이용하면 된다.
		*/
		printf("After\n");
		llSize.QuadPart -= ov.InternalHigh;

		LARGE_INTEGER ll;
		ll.LowPart = ov.Offset, ll.HighPart = ov.OffsetHigh;
		ll.QuadPart += ov.InternalHigh;
		ov.Offset = ll.LowPart, ov.OffsetHigh = ll.HighPart;
	}
	CloseThreadpoolIo(ptpIo);
	//생성한 TP_IO 객체를 삭제한다.
	CloseHandle(hFile);
}
// WaitForThreadpoolIoCallbacks 함수를 사용해 기존 코드에서 이벤트를 통한 대기 처리를 제거했다