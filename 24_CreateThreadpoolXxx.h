/*
스레드 풀에 등록할 작업 항목 객체의 생성에는 다음의 함수를 이용한다.
PTP_XXX WINAPI CreateThreadpoolXxx
(
  _In_      PTP_XXX_CALLBACK    pfnwa,
  _Inout_opt_   PVOID         pv,
  _In_opt_      PTP_CALLBACK_ENVIRON   pcbe
);

PTP_XXX_CALLBACK pfnwa
PVOID pv
   매개변수 pfnwa는 여러분이 정의한 콜백 함수의 포인터며, 매개변수 pv는 pfnwa 콜백 함수로 넘겨줄 유저 정의 참조 데이터의 값을 지정한다. 
   모든 카테고리에 대해 유저가 정의한 참조 데이터를 넘겨줄 수 있도록 매개변수를 제공하고 있다.

PTP_CALLBACK_ENVIRON pcbe
   콜백 환경 TP_CALLBACK_ENVIRON 구조체 인스턴스의 포인터를 지정한다.
   “콜백 환경”은 6.4.2절에서 자세히 설명할 예정이며, 우선은 NULL로 지정하도록 하자. 
   이 매개변수가 NULL이 되면 디폴트 스레드 풀을 사용하겠다는 것을 의미한다.

[반환값] PTP_XXX
   CreateThreadpoolXxx 함수는 작업 항목에 대한 정보를 담고 있는 TP_XXX의 객체를 생성해서 그 포인터를 리턴해준다. 
   작업 항목을 의미하는 이 객체 TP_XXX의 포인터를 중심으로 스레드 풀 관련된 작업 항목의 처리가 이루어진다.
   NULL이면 에러를 의미하고, GetLastError 함수를 통해서 에러 코드를 획득할 수 있다.
*/
