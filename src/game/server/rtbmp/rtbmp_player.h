//========= Copyright RTBMP Team, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef RTBMP_PLAYER_H
#define RTBMP_PLAYER_H
#pragma once

#include "hl2mp_player.h"
#include "capturetheflag.h"

class CRTBMP_Player : public CHL2MP_Player
{
public:
	DECLARE_CLASS( CRTBMP_Player, CHL2MP_Player );

	CRTBMP_Player();

	static CRTBMP_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CRTBMP_Player::s_PlayerEdict = ed;
		return (CRTBMP_Player*)CreateEntityByName( className );
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC();

	virtual void Precache( void );
	virtual void InitialSpawn( void );
	virtual void Spawn( void );
	virtual void PostThink( void );
	virtual bool ClientCommand( const CCommand &args );
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	void		RemoveAfterburn( void );
	virtual void SetIgniteBegin( CBaseCombatCharacter *pAttacker );
	void		TakeIgniteDamage( void );
	CBaseCombatCharacter *GetAfterburnAttacker( void ) const { return m_hAfterburnAttacker; }
	virtual int OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual void FireBullets( const FireBulletsInfo_t &info );
	virtual void ChangeTeam( int iTeam ) OVERRIDE;

	// movement
	virtual void HandleSpeedChanges( CMoveData *mv ) OVERRIDE;
	virtual void ReduceTimers( CMoveData *mv ) OVERRIDE;
	virtual bool CanAlwaysRun( void );

	void DisableSpawnProtection( void );
	bool IsSpawnProtected( void ) { return gpGlobals->curtime <= m_flSpawnProtectTime; }

	void StartXenHealExpireTimer( void );
	void XenHealTimerExpired( void );

	// To-do: Allow players to boogie after we fix their brokenness with deathcams
	virtual bool CanBoogie( void ) { return false; }

	virtual unsigned int	PlayerSolidMask( bool brushOnly = false ) const;
	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const;

	bool HasForcedModel( void ) { return m_bHasForcedModel; }
	void SetForcedModel( bool bForce ) { m_bHasForcedModel = bForce; }
	int GetDesiredPlayerModelTeam( const char *pszModel );
	bool CanSwitchToModel( const char *pszModel );

	virtual void SetPlayerModel( void );
	virtual void SetPlayerTeamModel( void );

	virtual bool HandleCommand_JoinTeam( int team );

	// flag methods
	void PickupFlag( CCaptureFlag *flag );
	void DropFlag( void );
	void ReturnFlag( void );
	CCaptureFlag *GetFlag() { return m_hFlag; }

private:
	CNetworkVar( float, m_flSpawnProtectTime );
	CCaptureFlag *m_hFlag;

	float			m_flIgniteBegin;
	float			m_flLastIgniteTick;
	CBaseCombatCharacter	*m_hAfterburnAttacker;
	float			m_flXenHealExpireTimer;
	bool			m_bBeingXenHealed;
	bool			m_bHasForcedModel;
};

inline CRTBMP_Player *ToRTBMPPlayer( CBaseEntity *pEntity )
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;

	return dynamic_cast<CRTBMP_Player*>(pEntity);
}

#endif //RTBMP_PLAYER_H
