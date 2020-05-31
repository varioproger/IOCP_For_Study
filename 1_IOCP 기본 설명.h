/*
HANDLE WINAPI CreateIoCompletionPort
(
    _In_       HANDLE    FileHandle,
    _In_opt_   HANDLE    ExistingCompletionPort,
    _In_       ULONG_PTR   CompletionKey,
    _In_       DWORD    NumberOfConcurrentThreads
);

HANDLE FileHandle
생성된 IOCP 커널 객체에 연결시키고자 하는, FILE_FLAG_OVERLAPPED 플래그와 함께
CreateFile로 생성되거나 열린 파일 또는 장치의 핸들이며, 새로운 IOCP 커널 객체만을 생성하
고자 한다면 이 값은 INVALID_HANDLE_VALUE가 되어야 한다.

HANDLE ExistingCompletionPort
이미 생성된 IOCP의 핸들을 의미하며, 장치 핸들을 이 매개변수에 지정된 IOCP 커널 객체에
연결시키고자 할 때 사용한다. IOCP 생성 시 CreateIoCompletionPort에 의해 반환된 핸들
을 넘겨준다. 새로운 IOCP 커널 객체를 생성한다면 이 값은 NULL이어야 한다.

ULONG_PTR CompletionKey
IOCP와 연결하고자 하는 장치를 식별할 수 있는 값을 지정한다. 여러 개의 장치가 하나의 IOCP
커널 객체에 연결되었을 때 각각의 장치를 이 매개변수가 지정한 값으로 식별할 수 있다. 새로운
IOCP 커널 객체를 생성하는 경우라면 이 값은 의미가 없으므로, 단순히 0을 넘겨주면 된다.

DWORD NumberOfConcurrentThreads
동시 실행 가능한 스레드의 수를 의미한다. 우선 이 매개변수는 여러 장치로부터 입출력이 완료
되었을 때, 완료된 입출력에 대한 처리를 동시에 수행할 수 있는 스레드의 최대 개수라고 알아두
자. 이 값을 0으로 넘기면 윈도우는 해당 시스템의 CPU 개수만큼의 값을 지정한다. 또한 이 매
개변수는 앞서 세 개의 매개변수와는 다르게 새로운 IOCP 커널 객체 생성 시에만 의미가 있고,
기존에 존재하는 IOCP에 특정 장치를 연결시킬 경우에는 의미가 없다.


[반환값] HANDLE
새로운 IOCP 커널 객체의 생성인 경우, 이 값은 생성된 IOCP 커널 객체의 핸들값이다. 기존의
IOCP 커널 객체에 장치 핸들을 연결시킬 경우, 이 값은 CreateIoCompletionPort 호출 시 넘
겨준 두 번째 매개변수인 ExistingCompletionPort 값과 동일하다. 만약 호출에 실패하면 이
값은 NULL이 되고 GetLastError를 통하여 그 원인을 알 수 있다.


새로운 IOCP 커널 객체의 생성

HANDLE hIocp =  CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);

사실 최대의 성능을 내려면 문맥 전환이 없도록 하나의 CPU가 스레드 하나씩을 담당하는 경우가 이상적일 것이다.


열린 장치와 IOCP의 연결

#define  IOCP_READ_KEY  100

HANDLE hDevice =  CreateFile(..., FILE_FLAG_OVERLAPPED, ...);
        ⋮
CreateIoCompletionPort(hDevice, hIocp, IOCP_READ_KEY, 0);

IOCP를 사용할 수 있는 장치들은 모두 OVERLAPPED 구조체와 관련이 있는 장치들이다

세 번째 매개변수는 장치를 식별하는 여러분이 정의한 고유의 값이다.
이 값은 일반 정수가 될 수도 있고 아니면 여러분이 정의한 고유의 데이터 구조체의포인터일 수도 있다.
입출력 완료에 대한 처리를 수행하는 코드를 작성할 때 식별 가능한 어떤 데이터 타입이면 된다.
IOCP에 대해 대기 중인 스레드가 깨어났을 때, 여러분의 목적에 따라 다양하게 생성키를 변환하여 사용할 수 있도록 하면 된다.

한줄로 묶을경우,
HANDLE hIocp =  CreateIoCompletionPort(hDevice, NULL, IOCP_READ_KEY, 2);

IOCP 종료
IOCP 역시 커널 객체고 핸들값으로 프로세스 내에서 식별되기 때문에, 이 IOCP 커널 객체를 사용
한 후 그 핸들을 CloseHandle의 매개변수로 넘겨 닫아주면 된다. IOCP의 핸들을 닫게 되면 해당
IOCP에 대해 대기 중이던 스레드는 ERROR_ABANDONED_WAIT_0(735) 에러와 함께 대기 상태로부터 탈출한다. 
따라서 CloseHandle을 호출해 IOCP를 닫음으로써 이 IOCP에 대기하는 스레드를 종료시킬 수 있는 수단이 되기도 한다.

IOCP를 위한 대기 함수
IOCP에대한 대기는 스레드 엔트리 함수 작성 시 GetQueuedCompletionStatus 함수를 통해 이뤄진다
BOOL WINAPI GetQueuedCompletionStatus
(
  _In_    HANDLE      CompletionPort,
  _Out_   LPDWORD     lpNumberOfBytes,
  _Out_   PULONG_PTR    lpCompletionKey,
  _Out_   LPOVERLAPPED*   lpOverlapped,
  _In_    DWORD      dwMilliseconds
);

HANDLE CompletionPort
CreateIoCompletionPort로 생성된 IOCP 커널 객체의 핸들이다.

LPDWORD  lpNumberOfBytes
장치로부터 전송되거나 장치로 전송한 데이터의 바이트 수를 받아올 DWORD형의 포인터다.
이 값은 OVERLAPPED 구조체의 InternalHigh 필드와 동일한 값을 가진다.

PULONG_PTR  lpCompletionKey
CreateIoCompletionPort를 통해서 해당 장치 핸들과 IOCP를 연결시킬 때, 장치 식별을 위
해 넘겨준 생성키를 받아오기 위한 포인터 변수로, 이 값을 통해 해당 장치를 식별할 수 있다.

LPOVERLAPPED* lpOverlapped
OVERLAPPED 구조체에 대한 더블 포인터 형의 매개변수로, ReadFile/WriteFile 호출 시에
여러분이 넘겨주는 OVERLAPPED 구조체의 포인터값을 돌려준다.

DWORD dwMilliseconds
대기 시간을 밀리초 단위로 지정할 수 있다. 지정된 시간이 경과되면 타임아웃이 발생되어
GetQueuedCompletionStatus로부터 리턴되고 스레드는 깨어난다. 이때 리턴값은 FALSE가
되고 GetLastError 값은 WAIT_TIMEOUT(258)이 된다. 물론 이 값을 INFINITE으로 설정
하게 되면 스레드는 해당 IOCP가 시그널 상태가 될 때까지 무한히 대기하게 된다.

[반환값] BOOL
성공했을 경우 이 값은 TRUE며, 여기서 성공했다는 의미는 GetQueuedCompletionStatus를 호출한 스레드가 
스레드 풀로부터 빠져나와 활성화된 경우를 말한다. 이것은 곧 입출력 완료 후의 처리를 수행할 수 있음을 의미한다. 
실패한 경우 이 값은 FALSE가 되고 그 원인은GetLastError를 통해서 알 수 있다. 
대표적으로 GetQueuedCompletionStatus를 호출 중인상태에서 CloseHandle을 통해서 IOCP를 닫을 경우 
FALSE가 리턴되고 GetLastError 값은ERROR_ABANDONED_WAIT_0(735)가 된다.*

IOCP는 특정 상황이 되면 시그널 상태가 되어 GetQueuedCompletionStatus를 리턴시켜 
스레드가 입출력 완료에 대한 사후 처리를 할 수 있도록 해준다.
*/