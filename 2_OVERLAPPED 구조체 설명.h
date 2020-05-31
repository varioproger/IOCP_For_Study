/*
typedef struct _OVERLAPPED
{
  ULONG_PTR  Internal;
  ULONG_PTR  InternalHigh;
  union
  {
   struct
   {
    DWORD Offset;
    DWORD OffsetHigh;
   };
   PVOID   Pointer;
  };
  HANDLE   hEvent;
} OVERLAPPED, *LPOVERLAPPED;

DWORD Offset, OffsetHigh
파일 제어 시 64비트의 파일 오프셋을 가리킨다. 기존 동기 모델에서 시스템이 자동적으로 관리
해주던 파일 포인터는 비동기 입출력에서는 의미가 없어진다. 따라서 유저가 직접 파일 포인터를
유지해줘야 하며, 이를 위해 Offset, OffsetHigh 두 필드가 사용된다. 유저가 관리해야 하는 필드이므로, 
비동기적으로 파일을 읽고 쓰고자 한다면 이 두 필드는 반드시 초기화되어야 한다.

HANDLE hEvent
입출력 완료 통지를 위한 이벤트다. 만약 이 필드를 NULL로 설정해서 넘기면 CreateFile을 통해서 
획득한 핸들 자체가 입출력 완료 통지를 위한 동기화 객체로 사용되지만, CreateEvent를통해 이 필드를 설정하게 되면 
파일 핸들이 아니라 이 필드에 지정된 이벤트 핸들이 동기화 객체로 사용된다. 
이 필드는 반드시 초기화해야 한다. 비동기 입출력을 사용하면서 유저가 가장 많이
저지르는 실수 중의 하나가 바로 이 필드를 초기화하지 않는 경우다. OVERLAPPED 구조체를
선언한 후 초기화하지 않고 사용하면 hEvent 필드는 쓰레기 값을 갖게 되며, 시스템은 이 쓰레기 값을 
이벤트 커널 객체의 핸들값으로 인식하고 파일 핸들 자체가 아니라 hEvent 필드값에 대해 SetEvent를 호출할 것이다.
따라서 입출력 완료에 대한 통지를 받을 수 없게 된다.

ULONG_PTR  Internal, InternalHigh

  ●   Internal : 입출력 결과에 대한 상태 코드
  ●   InternalHigh : 입출력 시 실제로 읽거나 쓴 바이트 수
  Internal 필드에 설정되는 상태 코드는 GetLastError를 통해서 얻을 수 있는 일반적인 에러 코드가 아니다. 
  커널에서 입출력은 IRP(IO Request Packet )를 통해서 이루어지는데, 
이때 이IRP의 처리 상태를 나타내는 값은 Internal 필드에 담기며 타입은 NTSTATUS로 정의된다. 
이 NTSTATUS 값은 “NTStatus.h”에 정의되어 있으며, 이 값은 유저 영역에 제공되는 입출력 함수내에서 서로 대응되는 에러 코드,
즉 GetLastError를 통해서 획득 가능한 에러 코드로 변환되어사용자에게 제공된다. 
예를 들어 파일을 읽는 도중 파일의 끝에 도달하면 실제 읽기를 담당하는 관련 드라이버는 Internal 필드에는
STATUS_END_OF_FILE(0xC0000011 ) 값을 설정하고,SetLastError를 통해서 사용자에게 돌려주는 에러 코드는 ERROR_HANDLE_EOF(38)로 설정한다. 
이 두 필드는 실제로 커널 내부에서 사용되는 필드라서 유저 영역에서 입출력을 사용하면 이 두 필드는 커널영역의 
장치 드라이버에서 그 정보를 채워주게 된다. 따라서 이 두 필드에 사용자가 직접 값을 설정
하는 일은 가급적이면 피하도록 하고 단지 참조만 하기 바란다.

비동기 입출력을 수행하기 위해서는 OVERLAPPED란 구조체가 필요하다.

현재 비동기입출력이 수행 중인지의 간단한 테스트를 OVERLAPPED 구조체를 통해서 할 수 있는 매크로가 있다.

BOOL HasOverlappedIoCompleted(LPOVERLAPPED lpOverlapped);

이 매크로는 “WinBase.h”에 다음과 같이 정의되어 있다.
#define HasOverlappedIoCompleted(lpOverlapped) \
      (((DWORD)(lpOverlapped)->Internal) != STATUS_PENDING)

따라서 비동기 입출력을 수행 중인 상태에서 GetLastError를 호출하면, 
리턴되는 에러 코드는STATUS_PENDING에 대응되는 ERROR_IO_PENDING(997)이 된다.

비동기 입출력 시 이 OVERLAPPED 구조체를 어떻게 유지하고 관리하는가가 매우 중요하다. 
입출력 과정에서 OVERLAPPED 구조체는 해당 장치와 더불어 계속 유지되어야 한다
주로 아래 형태로 많이 사용한다.

struct COPY_CHUNCK : OVERLAPPED
{
  HANDLE  _hfSrc, _hfDst;
  BYTE   _arBuff[BUFF_SIZE];
입출력 시 데이터를 주고받을 버퍼를 멤버 필드로 선언한다.
  COPY_CHUNCK(HANDLE hfSrc, HANDLE hfDst)
  {
   memset(this, 0, sizeof(*this));
생성자에서 본 구조체와 함께 OVERLAPPED 구조체를 초기화한다.
   _hfSrc =  hfSrc, _hfDst =  hfDst;
  }
};
typedef COPY_CHUNCK* PCOPY_CHUNCK;


 입출력에 대한 완료 통지를 받는 방법

 ● ● ● 장치 핸들 또는 이벤트 시그널링
CreateFile을 통해 열린 핸들값 자체 또는 OVERLAPPED 구조체의 hEvent 필드를 동기화 객체로 사용해 입출력
완료 여부의 통지를 수신한다.

● ● ● 경보가능 입출력 사용
비동기 프로시저 호출(APC)을 이용해 비동기 입출력 완료 후의 작업을 처리한다.

● ● ● 입출력 완료 포트 사용
입출력 완료 포트(IOCP) 커널 객체를 이용해 비동기 입출력 완료 후의 작업을 처리한다.

비동기 입출력에서 장치는 요구된 입출력을 계속수행 중이지만 그 입출력을 야기한 스레드는 
장치의 입출력 수행과는 상관없이 자신의 작업을 계속수행할 수 있다.

입출력이 완료된다는 것은 방금 든 예처럼 소켓을 통해 실제 데이터가 수신되어 성공적으로 완료되는 경우가 있고, 
수신 중에 에러가 발생해서 그 에러로 인해 입출력이 실패한 채로 완료될 수도있다. 
그리고 사용자가 개시한 입출력을 취소해서 입출력이 완료될 수도 있다. 따라서 입출력 완료통지를 받았을 때 
성공적인 완료인지 아니면 실패 또는 취소에 의한 완료인지를 체크해야 하며, 
이체크는 바로 OVERLAPPED 구조체의 Internal 필드를 통해서 이루어진다.
따라서 입출력 완료에대한 사후 처리는 성공적인 완료에 대한 처리뿐만 아니라, 
이러한 에러 또는 취소에 의한 완료 처리도 포함된다.
*/