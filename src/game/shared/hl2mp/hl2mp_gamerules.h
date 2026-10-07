//========= Copyright Valve Corporation, All rights reserved. ============//
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

#ifndef HL2MP_GAMERULES_H
#define HL2MP_GAMERULES_H
#pragma once

#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "gamevars_shared.h"
#ifdef MAPBASE
#include "hl2_gamerules.h"
#endif

#ifndef CLIENT_DLL
#include "hl2mp_player.h"
#endif

#ifdef MAPBASE
// Adds support for a basic co-op mode which makes unteamed players friendly and is recognized as "co-op" by the game.
// Comment out this definition if this is not desired.
#define BASIC_HL2MP_COOP 1
#endif

#define VEC_CROUCH_TRACE_MIN	HL2MPRules()->GetHL2MPViewVectors()->m_vCrouchTraceMin
#define VEC_CROUCH_TRACE_MAX	HL2MPRules()->GetHL2MPViewVectors()->m_vCrouchTraceMax

enum
{
	TEAM_COMBINE = 2,
	TEAM_REBELS,
};


#ifdef CLIENT_DLL
	#define CHL2MPRules C_HL2MPRules
	#define CHL2MPGameRulesProxy C_HL2MPGameRulesProxy
#endif

#ifdef HL2MP_USES_HL2_GAMERULES
	#define CHL2MPRulesBase CHalfLife2
	#define CHL2MPRulesProxyBase CHalfLife2Proxy
#else
	#define CHL2MPRulesBase CTeamplayRules
	#define CHL2MPRulesProxyBase CGameRulesProxy
#endif

class CHL2MPGameRulesProxy : public CHL2MPRulesProxyBase
{
public:
	DECLARE_CLASS( CHL2MPGameRulesProxy, CHL2MPRulesProxyBase );
	DECLARE_NETWORKCLASS();

#if defined(MAPBASE) && defined(GAME_DLL)
	bool KeyValue( const char *szKeyName, const char *szValue );
	bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen );

	virtual int	Save( ISave &save );
	virtual int	Restore( IRestore &restore );

	virtual void Activate( void ) OVERRIDE;

	void	TeamPlayerCountChanged( CTeam *pTeam );
	void	PlayerCountChanged( void );
	int		CountActivePlayers( void );

	// Inputs
	void InputSetAllowDefaultItems( inputdata_t &inputdata );
	void InputSetAllowDefaultSuit( inputdata_t &inputdata );
	// Get number of "active" players, i.e. connected players who aren't in spectator
	void InputGetPlayerCount( inputdata_t &inputdata );
#ifdef RTBMP
	// Player respawning in co-op
	void InputForceRespawnAllPlayers( inputdata_t &inputdata );
	void InputEnableRespawn( inputdata_t &inputdata );
	void InputDisableRespawn( inputdata_t &inputdata );
	bool RespawnEnabled( void ) { return m_bRespawnEnabled; }

	void InputEnableReadyVote( inputdata_t &inputdata );
	void InputDisableReadyVote( inputdata_t &inputdata );
	void InputResetPlayerReadyVotes( inputdata_t &inputdata );
	void OnPlayerVoteReady( int iPlayers, CBasePlayer *pPlayer );
#endif

	bool m_save_AllowDefaultItems;
	bool m_save_AllowDefaultSuit;

	DECLARE_DATADESC();

private:
#ifdef BASIC_HL2MP_COOP
	bool m_bCoOpEnabled;
	bool m_bCoOpPickupRespawn;
#ifdef RTBMP
	bool m_bRespawnEnabled = true;
#endif
#endif

	COutputInt m_OnGetPlayerCount;
	COutputInt m_OnPlayerCountChanged;
	COutputInt m_OnTeam1PlayersChanged;
	COutputInt m_OnTeam2PlayersChanged;
#ifdef RTBMP
	COutputInt m_OnPlayerVoteReady;
#endif

#endif
};

class HL2MPViewVectors : public CViewVectors
{
public:
	HL2MPViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vCrouchTraceMin,
		Vector vCrouchTraceMax ) :
			CViewVectors( 
				vView,
				vHullMin,
				vHullMax,
				vDuckHullMin,
				vDuckHullMax,
				vDuckView,
				vObsHullMin,
				vObsHullMax,
				vDeadViewHeight )
	{
		m_vCrouchTraceMin = vCrouchTraceMin;
		m_vCrouchTraceMax = vCrouchTraceMax;
	}

	Vector m_vCrouchTraceMin;
	Vector m_vCrouchTraceMax;	
};

class CHL2MPRules : public CHL2MPRulesBase
{
public:
	DECLARE_CLASS( CHL2MPRules, CHL2MPRulesBase );

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
#endif
	
	CHL2MPRules();
	virtual ~CHL2MPRules();

	virtual void Precache( void );
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );

#ifdef MAPBASE
	virtual void LevelInitPostEntity( void );
#endif

	virtual float FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon );
	virtual float FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon );
	virtual int WeaponShouldRespawn( CBaseCombatWeapon *pWeapon );
	virtual void Think( void );
	virtual void CreateStandardEntities( void );
	virtual void ClientSettingsChanged( CBasePlayer *pPlayer );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual void GoToIntermission( void );
	void GetDeathNoticeData( CBaseEntity *pVictim, const CTakeDamageInfo &info, const char **killer_weapon_name, int &killer_ID );
