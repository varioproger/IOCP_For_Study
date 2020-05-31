/*
TP_WORK 콜백 함수를 비동기적으로 호출
TP_WAIT 커널 객체에 신호가 전달되거나 대기 시간이 초과되면 콜백 함수 호출
TP_TIMER 예약 시간에 콜백 함수 호출
TP_IO 비동기 I/O가 완료되면 콜백 함수 호출
TP_POOL 콜백을 실행하는 데 사용되는 스레드의 풀
TP_CALLBACK_ENVIRON 스레드 풀을 해당 콜백 및 선택적으로 정리그룹에 바인딩
TP_CLEANUP_GROUP 한 개 이상의 스레드 풀 콜백 객체 추적


이 4개의 카테고리마다 다음과 같이 개별 객체를 별도로 정의하여 제공된다.

  ● 작업(TP_WORK) : 어떤 특정 함수를 비동기적으로 호출하고자 하는 경우
  ● 대기(TP_WAIT) : 임의의 커널 객체가 시그널 되었을 때 반응하기 위해
  ● 타이머(TP_TIMER) : 여러 타이머 군들이 만기되었을 때 반응하기 위해
  ● 입출력(TP_IO) : IOCP의 완료 콜백을 수행하고자 하는 경우

객체를 생성하고 삭제하며 완료 시 실행 중인 콜백 함수의 종료를 기다리기 위한 함수들을 다음의 형태로 제공한다. 
여기에서의 Xxx는 각각 Work, Wait, Timer, Io를 의미한다.
  ● CreateThreadpoolXxx : TP_XXX 객체 생성
  ● CloseThreadpoolXxx : TP_XXX 객체 삭제
  ● WaitForThreadpoolXxxCallback : 콜백 함수 PTP_XXX_CALLBACK의 실행 완료 대기

*/