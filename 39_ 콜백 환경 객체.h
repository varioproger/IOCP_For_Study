/*
콜백 환경은 TP_CALLBACK_ENVIRON 구조체로 표현되는 유저 영역의 객체며, 그 정의는 다음과 같이 “WinNT.h”에 나와 있다.

typedef struct _TP_CALLBACK_ENVIRON_V3
{
  TP_VERSION                          Version;
  PTP_POOL                            Pool;
  PTP_CLEANUP_GROUP                  CleanupGroup;
  PTP_CLEANUP_GROUP_CANCEL_CALLBACK   CleanupGroupCancelCallback;
  PVOID                           RaceDll;
  struct _ACTIVATION_CONTEXT*     ActivationContext;
  PTP_SIMPLE_CALLBACK                  FinalizationCallback;
  union
  {
   DWORD                           Flags;
   struct
   {
     DWORD                       LongFunction  :  1;
    DWORD                       Persistent    :  1;
    DWORD                       Private      : 30;
   } s;
  } u;
  TP_CALLBACK_PRIORITY               CallbackPriority;
  DWORD                              Size;
} TP_CALLBACK_ENVIRON_V3;
typedef TP_CALLBACK_ENVIRON_V3 TP_CALLBACK_ENVIRON, *PTP_CALLBACK_ENVIRON;


inline VOID TpSetCallbackThreadpool(PTP_CALLBACK_ENVIRON pcb, PTP_POOL Pool)
{
    pcb->Pool = Pool;
}

inline VOID TpSetCallbackCleanupGroup(PTP_CALLBACK_ENVIRON pcb,
    PTP_CLEANUP_GROUP pcg, PTP_CLEANUP_GROUP_CANCEL_CALLBACK pnfcbCancel)
{
    pcb->CleanupGroup =  pcg;
    pcb->CleanupGroupCancelCallback = pnfcbCancel;
}

inline VOID TpSetCallbackRaceWithDll(PTP_CALLBACK_ENVIRON pcb, PVOID DllHandle)
{
    pcb->RaceDll =  DllHandle;
}

inline VOID TpSetCallbackLongFunction(PTP_CALLBACK_ENVIRON pcb)
{
    pcb->u.s.LongFunction = 1;
}

inline VOID TpSetCallbackPersistent(_Inout_ PTP_CALLBACK_ENVIRON CallbackEnviron)
{
    CallbackEnviron->u.s.Persistent =  1;
}

inline VOID TpSetCallbackPriority(PTP_CALLBACK_ENVIRON pcb, TP_CALLBACK_PRIORITY Priority)
{
    pcb->CallbackPriority =  Priority;
}


콜백 환경의 필드를 설정하기 전에 우선 콜백 환경을 초기화해야 한다. 콜백 환경을 초기화하고 해제하는 함수

FORCEINLINE VOID InitializeThreadpoolEnvironment(_Out_ PTP_CALLBACK_ENVIRON pcbe)
{
  TpInitializeCallbackEnviron(pcbe);
}
FORCEINLINE VOID DestroyThreadpoolEnvironment(_Inout_ PTP_CALLBACK_ENVIRON pcbe)
{
  TpDestroyCallbackEnviron(pcbe);
}

TpInitializeCallbackEnviron과 TpDestroyCallbackEnviron 함수는 WinNT.h 헤더파일에 다음과 같이 정의되어 있다

FORCEINLINE VOID
TpInitializeCallbackEnviron(_Out_ PTP_CALLBACK_ENVIRON CallbackEnviron)
{
#if (_WIN32_WINNT >=  _WIN32_WINNT_WIN7)
    CallbackEnviron->Version =  3;
#else
    CallbackEnviron->Version =  1;
#endif
    CallbackEnviron->Pool = NULL;
    CallbackEnviron->CleanupGroup = NULL;
        ⋮
#if (_WIN32_WINNT >=  _WIN32_WINNT_WIN7)
    CallbackEnviron->CallbackPriority = TP_CALLBACK_PRIORITY_NORMAL;
    CallbackEnviron->Size = sizeof(TP_CALLBACK_ENVIRON);
#endif
}
FORCEINLINE VOID
TpDestroyCallbackEnviron(_In_ PTP_CALLBACK_ENVIRON CallbackEnviron)
{
  UNREFERENCED_PARAMETER(CallbackEnviron);
}

TpInitializeCallbackEnviron 인라인 함수는 매개변수로 전달된 콜백 환경 구조체를 단순히 초기화하고, 
TpDestroyCallbackEnviron 인라인 함수는 단순히 함수만 정의되어 있다는 것을 확인할 수 있다.

  TP_CALLBACK_ENVIRON ce;
  InitializeThreadpoolEnvironment(&ce);
            ⋮
  DestroyThreadpoolEnvironment(&ce);


























*/