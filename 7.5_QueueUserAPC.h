/*
입출력이란 관점을 떠나서 유저가 정의한 콜백 함수를 특정 스레드가 실행할수 있도록 하는 함수가 바로 QueueUserAPC다. 
QueueUserAPC는 사용자가 직접 정의한 콜백 함수를 특정 스레드의 APC 큐에 추가하는 데 사용된다.

DWORD WINAPI QueueUserAPC
(
  _In_  PAPCFUNC  pfnAPC,
  _In_  HANDLE   hThread,
  _In_  ULONG_PTR  dwData
);

PAPCFUNC pfnAPC
ULONG_PTR dwData
   첫 번째 매개변수 pfnAPC는 여러분이 정의한 APC 콜백 함수며, 
   세 번째 매개변수 dwData는 이 APC 콜백 함수의 매개변수로 넘겨줄 유저 정의 데이터를 지정하며, 
   다음과 같은 타입을 가져야 한다.

   void WINAPI APCCallback(ULONG_PTR dwData)

시스템은 APCProc를 호출할 때 QueueUserAPC의 세 번째 매개변수로 넘어온 dwData 값을APCProc의 dwParam으로 넘겨준다.
ReadFileEx/WriteFileEx의 입출력 완료 처리 콜백 함수와는 다르게 부가적인 정보를 dwParam을 통해 넘겨줄 수 있다.

HANDLE hThread
QueueUserAPC의 두 번째 매개변수 hThread는 pfnAPC를 실행할 스레드의 핸들을 지정해준다. 
QueueUserAPC의 경우 여러분이 직접 콜백 함수를 실행할 스레드를 지정해줄 수 있다. 
즉 이함수를 호출하면 시스템은 APC 엔트리를 이 매개변수로 지정된 스레드의 APC 큐에 추가시킨다.

[반환값] DWORD
QueueUserAPC의 반환값은 DWORD지만 BOOL 타입과 동일하다. 
콜백 함수가 APC 큐에성공적으로 등록되었을 경우에는 0이 아닌 값이 리턴되지만, 실패했을 경우에는 0이 리턴된다.
*/