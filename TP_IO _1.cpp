#include<tchar.h>
#include "windows.h"
#include "Ntsecapi.h"
#include "iostream"
using namespace std;


#define BUFF_SIZE	65536

struct COPY_CHUNCK : OVERLAPPED
{
	HANDLE	_hfSrc, _hfDst;
	PTP_IO	_ioSrc, _ioDst;// 소스와 타깃 핸들 각각에 대한 PTP_IO 필드
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
	LONG	_nCpCnt;
	HANDLE	_hevEnd;
};
typedef COPY_ENV* PCOPY_ENV;

VOID CALLBACK ReadCompleted(PTP_CALLBACK_INSTANCE pInst,
	PVOID pCtx, PVOID pOL, ULONG dwErrCode, ULONG_PTR dwTrBytes, PTP_IO pio)
{
	PCOPY_ENV	 pce = (PCOPY_ENV)pCtx;
	PCOPY_CHUNCK pcc = (PCOPY_CHUNCK)pOL;
	DWORD	     dwThrId = GetCurrentThreadId();
	BOOL bIsOK = false;
	if (dwErrCode != 0)
		goto $LABEL_CLOSE;

	/*
	읽기 완료 처리 후에는 비동기 쓰기 WriteFile을 호출해야 하며, 호출하기 전에 StartThreadpoolIo 함수를 호출해야 한다. 
	주의할 것은 WriteFile이 타깃 파일을 대상으로 하기 때문에 StartThreadpoolIo 역시 
	타깃 파일과 연계된 _ioDst TP_IO 객체여야 한다는 점 이다.
	*/
	printf(" => Thr %d Read bytes : %d\n", dwThrId, pcc->Offset);
	StartThreadpoolIo(pcc->_ioDst);// write과 관련된 변수 사용
	bIsOK = WriteFile(pcc->_hfDst, pcc->_arBuff, dwTrBytes, NULL, pcc);
	if (!bIsOK)
	{
		dwErrCode = GetLastError();
		if (dwErrCode != ERROR_IO_PENDING)
		{
			CancelThreadpoolIo(pcc->_ioDst);
			goto $LABEL_CLOSE;
		}
	}
	return;

$LABEL_CLOSE:
	if (dwErrCode == ERROR_HANDLE_EOF)
		printf(" ****** Thr %d copy successfully completed...\n", dwThrId);
	else
		printf(" ###### Thr %d copy failed, code : %d\n", dwThrId, dwErrCode);
	CloseHandle(pcc->_hfSrc);
	CloseHandle(pcc->_hfDst);
	if (InterlockedDecrement(&pce->_nCpCnt) == 0)
		SetEvent(pce->_hevEnd);
}

VOID CALLBACK WriteCompleted(PTP_CALLBACK_INSTANCE pInst,
	PVOID pCtx, PVOID pOL, ULONG dwErrCode, ULONG_PTR dwTrBytes, PTP_IO pio)
{
	PCOPY_ENV	 pce = (PCOPY_ENV)pCtx;
	PCOPY_CHUNCK pcc = (PCOPY_CHUNCK)pOL;
	DWORD	     dwThrId = GetCurrentThreadId();
	BOOL bIsOK = false;
	if (dwErrCode != 0)
		goto $LABEL_CLOSE;

	pcc->Offset += dwTrBytes;
	printf(" <= Thr %d Wrote bytes : %d\n", dwThrId, pcc->Offset);

	StartThreadpoolIo(pcc->_ioSrc);//// read과 관련된 변수 사용
	bIsOK = ReadFile(pcc->_hfSrc, pcc->_arBuff, BUFF_SIZE, NULL, pcc);
	if (!bIsOK)
	{
		dwErrCode = GetLastError();
		if (dwErrCode != ERROR_IO_PENDING)
		{
			CancelThreadpoolIo(pcc->_ioSrc);
			goto $LABEL_CLOSE;
		}
	}
	return;

$LABEL_CLOSE:
	if (dwErrCode == ERROR_HANDLE_EOF)
		printf(" ****** Thr %d copy successfully completed...\n", dwThrId);
	else
		printf(" ###### Thr %d copy failed, code : %d\n", dwThrId, dwErrCode);
	CloseHandle(pcc->_hfSrc);
	CloseHandle(pcc->_hfDst);
	if (InterlockedDecrement(&pce->_nCpCnt) == 0)//INTERLOCK.cpp 참조
		SetEvent(pce->_hevEnd);
}


