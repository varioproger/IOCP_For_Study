/*
2000 스레드 풀 API 중 RegisterWaitForSingleObject를 대체한 것이다
임의의 커널 객체가 시그널 상태가 되었을 때, 원하는 작업 수행을 담은 콜백 함수를 호출해준다. 
역시 시그널 상태뿐만 아니라 타임아웃이 발생했을 경우에도 콜백 함수는 실행된다.

1) TP _WAIT 객체의 생성과 삭제
TP _WAIT 객체의 생성과 삭제를 담당하는 함수는 다음의 CreateThreadpoolWait 함수와 CloseThreadpoolWait 함수다.
TP_WAIT 생성
PTP_WAIT WINAPI CreateThreadpoolWait
(
  _In_          PTP_WAIT_CALLBACK    pfnwa,
  _Inout_opt_   PVOID         pv,
  _In_opt_      PTP_CALLBACK_ENVIRON   pcbe
);

TP_WAIT 해제
VOID WINAPI CloseThreadpoolWait(_Inout_ PTP_WAIT pwa);

기존 XxxCallback 함수에 WaitResult 매개변수가 추가되었다

VOID CALLBACK WaitCallback
(
  _Inout_      PTP_CALLBACK_INSTANCE Instance,
  _Inout_opt_  PVOID         Context,
  _Inout_       PTP_WAIT        Wait,
  _In_          TP_WAIT_RESULT      WaitResult
);

  TP_WAIT _RESULT WaitResult
   대기 상태에서 풀렸을 경우의 결과를 의미하며, WaitForXXX 함수 리턴값인 WAIT_OBJECT_0과 WAIT_TIMEOUT 중 하나를 넘겨준다. 
    RegisterWaitForSingleObject 함수와 마찬가지로 동기화 커널 객체를 기다리는 동안 타임아웃을 지정할 수 있으며, 
    WaitResult 값이 WAIT_TIMEOUT이면 타임아웃에 의해 콜백 함수가 호출되었고, 
    WAIT_OBJECT_0이면 지정된 동기화 객체가 시그널 상태가 되어 호출되었다는 것을 구분하는 역할을 한다.

2) 동기화 객체의 설정
CreateThreadpoolWait를 이용해서 대기 작업 항목을 생성했다면 이제 구체적으로 특정 대기 커널 객체와 작업 항목을 연결시켜줘야
해당 커널 객체가 시그널 상태가 되었을 때 콜백 함수가 호출될 것이다.

VOID WINAPI SetThreadpoolWait
(
  _Inout_    PTP_WAIT    pwa,
  _In_opt_   HANDLE    h,
  _In_opt_   PFILETIME   pftTimeout
);
PTP_WAIT pwa
CreateThreadpoolWait 함수의 리턴값인 대기 작업 항목 구조체의 포인터다.

HANDLE h
시그널 상태가 되었을 때 콜백 함수를 실행할 수 있도록 하는 동기화 특성을 지닌 커널 객체의 핸들값을 지정한다.
h를 NULL로 넘겨주면 더 이상 새로운 콜백 함수의 추가를 막겠다는 의미가된다.

이 매개변수로 뮤텍스의 핸들을 넘겨주지 말아야 한다. 
뮤텍스의 핸들을 매개변수로 SetThreadpoolWait 함수를 호출하면 STATUS_THREADPOO_HANDLE_EXCEPTION(0xC000070A) 예외가 발생한다.

WaitCallback 함수의 WaitResult 매개변수의 값으로 WAIT_ABANDONED_0이 전달될 일은 없다고 봐도 무관하다.

PFILETIME pftTimeout
타임아웃 값을 지정한다.기존의 WaitForXXX 군의 함수나 RegisterWaitForSingleObject 함수와는 다르게 DWORD가 아니라 
FILETIME의 포인터를 넘겨준다.SetWaitableTimer의 LARGE_INTEGER 타입의 pDueTime 매개변수와 비슷하게 FILTETIME 구조체를 이용해
상대 시간 및 절대 시간을 설정할 수 있다.

이 매개변수를 NULL이 아닌 값으로 넘기면 타임아웃이 발생했을 때, 콜백 함수의 WaitResult 매개변수의 값은 WAIT_TIMEOUT으로 전달된다.

●   NULL
WaitForXXX 함수 호출 시 타임아웃 값을 INFINITE로 지정한 것과 동일하며, 무한대로 대기하겠다는 의미다.
SetThreadpoolWait(ptpWait, hEvent, NULL);

●   FILETIME ← 0
WaitForXXX 함수 호출 시 타임아웃 값을 0으로 지정한 것과 동일하며, 호출 즉시 콜백 함수가 실행된다.

FILETIME ft;
ft.dwHighDateTime = ft.dwLowDateTime = 0;
SetThreadpoolWait(ptpWait, hEvent, &ft);


●   FILETIME ← 음수
SetWaitableTimer 함수 호출 시 pDueTime 값을 음수로 설정한 것과 동일하며, 
호출 후 지정된 시간이 경과하면 타임아웃이 발생하도록 하는 상대적 시간을 지정한다.

ULARGE_INTEGER ll;
ll.QuadPart = -30000000LL;
FILETIME ft;
ft.dwHighDateTime = ll.HighPart;
ft.dwLowDateTime = ll.LowPart;
SetThreadpoolWait(ptpWait, hEvent, &ft);

●   FILETIME ← 양수
SetWaitableTimer 호출 시 pDueTime 값을 양수로 설정한 것과 동일하며, 
지정된 구체적인 날짜와 시간이 지나면 타임아웃이 발생하도록 하는 절대적 시간을 지정한다.

   SYSTEMTIME st;
   st.wYear =  년, st.wMonth = 월, st.wDay =  일;
   st.wHour =  시, st.wMinute = 분, st.wSecont =  초;
   st.wMilliseconds =  0;
   FILETIME ftLocal, ftUTC;
   SystemTimeToFileTime(&st, &ftLocal);
   LocalFileTimeToFileTime(&ftLocal, &ftUTC);
   SetThreadpoolWait(ptpWait, hEvent, &ft);

SetThreadpoolWait 함수의 역할은 스레드 풀의 구성요소인 대기 스레드로 하여금 지정된 동기화 객체에 대해 대기하도록 설정하는 것이다.

이 핸들이 시그널 상태가 되거나 타임아웃이 발생했을 때, 대기 스레드가 깨어나 TP_WAIT 객체에 설정된 콜백 항목을 
스레드 풀의 작업 큐에 엔큐하여 작업자 스레드로 하여금 콜백 함수를 실행하도록 한다.

주의할 것은 RegisterWaitForSingleObject 함수와는 다르게 시그널 상태가 될 때마다 계속 콜백 함수가 실행되도록 하려면, 
콜백 함수가 호출된 후 다시 SetThreadpoolWait 함수를 호출해서 해당 대기 핸들을 설정해주어야 한다는 점이다.

TPoolWaitVista 소스 참조
*/
