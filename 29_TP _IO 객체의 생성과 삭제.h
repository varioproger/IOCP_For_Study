/*
다음의 함수를 통해 TP_IO 객체를 생성한다.

PTP_IO WINAPI CreateThreadpoolIo
(
  _In_           HANDLE        fl,
  _In_          PTP_WIN32_IO_CALLBACK  pfnio,
  _Inout_opt_   PVOID        pv,
  _In_opt_      PTP_CALLBACK_ENVIRON   pcbe
);
입출력 작업 항목 객체를 생성하며, 첫 번째 매개변수 fl로 FILE_FLAG_OVERLAPPED 플래그
와 함께 열린 파일 또는 장치 핸들을 넘겨주는 것을 제외하고는 다른 카테고리와 동일하다. 
두 번째 매개변수인 PTP_WIN32_IO_CALLBACK은 다음과 같은 형태로 정의하면 된다.

VOID CALLBACK IoCompletionCallback
(
  _Inout_        PTP_CALLBACK_INSTANCE  Instance,
  _Inout_opt_   PVOID         Context,
  _Inout_opt_   PVOID         Overlapped,
  _In_           ULONG         IoResult,
  _In_           ULONG_PTR       NumberOfBytesTransferred,
  _Inout_        PTP_IO        Io
);
XxxCallback  함수와 다른 부분이 있다면 OVERLAPPED 구조체의 포인터 Overlapped, 
입출력 중의 에러 코드를 전달하는  IoResult, 
그리고 전송 바이트 수를 알려주는 NumberOfBytesTransferred 매개변수가 있다는 점이다.

IoCompletionCallback의 경우 전달되는 IoResult 매개변수의 값은 Internal 필드에 저장된 상태 코드가 아니라 대응되는 적절한 값으로 
변환된 윈도우 시스템 에러 코드라는 점에 주의하기 바란다.

CreateThreadpoolIo 함수를 통해 생성된 TP_IO 객체를 생성할 때에는 CloseThreadpoolIo 함수를 사용해 해제한다.

VOID WINAPI CloseThreadpoolIo(_Inout_ PTP_TIMER pti);
*/