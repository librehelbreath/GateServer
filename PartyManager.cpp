// PartyManager.cpp: implementation of the PartyManager class.
//
//////////////////////////////////////////////////////////////////////

#include "PartyManager.h"

extern char G_cTxt[120];
extern void PutLogList(char * cMsg);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

PartyManager::PartyManager(class CGateCore * pGateCore)
{
 int i;

	for (i = 0; i < DEF_MAXPARTY; i++) {
		m_iMemberNumList[i] = NULL;
		m_stMemberNameList[i].m_iPartyID = NULL;
		m_stMemberNameList[i].m_iIndex = NULL;
		m_stMemberNameList[i].m_dwServerChangeTime = NULL;
		ZeroMemory(m_stMemberNameList[i].m_cName, sizeof(m_stMemberNameList[i].m_cName)); 
	}

	m_pGateCore = pGateCore;
	m_dwCheckMemberActTime = timeGetTime();
}

PartyManager::~PartyManager()
{

}

int PartyManager::iCreateNewParty(int iClientH, char *pMasterName)
{
	int i, iPartyID;

	// 동일한 PartyMaster가 존재하는지 검색 
	for (i = 1; i < DEF_MAXPARTY; i++) 
	if ((m_stMemberNameList[i].m_iPartyID != NULL) && (strcmp(m_stMemberNameList[i].m_cName, pMasterName) == 0)) return 0;

	iPartyID = 0;
	for (i = 1; i < DEF_MAXPARTY; i++)
	if (m_iMemberNumList[i] == NULL) {
		// Party ID는 i, 멤버수 증가
		iPartyID = i;
		m_iMemberNumList[iPartyID]++;
		goto CNP_BREAKLOOP;
	}
	
	return 0;

CNP_BREAKLOOP:;
	// 멤버 이름을 등록한다.
	for (i = 1; i < DEF_MAXPARTY; i++) 
	if (m_stMemberNameList[i].m_iPartyID == NULL) {
		m_stMemberNameList[i].m_iPartyID = iPartyID;
		ZeroMemory(m_stMemberNameList[i].m_cName, sizeof(m_stMemberNameList[i].m_cName));
		strcpy(m_stMemberNameList[i].m_cName, pMasterName);
		m_stMemberNameList[i].m_iIndex = iClientH;
		
		//testcode
		//wsprintf(G_cTxt, "New party(ID:%d Master:%s)", iPartyID, pMasterName);
		//PutLogList(G_cTxt);
		
		return iPartyID;
	}

	return 0;
}

BOOL PartyManager::bDeleteParty(int iPartyID)
{
 int i;
 BOOL bFlag;
 char * cp, cData[120];
 DWORD * dwp;
 WORD * wp;

	bFlag = FALSE;
	m_iMemberNumList[iPartyID] = NULL;

	for (i = 1; i < DEF_MAXPARTY; i++)
	if (m_stMemberNameList[i].m_iPartyID == iPartyID) {
		m_stMemberNameList[i].m_iPartyID = NULL;
		m_stMemberNameList[i].m_iIndex = NULL;
		m_stMemberNameList[i].m_dwServerChangeTime = NULL;
		ZeroMemory(m_stMemberNameList[i].m_cName, sizeof(m_stMemberNameList[i].m_cName));
		bFlag = TRUE;
	}

	// 모든 게임 서버에게 파티가 사라졌음을 알려준다.
	ZeroMemory(cData, sizeof(cData));
	cp = (char *)cData;
	dwp = (DWORD *)cp;
	*dwp = MSGID_PARTYOPERATION;
	cp += 4;
	wp = (WORD *)cp;
	*wp = 2; // Code 2: 파티 해산 
	cp += 2; 
	wp = (WORD *)cp;
	*wp = iPartyID;
	cp += 2;
	m_pGateCore->SendMsgToAllGameServers(NULL, cData, 10, TRUE);

	//testcode
	//wsprintf(G_cTxt, "Delete party(ID:%d)", iPartyID);
	//PutLogList(G_cTxt);

	return bFlag;
}

