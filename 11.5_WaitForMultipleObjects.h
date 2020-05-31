/*
DWORD WINAPI WaitForMultipleObjects
(
  _In_  DWORD     nCount,
  _In_  const HANDLE*  pHandles,
  _In_  BOOL     bWaitAll,
  _In_  DWORD     dwMilliseconds
);

DWORD nCount
const HANDLE* pHandles
pHandles는 동기화 개체들의 핸들을 담고 있는 배열의 시작 포인터고,
nCount는 그 배열의 원소의 개수, 즉 동기화 커널 객체의 핸들 수를 지정한다. 
nCount의 최대 수는 WinNT.h에 정의되어 있는 MAXIMUM_WAIT_OBJECTS 매크로며, 그 값은 64다. 
만약 64개 이상의 동기화 객체에 대해 스레드를 대기 상태로 두려면 해당 객체들의 핸들을 64개 단위의 배열에 따로 담아서 
각 64개 단위의 배열에 대해 스레드를 할당해 WaitForMultipleObjects 함수를 따로 호출하면 된다. 
또한 이 배열에 속할 동기화 객체의 타입이 모두 같을 필요는 없다. 동기화가 가능한 객체면
이벤트든, 스레드든, 뮤텍스든 어떤 것이라도 상관없이 사용 가능하다.

BOOL bWaitAll
pHandles에 포함된 동기화 객체들의 상태가 모두 시그널 상태일 경우에만 이 함수에서 리턴할 것인지, 
아니면 그 객체 중 하나만 시그널 상태로 바뀌더라도 리턴할 것인지를 지시하는 플래그다. 
만약 TRUE면 pHandles에 포함된 모든 객체들이 시그널 상태가 될 때까지 대기하며,
FALSE면 하나라도 시그널 상태가 되면 바로 리턴된다.

DWORD dwMilliseconds
 WaitForSingleObject와 마찬가지로 타임아웃 값을 지정할 수 있다.

[반환값] DWORD
  ● WAIT_FAILED 및 WAIT_TIMEOUT
  WaitForSingleObject에서의 의미와 동일하다.
  ● 성공적인 대기
  - bAllWait이 TRUE : WAIT_OBJECT_0이 되고 모든 객체가 시그널 상태임을 의미한다.
  -   bAllWait이 FALSE  : WAIT_OBJECT_0 ~  (WAIT_OBJECT_0 + nCount–1) 사이의 값을 가진다.  
  (반환값 - WAIT_OBJECT_0)의 값은 pHandles 배열 원소 중에서 시그널 상태가 된 객체를 가리키는 인덱스가 된다. 
  WAIT_OBJECT_0이 0이므로 실제로는 반환값 자체가 인덱스가 된다.

  ● 포기된 뮤텍스
  pHandles 배열에 담긴 객체 중 뮤텍스가 적어도 하나 이상 존재하는 경우에 리턴 가능한 값이다.
  WAIT_ABANDONED_0 ~ (WAIT_ABANDONED_0 + nCount–1) 사이의 값을 가진다.
  (반환값 - WAIT_ABANDONED_0)의 값은 pHandles 배열 원소 중에서 포기된 뮤텍스의 인덱스가 된다.
  - bAllWait이 TRUE  : 모든 객체가 시그널 상태고, 그중 하나는 포기된 뮤텍스다.
  - bAllWait이 FALSE : pHandles 배열 원소 중에서 포기된 뮤텍스를 가리킨다.
*/
