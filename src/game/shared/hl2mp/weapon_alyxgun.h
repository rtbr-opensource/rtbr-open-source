//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef WEAPON_ALYXGUN_H
#define WEAPON_ALYXGUN_H

// If we don't include weapon_hl2mpbasehlmpcombatweapon.h then the code fails to compile,
// for whatever dumb reason it conjured up this time.
// Including weapon_hl2mpbase.h would also fix this problem.
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponAlyxGun C_WeaponAlyxGun
#endif

#if defined( _WIN32 )
#pragma once
#endif

#ifdef MAPBASE
extern acttable_t *GetPistolActtable();
extern int GetPistolActtableCount();
#endif

class CWeaponAlyxGun : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS( CWeaponAlyxGun, CHL2MPMachineGun );

	CWeaponAlyxGun();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// We need to call the base class directly, otherwise the recoil animation code doesn't work properly
	virtual void PrimaryAttack(void) { BaseClass::PrimaryAttack(); }

	virtual int		GetMinBurst(void) { return 4; }
	virtual int		GetMaxBurst(void) { return 7; }
	virtual float	GetMinRestTime(void);
	virtual float	GetMaxRestTime(void);

	float	GetFireRate(void) { return 0.077f; };

	void	AddViewKick(void);

	void Precache(void);

#ifndef CLIENT_DLL
	int		CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int		WeaponRangeAttack1Condition(float flDot, float flDist);
	int		WeaponRangeAttack2Condition(float flDot, float flDist);
#endif

	virtual const Vector& GetBulletSpread(void);

#ifndef CLIENT_DLL
	void FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles);
	void Operator_ForceNPCFire(CBaseCombatCharacter  *pOperator, bool bSecondary);
	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
#endif
	//virtual void SetPickupTouch( void )
	//{
	//	// Alyx gun cannot be picked up
	//	SetTouch(NULL);
	//}

#ifdef MAPBASE
	virtual acttable_t		*GetBackupActivityList() { return GetPistolActtable(); }
	virtual int				GetBackupActivityListCount() { return GetPistolActtableCount(); }
#endif

	float m_flTooCloseTimer;

	DECLARE_ACTTABLE();

private:
	CWeaponAlyxGun( const CWeaponAlyxGun & );
};

#endif // WEAPON_ALYXGUN_H
