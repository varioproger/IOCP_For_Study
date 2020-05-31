/*
BOOL WINAPI GetQueuedCompletionStatusEx
(
    _In_   HANDLE        CompletionPort,
    _Out_   LPOVERLAPPED_ENTRY   lpCompletionPortEntries,
    _In_   ULONG        ulCount,
    _Out_   PULONG        ulNumEntriesRemoved,
    _In_   DWORD        dwMilliseconds,
    _In_   BOOL         fAlertable
);

  ●   입출력 완료 큐의 입출력 완료 패킷을 한 번에 여러 개 처리할 수 있는 기능을 제공한다.
  ●   APC 콜백 함수 실행 완료를 위한 경보가능 대기 기능을 제공한다.

HANDLE CompletionPort
IOCP 커널 객체에 대한 핸들값을 지정한다.

LPOVERLAPPED_ENTRY  lpCompletionPortEntries
ULONG ulCount
PULONG ulNumEntriesRemoved
여러 장치에서 동시에 입출력이 완료되었을 경우, 입출력 완료 큐에 여러 개의 입출력 완료 패킷이 추가될 것이다.
위 세 개의 매개변수는 이런 여러 패킷들을 모두 한 번에 처리할 수 있도록 하기 위한 매개변수다.
GetQueuedCompletionStatus의 생성키, 전송 바이트 수, OVERLAPPED 구조체 포인터를 받아오는 매개변수를 
다음과 같은 OVERLAPPED_ENTRY구조체로 정의해 이 구조체의 배열을 통해 입출력 완료 큐의 여러 패킷들을 한 번에 데큐하여 
받아오도록 lpCompletionPortEntries 매개변수가 제공된다

OVERLAPPED_ENTRY 구조체 구성 :
typedef struct _OVERLAPPED_ENTRY
{
    ULONG_PTR   lpCompletionKey;
    LPOVERLAPPED  lpOverlapped;
    ULONG_PTR   Internal;
    DWORD     dwNumberOfBytesTransferred;
} OVERLAPPED_ENTRY, * LPOVERLAPPED_ENTRY;

OVERLAPPED_ENTRY 구조체는 GetQueuedCompletionStatus의 매개변수를 통해 받아올 수 있는 내용들을 그 멤버로 갖고 있다.
다만 Internal 멤버는 예약되어 있으므로 사용하지 말아야 한다.
lpCompletionPortEntries 매개변수는 이 구조체의 배열에 대한 포인터를 지정해주고 
ulCount매개변수는 이 배열의 크기를 지정해준다.
GetQueuedCompletionStatusEx로부터 리턴되었을 때, 실제로 lpCompletionPortEntries 배열에 담긴 엔트리의 수, 
즉 입출력 완료 큐로부터 제거된 패킷의 수를 ulNumEntriesRemoved 매개변수에 담아서 돌려준다

DWORD dwMilliseconds
타임아웃 값을 지정할 수 있다.
타임아웃이 발생할 경우 리턴값은 FALSE가 되고 GetLastError 값은 WAIT_TIMEOUT(258)이 된다.

BOOL  fAlertable
WaitForXXXEx 경보가능 대기 함수와 마찬가지로 경보가능 대기를 수행할 것인지를 지정한다.

[반환값] BOOL
APC 콜백 함수의 수행이 완료된 경우에는 FALSE가 리턴되고, GetLastError를 호출하면 
그 결과는 WAIT_IO_COMPLETION(192)이 된다
ulNumEntriesRemoved의 값은 1로 설정된다.
따라서 APC에 의해 리턴된 경우에는 절대 lpCompletionPortEntries 배열의 내용을 참조해서는 안 된다.

-주의 사항- 
GetQueuedCompletionStatusEx로부터의 리턴은 APC 콜백 함수의 수행이 완료되었을 경우보다 입출력 완료 큐에 패킷이 존재하는 경우를 더 우선한다. 
따라서 입출력 완료 큐에 입출력 완료 패킷이 빈번하게 쌓이는 경우 APC의 완료 처리는 뒤로 밀려 늦게 수행될 수 있음에 주의해야 한다.


GetQueuedCompletionStatusEx 소스 코드 참조


*/
