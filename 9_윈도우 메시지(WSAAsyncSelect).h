/*
윈도우 메시지 방식은 소켓의 상태가 변경되면 윈도우 메시지를 이용해서 유저가 생성한 윈도우로 
유저가 직접 정의한 메시지를 전송해주는 방식이다. 
해당 메시지가 전송되면 윈도우 프로시저에서 그 메시지를 받아 변경된 상태의 종류를 체크해서 그 변화에 대한 처리를 수행해준다.

int WSAAsyncSelect
(
    _In_ SOCKET     s,
    _In_ HWND     hWnd,
    _In_ unsigned int  wMsg,
    _In_ long      lEvent
);

SOCKET s
상태 체크를 수행할 대상 소켓이다.

HWND hWnd
unsigned  int wMsg
메시지를 수신받을 윈도우 핸들과 그 메시지를 식별하는 유저가 정의한 고유 메시지 값이다.
예를 들어 다음과 같이 유저 정의 메시지를 정의하자.

#define WM_ASYNC_SOCKET WM_USER + 500

정의된 메시지를 wMsg 매개변수로, 
이 통지를 받을 윈도우 핸들을 hWnd 매개변수로 지정해서 WSAAsyncSelect 함수를 호출하면 상태 변화가 발생했을 때 
시스템은 이 메시지를통해 그 상태 변화를 통지해준다. 
이 메시지의 WPARAM과 LPARAM 값으로 다음의 값을 전달해준다

●   WPARAM : 상태가 변경된 소켓의 핸들이다. 아래와 같이 형 변환을 이용해서 해당 소켓의 핸들을 획득할 수가 있다.
  SOCKET sock =  (SOCKET)wParam;

●   LPARAM  : 상태 변경 식별 플래그의 조합값을 담고 있는 하위 워드와 에러 코드를 담고 있는 상위 워드로 구성된LONG 타입의 값이다. 
다음의 매크로를 통해서 각각의 값을 획득할 수 있다.

  #define WSAGETSELECTEVENT (lParam)  LOWORD(lParam)
  #define WSAGETSELECTERROR (lParam)  HIWORD(lParam)

  - WSAGETSELECTERROR(lParam) : 통신상의 에러가 발생하면 에러값을 돌려준다.
  - WSAGETSELECTEVENT(lParam)  : 변경된 상태를 지시하는 비트 플래그의 집합이다. 
  이 값을 체크하여 해당되는 작업을 수행하면 된다. 우리가 관심을 가질 만한 비트 플래그 값은 다음과 같다.

FD_ACCEPT 소켓 s가 새로 들어온 접속을 받아들일 준비가 되었다. accept 함수를 호출할 수 있다.
FD_READ 데이터가 수신되었고 소켓 s가 읽어들일 준비가 되었다. recv 함수를 호출할 수 있다.
FD_CONNECT 소켓 s가 요청한 연결이 성립되었다. 이제 send나 recv 함수를 호출할 수 있다.
FD_WRITE 소켓 s의 데이터 전송이 완료되었다.
FD_CLOSE 리모트 측에서 소켓 s에 의해 성립된 연결을 닫았다. closesocket을 호출할 수 있다.

long  lEvent
통지받기 원하는 상태를 식별하는 비트값을 OR 조합을 통해 넘겨준다. 
상태 식별 비트의 정의는 WSAGETSELECTEVENT의 값들이다. 
만약 읽기와 쓰기의 상태 변경을 함께 받고자 한다면 다음과 같이 설정하면 된다.

  WSAASyncSelect(..., FD_READ | FD_WRITE);

WSAAsyncSelect 함수로 상태 변경을 통지받고자 하는 모든 소켓에 대해 WSAAsyncSelect 함수를 호출해줘야 한다.
시스템이 메시지를 통해 실제로 변경된 소켓의 핸들을 WPARAM을 통해서 전달해준다.


WSAAsyncSelect 소스 참조
*/
