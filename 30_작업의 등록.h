/*
윈도우 2000의 QueueUserWorkItem의 경우는 호출과 더불어 여러분이 정의한 콜백 함수가 바로 스레드 풀에 의하여 실행된다. 
하지만 새로운 스레드 풀의 경우는 바로 실행되지 않고 다음의 함수를 호출해줘야 실행이 된다.

새로운 스레드 풀의 경우는 바로 실행되지 않고 다음의 함수를 호출해줘야 실행이 된다.

VOID WINAPI SubmitThreadpoolWork(_Inout_ PTP_WORK pwk);

매개변수 pwk는 CreateThreadpoolWork 함수에 의해 생성된 TP_WORK 작업 항목 구조체의 포인터다.

TPoolWorkVista 소스 참조
EchoSvrTPWork 소스 참조
*/