// PartyManager.h: interface for the PartyManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined( \
    AFX_PARTYMANAGER_H__ED26235F_8BA2_493A_AF52_397C25D4CF5C__INCLUDED_)
#define AFX_PARTYMANAGER_H__ED26235F_8BA2_493A_AF52_397C25D4CF5C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif  // _MSC_VER > 1000

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "GateCore.h"

#define DEF_MAXPARTY 5000
#define DEF_MAXPARTYMEMBER 8

class PartyManager {
 public:
  void CheckMemberActivity();
  void SetServerChangeStatus(char *pName, int iPartyID);
  void GameServerDown(int iClientH);
  BOOL bGetPartyInfo(int iClientH, int iGSCH, char *pName, int iPartyID);
  BOOL bCheckPartyMember(int iClientH, int iGSCH, int iPartyID, char *pName);
  BOOL bRemoveMember(int iPartyID, char *pMemberName);
  BOOL bAddMember(int iClientH, int iPartyID, char *pMemberName);
  BOOL bDeleteParty(int iPartyID);
  int iCreateNewParty(int iClientH, char *pMasterName);
  PartyManager(class CGateCore *pGateCore);
  virtual ~PartyManager();

  int m_iMemberNumList[DEF_MAXPARTY];

  struct {
    int m_iPartyID, m_iIndex;
    char m_cName[12];
    DWORD m_dwServerChangeTime;
  } m_stMemberNameList[DEF_MAXPARTY];

  class CGateCore *m_pGateCore;
  DWORD m_dwCheckMemberActTime;
};

#endif  // !defined(AFX_PARTYMANAGER_H__ED26235F_8BA2_493A_AF52_397C25D4CF5C__INCLUDED_)
