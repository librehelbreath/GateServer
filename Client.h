// Client.h: interface for the CClient class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLIENT_H__2CB6AC23_DFA6_11D2_B145_00001C7030A6__INCLUDED_)
#define AFX_CLIENT_H__2CB6AC23_DFA6_11D2_B145_00001C7030A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif  // _MSC_VER >= 1000

#include <windows.h>
#include "UserMessages.h"
#include "XSocket.h"

#define DEF_MSGBUFFERSIZE 30000

class CClient {
 public:
  CClient(HWND hWnd);
  virtual ~CClient();

  BOOL m_bIsGameServer;     // 게임서버인가 모니터인가 판별용
  BOOL m_bIsShutDown;       // v2.17 셧다운 중인가 판별용
  char m_cName[11];         // 게임 서버의 이름.
  char m_cAddress[16];      // 게임 서버의 주소
  DWORD m_dwProcessHandle;  // 게임서버의 프로세스 핸들
  DWORD m_dwAliveTime;  // 게임서버로부터의 살아있음을 알리는 메시지
  int m_iTotalClients;  // 게임서버의 총 사용자 수
  // v2.17 2002-6-3 고광현 수정
  short m_sIsActivated;

  class XSocket* m_pXSock;
};

#endif  // !defined(AFX_CLIENT_H__2CB6AC23_DFA6_11D2_B145_00001C7030A6__INCLUDED_)
