//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef WEAPON_BASEHL2MPCOMBATWEAPON_SHARED_H
#define WEAPON_BASEHL2MPCOMBATWEAPON_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else
	#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbase.h"

#if defined( CLIENT_DLL )
#define CBaseHL2MPCombatWeapon C_BaseHL2MPCombatWeapon
#endif

#ifdef MAPBASE
	// Alias for migrated HL2 weapons
	#undef CBaseHLCombatWeapon
	#define CBaseHLCombatWeapon CBaseHL2MPCombatWeapon
#endif

class CBaseHL2MPCombatWeapon : public CWeaponHL2MPBase
{
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	DECLARE_CLASS( CBaseHL2MPCombatWeapon, CWeaponHL2MPBase );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CBaseHL2MPCombatWeapon();

	virtual bool	DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt ) OVERRIDE;
	virtual bool	DefaultReload( int iClipSize1, int iClipSize2, int iActivity ) OVERRIDE;
	virtual void	FinishReload() OVERRIDE;
	virtual void	ItemBusyFrame( void ) OVERRIDE;

	virtual bool	WeaponShouldBeLowered( void );

	virtual bool	Ready( void );
	virtual bool	CanLower( void );
	virtual bool	Lower( void );
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	WeaponIdle( void );

	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );

	virtual Vector	GetBulletSpread( WeaponProficiency_t proficiency );
	virtual float	GetSpreadBias( WeaponProficiency_t proficiency );

	virtual const	WeaponProficiencyInfo_t *GetProficiencyValues();
	static const	WeaponProficiencyInfo_t *GetDefaultProficiencyValues();

	virtual void	ItemHolsterFrame( void );
#ifdef RTBMP
	// Override this to set a custom backpack reload delay for your weapon
	virtual float			GetWeaponAutoReloadTime( void ) { return -1.f; }
#endif
	
	CNetworkVar( float, m_flReloadEndTime );
	CNetworkVar( bool, m_bReloadFinished );

	virtual void	SetDrawFramesTime();
	virtual float	GetAnimEventTime( int iAnimEvent );

	// used by bots to determine how long to charge an attack
	virtual const float	MaxChargeTime() const	{ return 0.0f; }
	virtual const float	MaxChargeDamage() const { return 0.0f; }

protected:

	bool			m_bLowered;			// Whether the viewmodel is raised or lowered
	float			m_flRaiseTime;		// If lowered, the time we should raise the viewmodel
	float			m_flHolsterTime;	// When the weapon was holstered

private:
	
	CBaseHL2MPCombatWeapon( const CBaseHL2MPCombatWeapon & );
};

#endif // WEAPON_BASEHL2MPCOMBATWEAPON_SHARED_H
