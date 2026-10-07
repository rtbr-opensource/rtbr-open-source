//========= Copyright (c) RTBR Team, 2018 ============//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "rumble_shared.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else
	#include "hl2mp_player.h"
	#include "basecombatcharacter.h"
	#include "ai_basenpc.h"
	#include "ai_memory.h"
	#include "soundent.h"
#endif

#include "weapon_hl2mpbase.h"
#include "weapon_hl2mpbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponAKM C_WeaponAKM
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef MAPBASE
extern acttable_t *GetAR2Acttable();
extern int GetAR2ActtableCount();
#endif

class CWeaponAKM : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS(CWeaponAKM, CHL2MPMachineGun);

	CWeaponAKM();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void Precache(void);

	void ItemPostFrame(void);

	bool Holster(CBaseCombatWeapon * pSwitchingTo);

	virtual void SecondaryAttack(void);

	virtual void PrimaryAttack(void);

	void	AddViewKick( void );

	float	GetFireRate( void );

	CNetworkVar(bool, m_bLowFire);

	CNetworkVar(float, m_flLastShot);

	CNetworkVar(int, m_iShotsFired);

	virtual const Vector& GetBulletSpread(void) {
		static const Vector cone = VECTOR_CONE_3DEGREES;
		return cone;
	}

#ifdef MAPBASE
	virtual acttable_t *GetBackupActivityList() { return GetAR2Acttable(); }
	virtual int			GetBackupActivityListCount() { return GetAR2ActtableCount(); }
#endif

	DECLARE_ACTTABLE();

private:
	CWeaponAKM( const CWeaponAKM & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAKM, DT_WeaponAKM )

BEGIN_NETWORK_TABLE( CWeaponAKM, DT_WeaponAKM )
#ifdef CLIENT_DLL
RecvPropBool( RECVINFO( m_bLowFire ) ),
RecvPropFloat( RECVINFO( m_flLastShot ) ),
RecvPropInt( RECVINFO( m_iShotsFired ) ),
#else
SendPropBool( SENDINFO( m_bLowFire ) ),
SendPropFloat( SENDINFO( m_flLastShot ) ),
SendPropInt( SENDINFO( m_iShotsFired ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAKM )
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD( m_bLowFire, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_flLastShot, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_iShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_akm, CWeaponAKM);
PRECACHE_WEAPON_REGISTER(weapon_akm);

acttable_t	CWeaponAKM::m_acttable[] = {
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_AR1, true },
	
#if EXPANDED_HL2_UNUSED_WEAPON_ACTIVITIES
	// Optional new NPC activities
	// (these should fall back to AR2 animations when they don't exist on an NPC)
	{ ACT_RELOAD,					ACT_RELOAD_AR1,			true },
	{ ACT_IDLE,						ACT_IDLE_AR1,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_AR1,		true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_AR1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_AR1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_AR1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_AR1_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_AR1_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_AR1,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_AR1_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_AR1_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_AR1,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_AR1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_AR1_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_AR1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_AR1_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_AR1_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_AR1,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_AR1_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_AR1_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_AR1,				false },//always aims
//End readiness activities

	{ ACT_WALK,						ACT_WALK_AR1,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_AR1,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,					true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,				true },
	{ ACT_RUN,						ACT_RUN_AR1,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_AR1,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,					true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,				true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR1,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_AR1_LOW,		true },
	{ ACT_COVER_LOW,				ACT_COVER_AR1_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR1_LOW,			false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_AR1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_AR1,		true },

	{ ACT_ARM,						ACT_ARM_RIFLE,					false },
	{ ACT_DISARM,					ACT_DISARM_RIFLE,				false },

#if EXPANDED_HL2_COVER_ACTIVITIES
	{ ACT_RANGE_AIM_MED,			ACT_RANGE_AIM_AR1_MED,			false },
	{ ACT_RANGE_ATTACK1_MED,		ACT_RANGE_ATTACK_AR1_MED,		false },
#endif

#if EXPANDED_HL2DM_ACTIVITIES
	// HL2:DM activities (for third-person animations in SP)
	{ ACT_HL2MP_IDLE,                    ACT_HL2MP_IDLE_AR1,                    false },
	{ ACT_HL2MP_RUN,                    ACT_HL2MP_RUN_AR1,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,            ACT_HL2MP_IDLE_CROUCH_AR1,            false },
	{ ACT_HL2MP_WALK_CROUCH,            ACT_HL2MP_WALK_CROUCH_AR1,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,    ACT_HL2MP_GESTURE_RANGE_ATTACK_AR1,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,            ACT_HL2MP_GESTURE_RELOAD_AR1,        false },
	{ ACT_HL2MP_JUMP,                    ACT_HL2MP_JUMP_AR1,                    false },
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_AR1,					false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_AR1,    false },
#endif
#endif
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
		return 0.125f;
	}
	else {
		return 0.1f;
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