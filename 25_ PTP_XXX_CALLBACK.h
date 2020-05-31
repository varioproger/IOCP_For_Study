/*
XxxCallback 콜백 함수는 4개의 카테고리 모두 다음의 공통적인 매개변수를 가져야한다. 
입출력 작업과 대기 작업의 경우 추가적인 매개변수가 존재한다.

VOID CALLBACK XxxCallback
(
  _Inout_        PTP_CALLBACK_INSTANCE   Instance,
  _Inout_opt_   PVOID         Context,
  _Inout_        PTP_XXX        ptObj
);

PTP_CALLBACK_INSTANCE  Instance
Instance 매개변수는 콜백 인스턴스 TP_CALLBACK_INSTANCE 구조체의 포인터가 전달된다.
”콜백 인스턴스” 매개변수를 사용하는 예는 6.3.6절에서 설명할 것이다.우선은 무시하기바란다.

PVOID Context
Context 매개변수는 CreateThreadpoolXxx 호출 시 pv 매개변수로 전달된 값이 넘어온다.

PTP_XXX ptObj
ptOjb는 CreateThreadpoolXxx 함수를 통해 생성된 작업 항목의 객체 TP_XXX의 포인터값이다.
먼저 CreateThreadpoolXxx 함수 호출을 통하여 원하는 카테고리별 작업 항목의 객체 TP_XXX를 생성한 후, 
이 객체의 포인터값을 스레드 풀에 전달하여 작업 처리를 스레드 풀에 맡길 수 있게된다.
*/
