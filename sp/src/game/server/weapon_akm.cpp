//========= Copyright (c) RTBR Team, 2018 ============//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "npcevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "in_buttons.h"
#include "ai_memory.h"
#include "soundent.h"
#include "rumble_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



class CWeaponAKM : public CHLSelectFireMachineGun {
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponAKM, CHLSelectFireMachineGun);
	DECLARE_SERVERCLASS();
	CWeaponAKM();

	void Precache(void);

	void ItemPostFrame(void);

	bool Holster(CBaseCombatWeapon * pSwitchingTo);

	virtual void SecondaryAttack(void);

	virtual void PrimaryAttack(void);

	void	AddViewKick( void );

	float	GetFireRate( void );

	bool	m_bLowFire;

	float	m_flLastShot;

	int m_iShotsFired;

	virtual const Vector& GetBulletSpread(void) {
		static const Vector cone = VECTOR_CONE_3DEGREES;
		return cone;
	}

	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponAKM, DT_WeaponAKM)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_akm, CWeaponAKM);
PRECACHE_WEAPON_REGISTER(weapon_akm);

BEGIN_DATADESC(CWeaponAKM)
DEFINE_FIELD(m_bLowFire, FIELD_BOOLEAN),
DEFINE_FIELD(m_flLastShot, FIELD_FLOAT),
DEFINE_FIELD(m_iShotsFired, FIELD_INTEGER),
END_DATADESC()
acttable_t	CWeaponAKM::m_acttable[] = {
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SMG1, true },
	{ ACT_RELOAD, ACT_RELOAD_SMG1, true },
	{ ACT_IDLE, ACT_IDLE_SMG1, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SMG1, true },

	{ ACT_WALK, ACT_WALK_RIFLE, true },
	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },

	{ ACT_IDLE_RELAXED, ACT_IDLE_SMG1_RELAXED, false },
	{ ACT_IDLE_STIMULATED, ACT_IDLE_SMG1_STIMULATED, false },
	{ ACT_IDLE_AGITATED, ACT_IDLE_ANGRY_SMG1, false },

	{ ACT_WALK_RELAXED, ACT_WALK_RIFLE_RELAXED, false },
	{ ACT_WALK_STIMULATED, ACT_WALK_RIFLE_STIMULATED, false },
	{ ACT_WALK_AGITATED, ACT_WALK_AIM_RIFLE, false },

	{ ACT_RUN_RELAXED, ACT_RUN_RIFLE_RELAXED, false },
	{ ACT_RUN_STIMULATED, ACT_RUN_RIFLE_STIMULATED, false },
	{ ACT_RUN_AGITATED, ACT_RUN_AIM_RIFLE, false },

	{ ACT_IDLE_AIM_RELAXED, ACT_IDLE_SMG1_RELAXED, false },
	{ ACT_IDLE_AIM_STIMULATED, ACT_IDLE_AIM_RIFLE_STIMULATED, false },
	{ ACT_IDLE_AIM_AGITATED, ACT_IDLE_ANGRY_SMG1, false },

	{ ACT_WALK_AIM_RELAXED, ACT_WALK_RIFLE_RELAXED, false },
	{ ACT_WALK_AIM_STIMULATED, ACT_WALK_AIM_RIFLE_STIMULATED, false },
	{ ACT_WALK_AIM_AGITATED, ACT_WALK_AIM_RIFLE, false },

	{ ACT_RUN_AIM_RELAXED, ACT_RUN_RIFLE_RELAXED, false },
	{ ACT_RUN_AIM_STIMULATED, ACT_RUN_AIM_RIFLE_STIMULATED, false },
	{ ACT_RUN_AIM_AGITATED, ACT_RUN_AIM_RIFLE, false },

	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_RIFLE, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_SMG1, true },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SMG1_LOW, true },
	{ ACT_COVER_LOW, ACT_COVER_SMG1_LOW, false },
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_SMG1_LOW, false },
	{ ACT_RELOAD_LOW, ACT_RELOAD_SMG1_LOW, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_SMG1, true },
};

IMPLEMENT_ACTTABLE(CWeaponAKM);

void CWeaponAKM::Precache(void)
{
	PrecacheModel("models/weapons/akm/m_akm.mdl");
	BaseClass::Precache();
}
void CWeaponAKM::ItemPostFrame(void)
{

	if (m_flLastShot < gpGlobals->curtime - (GetFireRate() * 1.1)) {
		m_iShotsFired = 0;
	}

	if (m_iShotsFired >= 5) {
		m_bLowFire = true;
	}
	else {
		m_bLowFire = false;
	}

	BaseClass::ItemPostFrame();
}
bool CWeaponAKM::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

CWeaponAKM::CWeaponAKM() {
	m_fMinRange1 = 0; m_fMaxRange1 = 2000;
	m_bAltFiresUnderwater = m_bFiresUnderwater = false;
}

float CWeaponAKM::GetFireRate( void )
{
	if (m_bLowFire) {
		return (GetRTBRWpnData().m_flFireRate * 1.25);
	}
	else {
		return GetRTBRWpnData().m_flFireRate;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAKM::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	2.0f	//Degrees
	#define	SLIDE_LIMIT			4.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

void CWeaponAKM::PrimaryAttack(void) {
	m_flLastShot = gpGlobals->curtime;
	m_iShotsFired++;

	BaseClass::PrimaryAttack();
}

void CWeaponAKM::SecondaryAttack(void) {
	return;
}