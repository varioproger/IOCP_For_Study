/*
36번이후로 알아야 할 것들이 더 있지만 일단 생략하고 바로 2로 넘어가기로 했다.

PTP_XXX WINAPI CreateThreadpoolXxx
(
  _In_       PTP_XXX_CALLBACK   pfnwa,
  _Inout_opt_  PVOID        pv,
  _In_opt_       PTP_CALLBACK_ENVIRON  pcbe
);

함수의 마지막 매개변수는 PTP_CALLBACK_ENVIRON 타입을 가진 pcbe이다.

환경 설정을 위해 TP_CALLBACK_ENVIRON 객체가 제공된다.

이 구조체를 통해 콜백 항목이 실행될 별도의 스레드 풀을 제공할 수 있고, 
작업자 스레드의 우선순위나 스택 크기를 설정하거나 스레드 풀에 대한 
일괄 정리 작업을 제공할 수도 있다.

CreateThreadpoolXxx 호출 시에 마지막 매개변수를 NULL로 넘겨주면 이는 
시스템의 디폴트 스레드 풀을 사용하겠다는 의미다.

풀 내의 스레드 수의 최댓값, 최솟값을 필요에 맞게 설정할 수 있다.

PTP_POOL WINAPI CreateThreadpool(_Reserved_ PVOID reserved);
VOID WINAPI CloseThreadpool(_Inout_ PTP_POOL ptpp);

reserved 매개변수를 NULL로 지정하여 CreateThreadpool 함수를 호출하면 리턴값으로 
새롭게 생성된 TP_POOL 객체의 포인터를 돌려준다. 
생성된 별도의 스레드 풀을 제거하고자 할 때에는 CloseThreadpool을 호출한다.

이렇게 CreateThreadpool을 이용해 만들어진 스레드 풀은 기본적으로 최대 500개의 작업 스레드를 가질 수 있다.

이 스레드 풀에서 관리할 최대 및 최소 스레드의 수를 변경하고자 한다면
VOID WINAPI SetThreadpoolThreadMaximum
(
  _Inout_ PTP_POOL   ptpp,
  _In_    DWORD     cthrdMost
);

BOOL WINAPI SetThreadpoolThreadMinimum
(
  _Inout_ PTP_POOL   ptpp,
  _In_    DWORD     cthrdMic
);

  PTP_POOL ptpp
  CreateThreadpool을 통해 생성된 스레드 풀 인스턴스의 포인터다.

  DWORD cthrdMost/cthrdMic
   스레드 풀 내 작업 스레드의 최대-cthrdMost, 최소-cthrdMic 수를 지정한다. 
   현재 스레드 풀의 최대치보다 큰 값을 SetThreadpoolThreadMinimum 함수를 통해 최솟값으로 설정하면 
   최댓값, 최솟값 모두 새롭게 지정된 최솟값으로 설정되며, 반대로 현재 최소치보다 작은 
   최댓값을 SetThreadpoolThreadMaximum 함수를 통해 설정하게 되면 최솟값이 새로운 최댓값으로 설정된다. 
   그리고 매개변수 cthrdMost와 cthrdMic의 타입은 DWORD이지만 내부적으로 LONG 타입으로 취급되며, 
   0보다 크거나 같은지를 체크한다.

   위의 함수에서 주목해야 할 점이 있다. 최솟값 설정 함수(SetThreadpoolThreadMinimum)는리턴값이 BOOL 타입이지만, 
   최댓값을 설정하는 함수(SetThreadpoolThreadMaximum)는 리턴값이 없다는 점이다. 
   왜 이런 차이가 생길까? 최댓값 설정은 작업 스레드의 상한만을 지정할 뿐이라서 내부적으로 상한값을 보관하는 필드만 갱신하면 된다. 
   하지만 최솟값의 경우 새로 지정될 최솟값이 이전 최소치보다 클 경우는 실제로 작업자 스레드 수를 새로운 최소치만큼 늘려야만 한다.
   따라서 추가적인 작업자 스레드를 할당하는 과정에서 오류가 발생될 수 있기 때문에 BOOL 타입의 성공 또는 실패 여부를 반환하게 되는 것이다. 
   작업자 스레드 수는 풀의 실제 작업 부하에 따라서 최대치와 최소치 사이에서 증감을 반복할 것이다.

PTP_POOL pPool =  CreateThreadpool(NULL);

SetThreadpoolThreadMaximum(pPool, 1);
SetThreadpoolThreadMinimum(pPool, 1);

최대, 최소치를 각각 1로 설정하면 해당 스레드 풀의 작업 스레드 수는 언제나 한개가 되며, 
이 스레드는 해당 스레드 풀이 제거되기 전까지 존재하게 되는 영속(Persistent ) 스레드가 된다. 
최대치와 최소치를 동일하게 설정하면 그 수만큼의 영속 스레드가 풀 내에 존재할것이다.

이 영속 스레드의 의미는 CreateThread를 이용해 사용자 정의 스레드를 만드는 것과도 같은 개념이다

지속적인 처리를 요구하는 함수를 콜백 함수 내에서 사용할 때는 CreateThread를 이용해 사용자 정의 스레드를 사용하든지,
아니면 위의 코드처럼 영속 스레드를 지정하고 TrySubmitThreadpoolCallback 함수를 호출해
RegNotifyChangeKeyValue를 사용하는 콜백 함수의 실행을 영속 스레드에 맡기든지 하면 된다.




*/
