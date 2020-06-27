/*
윈도우 2000의 QueueUserWorkItem의 경우는 TP _WORK라는 객체의 명시적인 지정 없이 바로 유저가 정의한 콜백을 수행하는 구조였다. 
그런 형식의 수행을 원한다면 CreateThreadpoolWork 호출 없이, 
즉 명시적인 작업 항목의 생성 없이 TrySubmitThread-poolCallback 함수를 호출하면 된다.

BOOL WINAPI TrySubmitThreadpoolCallback
(
  _In_          PTP_SIMPLE_CALLBACK    pfns,
  _Inout_opt_   PVOID         pv,
  _In_opt_      PTP_CALLBACK_ENVIRON   pcbe
);

PTP _CALLBACK_ENVIRON 타입의 pcbe 매개변수는 콜백 환경에 대한 포인터며, 우선 NULL로 설정하여 넘기도록 하자. 
PTP_SIMPLE_CALLBACK 타입의 pfns 매개변수는 다음 형태의 콜백 함수를 정의해 그 포인터를 전달해야 하며, 
pv 매개변수는 이 콜백 함수의 Context 매개변수로 전달될 유저 정의 참조 데이터를 지정한다.

VOID CALLBACK SimpleCallback
(
  _Inout_      PTP_CALLBACK_INSTANCE Instance,
  _Inout_opt_  PVOID         Context
);

EchoSvrTPWorkSimple 소스 참조
*/