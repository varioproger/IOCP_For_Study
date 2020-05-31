/*
콜백 함수가 실행 중이면 그 실행이 완료될 때까지 기다릴 수 있게 해주는 기능을 제공하며, 
더 나아가서 6.4.3절에서 설명할 정리그룹이라는 객체를 통해서 콜백 함수의 실행이 완료될 때마다 
또 다른 콜백 함수를 통해서 그 사실을 통지해주는 기능까지 제공한다.

VOID WINAPI WaitForThreadpoolXxxCallbacks
(
  _Inout_ PTP_XXX pwa,
  _In_   BOOL   fCancelPendingCallbacks
);

PTP_XXX pwa
CreateThreadpoolXxx 함수를 통해서 생성된 작업 항목의 포인터를 넘겨준다.

BOOL  fCancelPendingCallbacks
아직 실행되지 않고 큐에 대기 중인 콜백을 취소할지의 여부를 지정한다.
  ●   FALSE인 경우, 현재 실행 중인 콜백의 완료뿐만 아니라, 아직 실행되지 않은 콜백 항목이 큐에 남아 있으면 그 콜백까지 실행한 후 리턴한다.
  ●   TRUE인 경우, 현재 실행 중인 콜백이 완료되면 큐에 남아 있는 콜백 항목은 취소하고 즉시 리턴한다.

현재 실행 중인 콜백 함수의 실행 완료 대기 기능과 더불어 실행되기를 기다리며, 
스레드 풀의 작업 큐에서 대기 중인 콜백 항목에 대한 취소 처리를 할 수 있다. 
주의할 것은 이 “취소”라는 의미가 현재 실행 중인 콜백 함수의 실행 자체를 취소한다는 의미는 아니라는 점이다. 
실행 중인 콜백 함수는 여러분이 별도의 취소 처리를 추가하지 않는다면 취소할 수 없으며, 단지 그 실행이 완료되기만을 기다릴 수 있다.


  ●   작업 : SubmitThreadpoolWork
  ●   대기 : SetThreadpoolWait
  ●   타이머 : SetThreadpoolTimer
  ●   입출력 : StartThreadpoolIo

위 함수들을 이용해 TP_XXX 객체에 대해 콜백 항목 A, B, C를 추가했다고 하자.
그리고 실행될 조건(시그널 상태 또는 입출력 완료 등)이 만족되어 스레드 풀이 큐에 존재하는 A는 실행 중이고 
B, C는 여전히 큐에 남아 있는 상태라고 하자. 이 상태에서 여러분은 프로그램 종료를 위해서 WaitForThreadpoolXxxCallbacks을 호출했다고 하자. 
그러면 WaitForThread-poolXxxCallbacks 호출 시 fCancelPendingCallbacks 매개변수를 TRUE로 넘겨주면 
이 함수는 현재 실행 중인 콜백 함수 A의 실행이 완료되기를 기다리다 A가 완료되면 큐에 남아 있던 B,C를 실행하지 않고 
그 항목을 제거한 후 리턴된다. 만약 fCancelPendingCallbacks 매개변수를 FALSE로 넘겨주면 현재 실행 중인 A의 실행 완료뿐만 아니라 
큐에 존재하던 B와 C가 모두 실행될 때까지 대기하게 된다. 
따라서, fCancelPendingCallbacks를 FALSE로 넘겨준 상태에서 A가 실행이 완료되고 이제 B가 호출되어 B의 완료를 기다리던 중에 
조건이 만족되어 콜백 항목 D가 큐에 새롭게 추가되었다면 WaitForThreadpoolXxxCallbacks는 기존의 B와 C뿐만 아니라 D가 실행이 완료될 때까지 
대기하게 된다. 따라서 fCancelPendingCallbacks 매개변수를 FALSE로 지정할 경우, 
이러한 상황으로 인해 계속 대기하게 되는 사태가 발생될 수 있기 때문에 주의해야 한다.

WaitForThreadpoolXxxCallbacks 함수는 만약 현재 실행 중인 콜백 함수가 있다면 대기하고 있기 때문에, 
콜백 함수 내에서 이 함수를 호출할 경우에는 데드락 상황에 빠지게 된다.
*/
