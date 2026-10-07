//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "weapon_hl2mpbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifndef BASEHLCOMBATWEAPON_H
#define BASEHLCOMBATWEAPON_H
#ifdef _WIN32
#pragma once
#endif

#if defined( CLIENT_DLL )
	#define CHL2MPMachineGun C_HL2MPMachineGun
#endif

//=========================================================
// Machine gun base class
//=========================================================
class CHL2MPMachineGun : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CHL2MPMachineGun, CBaseHL2MPCombatWeapon );
	DECLARE_DATADESC();

	CHL2MPMachineGun();
	
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	PrimaryAttack( WeaponSound_t weaponSound = SINGLE );

	// Default calls through to m_hOwner, but plasma weapons can override and shoot projectiles here.
	virtual void	ItemPostFrame( void );
	virtual void	FireBullets( const FireBulletsInfo_t &info );
	virtual bool	Deploy( void );

	virtual const Vector &GetBulletSpread( void );

	int				WeaponSoundRealtime( WeaponSound_t shoot_type );

	// utility function
	static void DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime );

#if defined(MAPBASE_MP) && !defined(CLIENT_DLL)
	virtual int		WeaponRangeAttack1Condition( float flDot, float flDist );
#endif

	Activity GetPrimaryAttackActivity( void );

	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );

private:
	
	CHL2MPMachineGun( const CHL2MPMachineGun & );

protected:

	int	m_nShotsFired;	// Number of consecutive shots fired

	float	m_flNextSoundTime;	// real-time clock of when to make next sound
};

#endif // BASEHLCOMBATWEAPON_H
