/*
스레드 풀 내의 작업자 스레드의 스택 크기를 지정할 수 있는 함수

BOOL SetThreadpoolStackInformation
(
  _Inout_ PTP_POOL          ptpp,
  _In_   PTP_POOL_STACK_INFORMATION   ptpsi
);
SetThreadpoolStackInformation 함수는 스레드 풀의 작업자 스레드의 스택 예약 크기와 커밋 크기를 설정

BOOL QueryThreadpoolStackInformation
(
  _In_   PTP_POOL          ptpp,
  _Out_   PTP_POOL_STACK_INFORMATION  ptpsi
);
QueryThreadpoolStackInformation 함수는 현재 스레드 풀에 설정된 작업자 스레드의 스택 예약 및 커밋 크기를 문의

스택의 예약 및 커밋 크기 설정은 TP_POOL_STACK_INFORMATION 구조체로 표현
typedef struct _TP_POOL_STACK_INFORMATION
{
  SIZE_T  StackReserve;   // 스택 예약 크기
  SIZE_T  StackCommit;   // 스택 커밋 크기
} TP_POOL_STACK_INFORMATION, *PTP_POOL_STACK_INFORMATION;


*/