#ifdef MAPBASE
	virtual void DeathNotice( CBaseCombatCharacter *pVictim, const CTakeDamageInfo &info ); // Supports NPCs
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info ) { DeathNotice( pVictim->MyCombatCharacterPointer(), info ); }
#else
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
#endif
	virtual const char *GetGameDescription( void );
	// derive this function if you mod uses encrypted weapon info files
	virtual const unsigned char *GetEncryptionKey( void ) { return (unsigned char *)"x9Ke0BY7"; }
	virtual const CViewVectors* GetViewVectors() const;
	const HL2MPViewVectors* GetHL2MPViewVectors() const;

	float GetMapRemainingTime();
	void CleanUpMap();
	void CheckRestartGame();
	void RestartGame();

	void OnNavMeshLoad( void );
	
#ifndef CLIENT_DLL
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual QAngle VecItemRespawnAngles( CItem *pItem );
	virtual float	FlItemRespawnTime( CItem *pItem );
	virtual bool	CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem );
	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );
#ifdef RTBMP
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer ) OVERRIDE;
#endif

	void	AddLevelDesignerPlacedObject( CBaseEntity *pEntity );
	void	RemoveLevelDesignerPlacedObject( CBaseEntity *pEntity );
	void	ManageObjectRelocation( void );
	void    CheckChatForReadySignal( CHL2MP_Player *pPlayer, const char *chatmsg );
	const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );
 
#ifdef MAPBASE
	void NPCKilled( CAI_BaseNPC *pVictim, const CTakeDamageInfo &info );

	void PlayerSpawn( CBasePlayer *pPlayer );
	void PlayerIdle( CBasePlayer *pPlayer );

	bool	AllowDefaultItems();
	void	SetAllowDefaultItems( bool toggle );

	bool	AllowDefaultSuit();
	void	SetAllowDefaultSuit( bool toggle );
#endif

#endif

	bool IsOfficialMap( void );

	virtual void ClientDisconnected( edict_t *pClient );

	bool CheckGameOver( void );
	bool IsIntermission( void );

	void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );

	
	bool	IsTeamplay( void ) { return m_bTeamPlayEnabled;	}
#if defined(MAPBASE) && defined(GAME_DLL)
	void	TeamPlayerCountChanged( CTeam *pTeam );
	void	PlayerCountChanged( void );
#endif
#ifdef BASIC_HL2MP_COOP
	bool	IsDeathmatch( void ) { return !m_bCoOpEnabled; }
	bool	IsCoOp( void ) { return m_bCoOpEnabled; }
	void	SetCoOp( bool bEnable ) { m_bCoOpEnabled = bEnable; }
#ifndef CLIENT_DLL
	bool	CoOpItemShouldRespawn( void );
	bool	CoOpWeaponShouldRespawn( void );
	void	SetCoOpPickupRespawn( bool bEnable ) { m_bCoOpPickupRespawn = bEnable; }
#ifdef RTBMP
	bool	AnyPlayerAlive( void );
	void	RespawnAllPlayers( void );
	void	SetPlayerReady( int index, bool setting );
	void	ResetPlayerReadyVotes();
	bool	IsPlayerReady( CBasePlayer *pPlayer );
	bool	CanPlayerVoteReady( CBasePlayer *pPlayer );
	void	SetAcceptingReadyVote( bool setting );
	bool	IsAcceptingReadyVote( void ) { return m_bAcceptingReadyVote; }
	void	SetForcedPlayerModelTeam( int iTeam );
	int		GetForcedPlayerModelTeam( void );
#endif
#endif
#endif
	void	CheckAllPlayersReady( void );

	virtual bool IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer );

#ifndef HL2MP_USES_HL2_GAMERULES
	bool	MegaPhyscannonActive( void ) { return false; }
#endif
	
private:
	
	CNetworkVar( bool, m_bTeamPlayEnabled );
#ifdef BASIC_HL2MP_COOP
	CNetworkVar( bool, m_bCoOpEnabled );
	bool m_bCoOpPickupRespawn;
#ifdef RTBMP
	bool m_bPlayersReady[MAX_PLAYERS_ARRAY_SAFE];
	bool m_bAcceptingReadyVote;
	int m_iForcedPlayerModelTeam;
#endif
#endif
	CNetworkVar( float, m_flGameStartTime );
	CUtlVector<EHANDLE> m_hRespawnableItemsAndWeapons;
	float m_tmNextPeriodicThink;
	float m_flRestartGameTime;
	bool m_bCompleteReset;
	bool m_bAwaitingReadyRestart;
	bool m_bHeardAllPlayersReady;

#ifndef CLIENT_DLL
	bool m_bChangelevelDone;

#ifdef MAPBASE
	bool m_bAllowDefaultItems = true;
	bool m_bAllowDefaultSuit = true;

	CNetworkVar( bool, m_bHideBotJoinGame );	// Hides when bots connect and disconnect from the game
#endif
#endif

#ifdef MAPBASE
	CHandle<CHL2MPGameRulesProxy> m_hGamerulesProxy;
#endif
};

inline CHL2MPRules* HL2MPRules()
{
	return static_cast<CHL2MPRules*>(g_pGameRules);
}

#endif //HL2MP_GAMERULES_H