BOOL PartyManager::bAddMember(int iClientH, int iPartyID, char *pMemberName)
{
 int i;


	if (m_iMemberNumList[iPartyID] >= DEF_MAXPARTYMEMBER) return FALSE;

	// 이미 등록된 파티 멤버인지 확인
	for (i = 1; i < DEF_MAXPARTY; i++)
		if ((m_stMemberNameList[i].m_iPartyID != NULL) && (strcmp(m_stMemberNameList[i].m_cName, pMemberName) == 0))
		{
			m_stMemberNameList[i].m_iPartyID = NULL;
			m_stMemberNameList[i].m_iIndex = NULL;
			m_stMemberNameList[i].m_dwServerChangeTime = NULL;
			ZeroMemory(m_stMemberNameList[i].m_cName, sizeof(m_stMemberNameList[i].m_cName));
			
			m_iMemberNumList[iPartyID]--;
			if (m_iMemberNumList[iPartyID] <= 1) bDeleteParty(iPartyID); // 파티원이 1명만 남으면 파티 제거
		}
//		return FALSE;

	
	for (i = 1; i < DEF_MAXPARTY; i++)
	if (m_stMemberNameList[i].m_iPartyID == NULL) {
		m_stMemberNameList[i].m_iPartyID = iPartyID;
		m_stMemberNameList[i].m_iIndex = iClientH;
		m_stMemberNameList[i].m_dwServerChangeTime = NULL;
		ZeroMemory(m_stMemberNameList[i].m_cName, sizeof(m_stMemberNameList[i].m_cName));
		strcpy(m_stMemberNameList[i].m_cName, pMemberName);
		m_iMemberNumList[iPartyID]++;

		//testcode
		//wsprintf(G_cTxt, "Add Member: PartyID:%d Name:%s", iPartyID, pMemberName);
		//PutLogList(G_cTxt);
		return TRUE;
	}

	return FALSE;
}

BOOL PartyManager::bRemoveMember(int iPartyID, char *pMemberName)
{
 int i;

	for (i = 1; i < DEF_MAXPARTY; i++)
	if ((m_stMemberNameList[i].m_iPartyID == iPartyID) && (strcmp(m_stMemberNameList[i].m_cName, pMemberName) == 0)) {

		m_stMemberNameList[i].m_iPartyID = NULL;
		m_stMemberNameList[i].m_iIndex = NULL;
		m_stMemberNameList[i].m_dwServerChangeTime = NULL;
		ZeroMemory(m_stMemberNameList[i].m_cName, sizeof(m_stMemberNameList[i].m_cName));
		
		m_iMemberNumList[iPartyID]--;
		if (m_iMemberNumList[iPartyID] <= 1) bDeleteParty(iPartyID); // 파티원이 1명만 남으면 파티 제거 

		//testcode
		//wsprintf(G_cTxt, "Remove Member: PartyID:%d Name:%s", iPartyID, pMemberName);
		//PutLogList(G_cTxt);
		return TRUE;
	}

	return FALSE;
}


BOOL PartyManager::bCheckPartyMember(int iClientH, int iGSCH, int iPartyID, char *pName)
{
 int i, iRet;
 char * cp, cData[120];
 DWORD * dwp;
 WORD * wp;

	for (i = 1; i < DEF_MAXPARTY; i++)
	if ((m_stMemberNameList[i].m_iPartyID == iPartyID) && (strcmp(m_stMemberNameList[i].m_cName, pName) == 0)) {
		// 서버 이동 타임 클리어 
		m_stMemberNameList[i].m_dwServerChangeTime = NULL;
		return TRUE;
	}

	// 파티 멤버가 아니다. 클리어를 요청한다.
	ZeroMemory(cData, sizeof(cData));
	cp = (char *)cData;
	dwp = (DWORD *)cp;
	*dwp = MSGID_PARTYOPERATION;
	cp += 4;
	wp = (WORD *)cp;
	*wp = 3; // 파티 멤버가 아니라는 코드
	cp += 2; 
	wp = (WORD *)cp;
	*wp = iGSCH;
	cp += 2;
	memcpy(cp, pName, 10);
	cp += 10;
	iRet = m_pGateCore->m_pClientList[iClientH]->m_pXSock->iSendMsg(cData, 20);
	
	return FALSE;
}

