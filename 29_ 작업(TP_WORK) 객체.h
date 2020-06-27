/*
이 카테고리의 스레드 풀 기능은 윈도우 2000에서의 QueueUserWorkItem 함수를 대체한 것이다. 
어떤 특정 함수를 비동기적으로 호출하고자 할 경우에 사용하면 편리하다.

생성과 삭제는 앞서 설명한 형태 그대로 다음의 함수를 통해 수행된다.

TP_WORK 생성
PTP_WORK WINAPI CreateThreadpoolWork
(
  _In_          PTP_WORK_CALLBACK    pfnwk,
  _Inout_opt_   PVOID        pv,
  _In_opt_      PTP_CALLBACK_ENVIRON   pcbe
);

TP_WORK 해제
VOID WINAPI CloseThreadpoolWork(_Inout_ PTP_WORK pwk);

콜백 함수 역시 앞서 설명한 대로 아래의 형식으로 정의하면 된다.
VOID CALLBACK WorkCallback
(
  _Inout_      PTP_CALLBACK_INSTANCE   Instance,
  _Inout_opt_  PVOID         Context,
  _Inout_      PTP_WORK       Work
);

*/