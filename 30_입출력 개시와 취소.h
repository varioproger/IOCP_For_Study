/*
CreateThreadpoolIo 함수에 의해 생성된 입출력 객체를 스레드 풀에서 처리할 수 있도록 하려면
StartThreadpoolIo 함수를 호출해야 한다.

VOID WINAPI StartThreadpoolIo (_Inout_ PTP_IO pio);

장치 핸들에 대해 비동기 입출력을 요청하기 전에 반드시 StartThreadpoolIo를 호출해줘야 개시된 비동기 입출력에 대해 완료 통지를 받아서 
스레드 풀이 IoCompletionCallback 콜백 함수를 호출할 수 있다. 이 함수의 호출 없이 비동기 입출력을 개시하면 
스레드 풀이 입출력 완료 시 이를 무시하게 되면서 메모리 손상이 발생되므로 주의해야 한다.

입출력 작업을 개시한 후 콜백 함수가 호출되는 것을 취소하고자 할 경우에는 CancelThreadpoolIo 함수를 호출하면 된다.
VOID WINAPI CancelThreadpoolIo(_Inout_ PTP_IO pio);

CancelThreadpoolIo를 통한 취소는 CancelIoEx 함수 호출을 통해 대기 중인 비동기 입출력 자체의 취소를 의미하는 것은 아니다. 
CancelThreadpoolIo의 호출은 StartThreadpoolIo 호출에 대한 취소를 의미하며, 큐에 등록된 입출력 콜백 항목의 취소를 의미한다. 
따라서 메모리 누수를 막기 위해서는, 다음의 두 경우에 반드시 CancelThreadpoolIo를 호출해 콜백 항목을 취소시켜줘야한다.

①  비동기 입출력 함수 호출 후 FALSE가 리턴되고, 에러 코드가 ERROR_IO_PENDING 이외인 경우
②  4장에서 언급했던 완료 통지 모드 설정 시, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS 플래그와 함께 
   SetFileCompletionNotificationModes 함수를 호출한 상태에서 비동기 입출력 함수의 호출 결과가 TRUE인경우

①의 경우는 비동기 입출력을 요청했으나 다른 이유로 인해 요청 자체가 실패인 경우기 때문에,
StartThreadpoolIo 호출을 통해 등록된 입출력 콜백 항목은 실행되지 않을 것이다. 
이 경우 콜백 항목이 제거되지 않기 때문에 메모리 누수가 발생된다. 
따라서, 다음과 같이 CancelThreadpoolIo 함수를 호출해줘야 한다.

StartThreadpoolIo(ptpIo);
if (!ReadFile(hFile, arrBuff, sizeof(arrBuff), NULL, &ov))
{
  int nErrCode = GetLastError();
  if (nErrCode != ERROR_IO_PENDING)
  {
    CancelThreadpoolIo(ptpIo);
            ⋮
  }
}


②의 경우는 완료 통지 모드를 비동기 입출력을 요청했을 때, 이미 입출력이 완료되어 성공했을 경우에는 
입출력 완료 큐에 IOCP 패킷을 추가하지 말 것을 설정한 상태다. 따라서 ReadFile/WriteFile을 비동기로 호출했을 때 
TRUE가 리턴되면 입출력이 이미 완료된 상태를 의미하므로 IOCP 큐에 완료 항목이 등록되지 않게 되며, 
따라서 StartThreadpoolIo에 의해 등록된 콜백 항목은 실행되지 않는다. 
그러므로 이 역시 콜백 항목이 제거되지 않기 때문에 메모리 누수가 발생하고, 이런 경우의 문제를 막기 위해 
CancelThreadpoolIo 함수를 호출해 취소 통지를 함으로써 콜백 항목을 제거해야 한다.

StartThreadpoolIo(ptpIo);
if (!ReadFile(hFile, arrBuff, sizeof(arrBuff), NULL, &ov))
{
  int nErrCode =  GetLastError();
  if (nErrCode != ERROR_IO_PENDING)
  {
   CancelThreadpoolIo(ptpIo);
            ⋮
  }
}
else
{
  CancelThreadpoolIo(ptpIo);
            ⋮
}

*/