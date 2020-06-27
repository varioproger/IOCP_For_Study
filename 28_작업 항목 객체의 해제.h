/*
CreateThreadpoolXxx를 통해서 생성된 작업 항목의 리소스를 제거하고자 할 때에는 다음의 함수를 사용한다.

VOID WINAPI CloseThreadpoolXxx(_Inout_ PTP_XXX pwa);

paw 매개변수는 당연히 CreateThreadpoolXxx의 리턴값인 TP _XXX 객체의 포인터다.
CreateThreadpoolXxx를 호출했을 시점에 만약 현재 실행 중인 콜백 항목이 없다면 즉각적으로
해당 객체는 해제된다. 물론 풀의 큐에 대기 중인 항목이 남아있다면 그 항목은 모두 취소된다.
만약 현재 실행 중인 콜백 항목이 존재한다면 우선 CloseThreadpoolXxx 호출에서 리턴되고, 실제 객체는 그 콜백 항목이 완료된 후에 해제된다. 
이런 경우라면 해당 객체의 정확한 해제 시점을 알 수없기 때문에, CloseThreadpoolXxx 호출 전에 
WaitForThreadpoolXxxCallbacks 함수를 호출해 현재 실행 중인 콜백 함수의 실행 완료를 기다리고 나서 
CloseThreadpoolXxx 함수를 호출하는 것이 좋다.

실행 중인 콜백 함수가 존재하는 상황에서 CloseThreadpoolXxx 함수의 성격을 이용한다면
XxxCallback 콜백 함수 내에서 CloseThreadpoolXxx 함수를 호출할 수도 있다. 
콜백 함수 내에서 CloseThreadpoolXxx를 호출하면 콜백 함수는 현재 실행 중이므로 CloseThreadpoolXxx 함수로부터 바로 리턴되고, 
그 후 콜백 함수가 리턴되면 비로소 TP _XXX 객체가 해제된다.
CloseThreadpoolXxx 호출 후 객체를 더 이상 사용할 수 없다는 점에 주의한다면 소켓 통신과 같이 
동적으로 TP_XXX 객체를 생성 및 해제해야 하는 솔루션에 이 방법을 유용하게 사용할 수 있다.
*/