BOOL PartyManager::bGetPartyInfo(int iClientH, int iGSCH, char * pName, int iPartyID)
{
 int i, iRet, iTotal;
 char * cp, cData[1024];
 DWORD * dwp;
 WORD * wp, * wpTotal;

	ZeroMemory(cData, sizeof(cData));
	cp = (char *)cData;
	dwp = (DWORD *)cp;
	*dwp = MSGID_PARTYOPERATION;
	cp += 4;
	wp = (WORD *)cp;
	*wp = 5; // 파티 정보라는 의미의 코드
	cp += 2; 
	wp = (WORD *)cp;
	*wp = iGSCH;
	cp += 2;
	memcpy(cp, pName, 10);
	cp += 10;
	wp = (WORD *)cp;
	wpTotal = wp;
	cp += 2;

	iTotal = 0;
	for (i = 1; i < DEF_MAXPARTY; i++)
	if ((m_stMemberNameList[i].m_iPartyID == iPartyID) && (m_stMemberNameList[i].m_iPartyID != NULL)) {
		memcpy(cp, m_stMemberNameList[i].m_cName, 10);
		cp += 11;
		iTotal++;
	}
	
	*wpTotal = iTotal;
	iRet = m_pGateCore->m_pClientList[iClientH]->m_pXSock->iSendMsg(cData, 20 + iTotal*11 +1);

	return TRUE;
}

void PartyManager::GameServerDown(int iClientH)
{
 int i;

	for (i = 0; i < DEF_MAXPARTY; i++)
	if (m_stMemberNameList[i].m_iIndex == iClientH) {
		//testcode
		//wsprintf(G_cTxt, "Removing Party member(%s) by Server down", m_stMemberNameList[i].m_cName);
		//PutLogList(G_cTxt);

		m_iMemberNumList[m_stMemberNameList[i].m_iPartyID]--;
		m_stMemberNameList[i].m_iPartyID  = NULL;
		m_stMemberNameList[i].m_iIndex    = NULL;
		m_stMemberNameList[i].m_dwServerChangeTime = NULL;
		ZeroMemory(m_stMemberNameList[i].m_cName, sizeof(m_stMemberNameList[i].m_cName));
	}
}

void PartyManager::SetServerChangeStatus(char *pName, int iPartyID)
{
 int i;

	for (i = 1; i < DEF_MAXPARTY; i++)
	if ((m_stMemberNameList[i].m_iPartyID == iPartyID) && (strcmp(m_stMemberNameList[i].m_cName, pName) == 0)) {
		m_stMemberNameList[i].m_dwServerChangeTime = timeGetTime();
		return;
	}
}

void PartyManager::CheckMemberActivity()
{
 int i;
 DWORD * dwp, dwTime = timeGetTime();
 char * cp, cData[120];
 WORD * wp;

	if ((dwTime - m_dwCheckMemberActTime) > 1000*2) {
		m_dwCheckMemberActTime = dwTime;
	} else return;

	for (i = 1; i < DEF_MAXPARTY; i++)
	if ((m_stMemberNameList[i].m_dwServerChangeTime != NULL) && ((dwTime - m_stMemberNameList[i].m_dwServerChangeTime) > 1000*20)) {
		// 시간 초과. 
		ZeroMemory(cData, sizeof(cData));
		cp = (char *)cData;
		dwp = (DWORD *)cp;
		*dwp = MSGID_PARTYOPERATION;
		cp += 4;
		wp = (WORD *)cp;
		*wp = 6; // 멤버 제거에 대한 응답이다.
		cp += 2;
		*cp = 1; // 제거 성공 
		cp++;
		wp = (WORD *)cp;
		*wp = NULL;  // 인덱스는 0 
		cp += 2;
		memcpy(cp, m_stMemberNameList[i].m_cName, 10);
		cp += 10;
		wp = (WORD *)cp;
		*wp = (WORD)m_stMemberNameList[i].m_iPartyID;
		cp += 2;
		// 파티 멤버가 제거되었다는 내용은 모든 서버에 전송한다.
		m_pGateCore->SendMsgToAllGameServers(NULL, cData, 22, TRUE);

		bRemoveMember(m_stMemberNameList[i].m_iPartyID, m_stMemberNameList[i].m_cName);
	}
}
