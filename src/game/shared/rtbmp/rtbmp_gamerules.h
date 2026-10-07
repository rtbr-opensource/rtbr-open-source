//========= Copyright RTBMP Team, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef RTBMP_GAMERULES_H
#define RTBMP_GAMERULES_H
#pragma once

#include "hl2mp_gamerules.h"

#ifdef CLIENT_DLL
#define CRTBMPRules C_RTBMPRules
#define CRTBMPGameRulesProxy C_RTBMPGameRulesProxy
#endif

enum RTBMP_GameMode
{
	GAMETYPE_DEATHMATCH = 0, // also includes team deathmatch
	GAMETYPE_CTF,
};

class CRTBMPGameRulesProxy : public CHL2MPGameRulesProxy
{
public:
	DECLARE_CLASS( CRTBMPGameRulesProxy, CHL2MPGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CRTBMPRules : public CHL2MPRules
{
public:
	DECLARE_CLASS( CRTBMPRules, CHL2MPRules );

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
#endif

	CRTBMPRules();

	virtual void Think( void );
	virtual void CreateStandardEntities( void );
	virtual void ClientSettingsChanged( CBasePlayer *pPlayer );
	virtual const char *GetGameDescription( void );
	virtual const unsigned char *GetEncryptionKey( void ) { return (unsigned char *)"q9y6N081"; }
	virtual const char *GetGameTypeName( void ){ return NULL; }
	virtual int GetGameType( void ) { return m_iGameType; }
	void SetGameType( int type ) { m_iGameType = type; }

	bool IsSprintInfinite( void );

#ifndef CLIENT_DLL
	float FlSpawnProtectionTime( void );
#endif

	virtual bool  FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return false; }

#ifdef GAME_DLL
	virtual void BalanceTeams( bool bRequireSwitcheesToBeDead );
#endif // GAME_DLL

	bool		SwitchedTeamsThisRound( void ) { return m_bSwitchedTeamsThisRound; }
	virtual bool ShouldBalanceTeams( void );

	bool WouldChangeUnbalanceTeams( int iNewTeam, int iCurrentTeam );
	bool AreTeamsUnbalanced( int &iHeaviestTeam, int &iLightestTeam );

private:
	CNetworkVar( int, m_iGameType );
	CNetworkVar( bool, m_bSwitchedTeamsThisRound );

	float m_flStartBalancingTeamsAt;
	float m_flNextBalanceTeamsTime;
	bool m_bPrintedUnbalanceWarning;
	float m_flFoundUnbalancedTeamsTime;

	float	m_flAutoBalanceQueueTimeEnd;
	int		m_nAutoBalanceQueuePlayerIndex;
	int		m_nAutoBalanceQueuePlayerScore;
};

inline CRTBMPRules* RTBMPRules()
{
	return static_cast<CRTBMPRules*>(g_pGameRules);
}

#endif //RTBMP_GAMERULES_H
