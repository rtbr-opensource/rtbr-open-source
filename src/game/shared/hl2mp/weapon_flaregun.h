//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "soundenvelope.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
	#define CFlaregun C_Flaregun
#endif

#ifndef WEAPON_FLAREGUN_H
#define WEAPON_FLAREGUN_H
#ifdef _WIN32
#pragma once
#endif

#define	SF_FLARE_NO_DLIGHT	0x00000001
#define	SF_FLARE_NO_SMOKE	0x00000002
#define	SF_FLARE_INFINITE	0x00000004
#define	SF_FLARE_START_OFF	0x00000008

#define	FLARE_DURATION		30.0f
#define FLARE_DECAY_TIME	10.0f
#define FLARE_BLIND_TIME	6.0f

//---------------------
// Flare
//---------------------

class CFlare : public CBaseCombatCharacter
{
public:
	DECLARE_CLASS( CFlare, CBaseCombatCharacter );
#ifndef CLIENT_DLL
	CFlare();
	~CFlare();

	static CFlare *	GetActiveFlares( void );
	CFlare *		GetNextFlare( void ) const { return m_pNextFlare; }

	static CFlare *Create( Vector vecOrigin, QAngle vecAngles, CBaseEntity *pOwner, float lifetime );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	void	Spawn( void );
	void	Precache( void );
	int		Restore( IRestore &restore );
	void	Activate( void );

	void	StartBurnSound( void );

	void	Start( float lifeTime );
	void	Die( float fadeTime );
	void	Launch( const Vector &direction, float speed );

	Class_T Classify( void );

	void	FlareTouch( CBaseEntity *pOther );
	void	FlareBurnTouch( CBaseEntity *pOther );
	void	FlareThink( void );

	void	InputStart( inputdata_t &inputdata );
	void	InputDie( inputdata_t &inputdata );
	void	InputLaunch( inputdata_t &inputdata );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	static CFlare *activeFlares;

	CBaseEntity *m_pOwner;
	int			m_nBounces;			// how many times has this flare bounced?
	CNetworkVar( float, m_flTimeBurnOut );	// when will the flare burn out?
	CNetworkVar( float, m_flScale );
	float		m_flDuration;
	float		m_flNextDamage;
	
	CSoundPatch	*m_pBurnSound;
	bool		m_bFading;
	CNetworkVar( bool, m_bLight );
	CNetworkVar( bool, m_bSmoke );
	CNetworkVar( bool, m_bPropFlare );

	bool		m_bInActiveList;
	CFlare *	m_pNextFlare;

	void		RemoveFromActiveFlares( void );
	void		AddToActiveFlares( void );
#endif
};

#ifdef MAPBASE
extern acttable_t *GetPistolActtable();
extern int GetPistolActtableCount();
#endif

//---------------------
// Flaregun
//---------------------
class CFlaregun:public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CFlaregun, CBaseHL2MPCombatWeapon );

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CFlaregun();

	void Precache( void );
	void PrimaryAttack( void );

	virtual float GetWeaponAutoReloadTime( void ) OVERRIDE { return 1.5f; }

	DECLARE_ACTTABLE();

#ifdef MAPBASE
	// Pistols are their own backup activities
	virtual acttable_t		*GetBackupActivityList() { return GetPistolActtable(); }
	virtual int				GetBackupActivityListCount() { return GetPistolActtableCount(); }
#endif

private:
	CFlaregun( const CFlaregun & );
};


#endif // WEAPON_FLAREGUN_H

