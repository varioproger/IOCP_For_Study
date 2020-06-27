/*
새로운 스레드 풀 인스턴스를 작업 항목과 바인딩하기 위한 필드

VOID SetThreadpoolCallbackPool
(
  _Inout_  PTP_CALLBACK_ENVIRON  pcbe,
  _In_     PTP_POOL       ptpp
);

인라인 함수며,  InitializeThreadpoolEnvironment 함수처럼 내부적으로 TpSetCallbackThreadpool 인라인 함수를 호출한다. 
즉 위의 함수는 단순히 TP_CALLBACK_ENVIRON 구조체의 Pool 필드를 설정하는 역할밖에 없다. 
하지만 이 함수를 통하지 않으면 새롭게 생성된 스레드 풀을 작업 항목과 바인드할 수 없다. 
이렇게 콜백 환경의 Pool 필드에 새로운 스레드 풀의 포인터가 설정되면, 이제 CreateThreadpoolXxx 호출 시 마지막 매개변수를 
이 콜백 환경 인스턴스의 포인터를 넘겨주어 작업 항목을 새로운 스레드 풀에서 수행되도록 할 수 있다.

TP_CALLBACK_ENVIRON ce;
  InitializeThreadpoolEnvironment(&ce);
① 콜백 환경을 선언하고 초기화한다.

  PTP_POOL pPool = CreateThreadpool(NULL);
  SetThreadpoolThreadMaximum(pPool, 1);
  SetThreadpoolThreadMinimum(pPool, 10);
② 새로운 스레드 풀을 생성하고, 필요하다면 작업 스레드 수의 최대/최소치를 설정한다.
 
 SetThreadpoolCallbackPool(&ce, pPool);
③ 콜백 환경을 새롭게 생성한 스레드 풀과 연결한다.
 
 PTP_WORK ptpWork =  CreateThreadpoolWork(MyWorkCallback, NULL, &ce);
④ 작업 항목을 새로운 스레드 풀에 등록한다. 마지막 매개변수로 ce의 포인터를 지정한다.
		⋮
  CloseThreadpoolWork(ptpWork);
  CloseThreadpool(pPool);
⑤ 새로운 스레드 풀에 대한 작업이 모두 끝났으면 이 스레드 풀을 제거한다.
  
  DestroyThreadpoolEnvironment(&ce);
⑥ 콜백 환경을 해제한다.












*/