#define MAX_COPY_CNT  10
void _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
	{
		cout << "Uasge : MultiCopyBICC SourceFile1 SourceFile2 SourceFile3 ..." << endl;
		return;
	}
	if (argc > MAX_COPY_CNT + 1)
		argc = MAX_COPY_CNT + 1;

	PCOPY_CHUNCK arChunk[MAX_COPY_CNT];
	memset(arChunk, 0, sizeof(PCOPY_CHUNCK) * MAX_COPY_CNT);

	COPY_ENV env;
	env._nCpCnt = 0;
	env._hevEnd = CreateEvent(NULL, TRUE, FALSE, NULL);

	for (int i = 1; i < argc; i++)
	{
		TCHAR* pszSrcFile = argv[i];
		HANDLE hSrcFile = CreateFile
		(
			pszSrcFile, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL
		);
		if (hSrcFile == INVALID_HANDLE_VALUE)
		{
			cout << pszSrcFile << " open failed, code : " << GetLastError() << endl;
			return;
		}

		TCHAR szDstFile[MAX_PATH];
		_tcscpy(szDstFile, pszSrcFile);
		_tcscat(szDstFile, _T(".copied"));
		HANDLE hDstFile = CreateFile
		(
			szDstFile, GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL
		);
		if (hDstFile == INVALID_HANDLE_VALUE)
		{
			cout << szDstFile << " open failed, code : " << GetLastError() << endl;
			return;
		}

		PCOPY_CHUNCK pcc = new COPY_CHUNCK(hSrcFile, hDstFile);
		pcc->_ioSrc = CreateThreadpoolIo(hSrcFile, ReadCompleted, &env, NULL);
		pcc->_ioDst = CreateThreadpoolIo(hDstFile, WriteCompleted, &env, NULL);
		/*
		소스 파일과 타깃 파일에 대해 각각의 TP_IO 객체를 생성한다. 
		각각의 콜백 함수와 함께 COPY_ENV 구조체에 대한 참조 포인터를 별도로 전달한다.
		*/
		arChunk[i - 1] = pcc;
		env._nCpCnt++;
	}

	for (int i = 0; i < env._nCpCnt; i++)
	{
		PCOPY_CHUNCK pcc = arChunk[i];
		StartThreadpoolIo(pcc->_ioSrc);//비동기 읽기 ReadFile 함수를 호출하기 전에 StartThreadpoolIo를 호출해줘야 한다.
		BOOL bIsOK = ReadFile
		(
			pcc->_hfSrc, pcc->_arBuff, BUFF_SIZE, NULL, pcc
		);
		if (!bIsOK)
		{
			DWORD dwErrCode = GetLastError();
			if (dwErrCode != ERROR_IO_PENDING)
			{//에러 코드가 ERROR_IO_PENDING이 아닐 경우에는 CancelThreadpoolIo 함수를 호출해야 한다.
				CancelThreadpoolIo(pcc->_ioSrc);
				break;
			}
		}
	}

	WaitForSingleObject(env._hevEnd, INFINITE);
	for (int i = 0; i < env._nCpCnt; i++)
	{
		PCOPY_CHUNCK pcc = arChunk[i];
		CloseThreadpoolIo(pcc->_ioSrc);
		CloseThreadpoolIo(pcc->_ioDst);
		/*
		소스와 타깃 파일에 대해 생성한 각각의 TP_IO 객체를 해제한다. 
		env._hevEnd 이벤트 객체를 통해 모든 파일의 복사가 완료될 때까지 직접 대기하기 때문에, 
		본 코드에서는 굳이 개별 복사 항목에 대해 WaitForThreadpoolIoCallbacks 함수를 호출할 필요는 없다.
		*/
		delete pcc;
	}
	CloseHandle(env._hevEnd);
}
