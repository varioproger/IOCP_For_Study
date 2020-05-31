/*
VOID WINAPI WaitForThreadpoolIoCallbacks
(
  _Inout_ PTP_IO  pti,
  _In_    BOOL     fCancelPendingCallbacks
);

앞서 설명했던 WaitForThreadpoolXxxCallbacks 함수들처럼 입출력 객체 역시 현재 실행 중
인 콜백 함수가 있다면 실행이 끝날 때까지 기다리거나, 실행 대기 중인 콜백 항목을 취소하기 위해
WaitForThreadpoolIoCallbacks 함수를 사용할 수 있다. 특히 비동기 입출력을 다룰 경우에는
중첩된 입출력이 가능하므로, 하나의 장치에 대해 비동기적으로 여러 입출력을 요청했을 경우에는
fCancelPendingCallbacks 매개변수를 TRUE로 지정해 실행 대기 중인 여러 입출력 항목을 모
두 취소할 수 있는 장점이 있다

TP_IO 객체를 사용하게 되면 CreateThreadpoolIo 호출 시 pv 매개변수를 통해 콜백 함수로 부가적인 참조 정보를 전달할 수 있고, 
또한 WaitForThreadpoolIoCallbacks 함수를 통해 현재 실행 중인 콜백 함수가 있다면 그 실행이 완료될 때까지 대기할 수 있다.

TP_IO_0, TP_IO_1 소스 참조
*/