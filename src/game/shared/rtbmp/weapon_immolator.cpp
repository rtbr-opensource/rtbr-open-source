//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "in_buttons.h"
#include "particle_parse.h"
#include "decals.h"
#include "beam_shared.h"
#include "view_shared.h"
#ifndef CLIENT_DLL
	#include "basecombatcharacter.h"
	#include "rtbmp_player.h"
	#include "soundent.h"
	#include "te_particlesystem.h"
	#include "ndebugoverlay.h"
	#include "ai_basenpc.h"
	#include "ai_memory.h"
	#include "props.h"
	#include "baseentity.h"
#else
	#include "view.h"
	#include "iviewrender.h"
	#include "c_weapon__stubs.h"
	#include "c_rtbmp_player.h"
#endif



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponImmolator C_WeaponImmolator
#endif

#define MAX_BURN_RADIUS		256
#define RADIUS_GROW_RATE	50.0	// units/sec 
#define IMMOLATOR_BURN_TIME 3.0
#define IMMOLATOR_PLASMABALL_DMG 5.0 // dumb hardcode!!

#define IMMOLATOR_TARGET_INVALID Vector( FLT_MAX, FLT_MAX, FLT_MAX )

ConVar sk_immolator_rtbr_plasma_stream("sk_immolator_rtbr_plasma_stream", "1", FCVAR_REPLICATED);

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------
class CImmolatorPlasmaBall : public CBaseCombatCharacter
{
	DECLARE_CLASS(CImmolatorPlasmaBall, CBaseCombatCharacter);

public:
	Class_T Classify(void) { return CLASS_NONE; }

public:
	void Spawn(void);
	void Precache(void);
	void BoltTouch(CBaseEntity *pOther);
	void	IgniteOtherIfAllowed(CBaseEntity *pOther);
	bool CreateVPhysics(void);
	unsigned int PhysicsSolidMaskForEntity() const;

protected:

	DECLARE_DATADESC();
};
LINK_ENTITY_TO_CLASS(immolator_plasma_ball, CImmolatorPlasmaBall);

BEGIN_DATADESC(CImmolatorPlasmaBall)
// Function Pointers,
DEFINE_FUNCTION(BoltTouch),

END_DATADESC()
#endif

#ifdef MAPBASE
extern acttable_t *GetAR2Acttable();
extern int GetAR2ActtableCount();
#endif

class CWeaponImmolator : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponImmolator, CBaseHL2MPCombatWeapon );

	CWeaponImmolator(void);

	void Precache(void);
	void PrimaryAttack(void);
	void ItemPostFrame(void);
	bool Holster(CBaseCombatWeapon * pSwitchingTo);
	
#ifndef CLIENT_DLL
	virtual bool WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions);
	virtual int	WeaponRangeAttack1Condition(float flDot, float flDist);
	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	void Update();
	void UpdateThink();
	void ImmolationDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore );
	bool ImmolateEntity( const CTakeDamageInfo &info, CBaseCombatCharacter *pBCC, int iClassIgnore ); // 1upD - Do damage to one entity in particular
#endif
	void	OnPickedUp(CBaseCombatCharacter *pNewOwner);

	void StartImmolating();
	void StopImmolating();
	void StopImmolatingSilent();
	bool IsImmolating() { return m_flBurnRadius != 0.0; }

	Vector GetMuzzleVector();

#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

	int	m_beamIndex;

	float m_flBurnRadius;
	float m_flTimeLastUpdatedRadius;

	Vector  m_vecImmolatorTarget;

	int m_iUpdateCounter;

	CNetworkVar( bool, m_bImmolating );
	CNetworkVar( Vector, m_vPlayerMuzzleVector );
	CNetworkVar( Vector, m_vAiming );

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef CLIENT_DLL
	virtual void OnDataChanged( DataUpdateType_t updateType );
	CNewParticleEffect *m_hImmolatorFlame;
#endif

#ifdef MAPBASE
	virtual acttable_t		*GetBackupActivityList() { return GetAR2Acttable(); }
	virtual int				GetBackupActivityListCount() { return GetAR2ActtableCount(); }
#endif

	DECLARE_ACTTABLE();
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponImmolator, DT_WeaponImmolator )

BEGIN_NETWORK_TABLE( CWeaponImmolator, DT_WeaponImmolator )
#ifdef CLIENT_DLL
RecvPropBool( RECVINFO( m_bImmolating ) ),
RecvPropVector( RECVINFO( m_vPlayerMuzzleVector ) ),
RecvPropVector( RECVINFO( m_vAiming ) ),
#else
SendPropBool( SENDINFO( m_bImmolating ) ),
SendPropVector( SENDINFO( m_vPlayerMuzzleVector ) ),
SendPropVector( SENDINFO( m_vAiming ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponImmolator )
DEFINE_PRED_FIELD( m_bImmolating, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_vPlayerMuzzleVector, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_vAiming, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS(info_target_immolator, CPointEntity);
#endif
LINK_ENTITY_TO_CLASS(weapon_immolator, CWeaponImmolator);
PRECACHE_WEAPON_REGISTER(weapon_immolator);

acttable_t	CWeaponImmolator::m_acttable[] =
{
#if AR2_ACTIVITY_FIX == 1
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_AR2,				true },
	{ ACT_IDLE,						ACT_IDLE_AR2,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_AR2,			false },

	{ ACT_WALK,						ACT_WALK_AR2,					true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_AR2_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_AR2_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_AR2,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_AR2_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_AR2_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_AR2,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_AR2_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_AR2_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_AR2_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_AR2_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_AR2,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_AR2_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_AR2_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_AR2,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_AR2_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_AR2_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_AR2,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_AR2,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_AR2,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_COVER_LOW,				ACT_COVER_AR2_LOW,				true },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_AR2_LOW,		false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_AR2_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_AR2,		true },
//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },
#else
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },		// FIXME: hook to AR2 unique

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },		// FIXME: hook to AR2 unique
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },		// FIXME: hook to AR2 unique
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },
#endif

#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_ARM,						ACT_ARM_RIFLE,					false },
	{ ACT_DISARM,					ACT_DISARM_RIFLE,				false },
#endif

#if EXPANDED_HL2_COVER_ACTIVITIES
	{ ACT_RANGE_AIM_MED,			ACT_RANGE_AIM_AR2_MED,			false },
	{ ACT_RANGE_ATTACK1_MED,		ACT_RANGE_ATTACK_AR2_MED,		false },

	{ ACT_COVER_WALL_R,				ACT_COVER_WALL_R_RIFLE,			false },
	{ ACT_COVER_WALL_L,				ACT_COVER_WALL_L_RIFLE,			false },
	{ ACT_COVER_WALL_LOW_R,			ACT_COVER_WALL_LOW_R_RIFLE,		false },
	{ ACT_COVER_WALL_LOW_L,			ACT_COVER_WALL_LOW_L_RIFLE,		false },
#endif

#ifdef MAPBASE
	// HL2:DM activities (for third-person animations in SP)
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_AR2,                    false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_AR2,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_AR2,            false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_AR2,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_AR2,        false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_AR2,                    false },
#if EXPANDED_HL2DM_ACTIVITIES
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_AR2,						false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_AR2,	false },
#endif
#endif
};

IMPLEMENT_ACTTABLE( CWeaponImmolator );

#ifndef CLIENT_DLL
BEGIN_DATADESC(CWeaponImmolator)

DEFINE_FIELD(m_beamIndex, FIELD_INTEGER),
DEFINE_FIELD(m_flBurnRadius, FIELD_FLOAT),
DEFINE_FIELD(m_flTimeLastUpdatedRadius, FIELD_TIME),
DEFINE_FIELD(m_vecImmolatorTarget, FIELD_VECTOR),
DEFINE_FIELD(m_bImmolating, FIELD_BOOLEAN),
DEFINE_FIELD(m_vPlayerMuzzleVector, FIELD_VECTOR),
DEFINE_FIELD( m_vAiming, FIELD_VECTOR ),
DEFINE_FIELD( m_iUpdateCounter, FIELD_INTEGER),

DEFINE_ENTITYFUNC(UpdateThink),
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponImmolator::CWeaponImmolator(void)
{
	m_fMaxRange1 = 4096;
	StopImmolating();
}

void CWeaponImmolator::StartImmolating()
{
	//CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	// Start the radius really tiny because we use radius == 0.0 to 
	// determine whether the immolator is operating or not.
	m_flBurnRadius = 0.1;
	m_flTimeLastUpdatedRadius = gpGlobals->curtime;
#ifndef CLIENT_DLL
	SetThink(&CWeaponImmolator::UpdateThink);
#endif
	SetNextThink(gpGlobals->curtime);

	WeaponSound(SINGLE);
	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		// player "shoot" animation
		pOwner->SetAnimation( PLAYER_ATTACK1 );
	}

	DispatchParticleEffect( "weapon_immolator_muzzle", PATTACH_POINT_FOLLOW, ToBasePlayer( GetOwner() )->GetViewModel(), "muzzle", true );
	
#ifndef CLIENT_DLL
	CSoundEnt::InsertSound(SOUND_DANGER, m_vecImmolatorTarget, 256, 5.0, GetOwner());
#endif

	m_bImmolating = true;
}

void CWeaponImmolator::StopImmolating()
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	m_flBurnRadius = 0.0;
	SetThink(NULL);
	m_vecImmolatorTarget = IMMOLATOR_TARGET_INVALID;
	if (pOwner && pOwner->GetViewModel()) {
		StopParticleEffects(pOwner->GetViewModel());
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.1; // 1upD - Use 5 second delay only if maximum burn radius was achieved
	WeaponSound(WPN_DOUBLE);
	SendWeaponAnim(ACT_VM_SECONDARYATTACK);

	m_bImmolating = false;
}

void CWeaponImmolator::OnPickedUp(CBaseCombatCharacter *pNewOwner)
{
	BaseClass::OnPickedUp(pNewOwner);
	//SendWeaponAnim(ACT_VM_FIRSTDRAW);
}

void CWeaponImmolator::StopImmolatingSilent()
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	m_flBurnRadius = 0.0;
	SetThink(NULL);
	m_vecImmolatorTarget = IMMOLATOR_TARGET_INVALID;
	if (pOwner && pOwner->GetViewModel()) {
		StopParticleEffects(pOwner->GetViewModel());
	}
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.1; // 1upD - Use 5 second delay only if maximum burn radius was achieved
	m_bImmolating = false;
}

bool CWeaponImmolator::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	StopImmolatingSilent();
	StopWeaponSound(SINGLE);
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::Precache(void)
{
	PrecacheParticleSystem("weapon_immolator_muzzle");
	m_beamIndex = PrecacheModel("sprites/bluelaser1.vmt");
#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "immolator_plasma_ball" );
#endif
#ifdef CLIENT_DLL
	PrecacheParticleSystem( "weapon_immolator_flame" );
#endif

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::PrimaryAttack(void)
{
	if (!IsImmolating())
	{
		StartImmolating();
#ifndef CLIENT_DLL
	CRTBMP_Player *pMPPlayer = ToRTBMPPlayer( GetOwner() );
	if (pMPPlayer)
		pMPPlayer->DisableSpawnProtection();
#endif
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// This weapon is said to have Line of Sight when it CAN'T see the target, but
// can see a place near the target than can.
//-----------------------------------------------------------------------------
bool CWeaponImmolator::WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions)
{
	CAI_BaseNPC* npcOwner = GetOwner()->MyNPCPointer();

	if (!npcOwner)
	{
		return false;
	}

	if (IsImmolating())
	{
		// Don't update while Immolating. This is a committed attack.
		return false;
	}

	// Assume we won't find a target.
	m_vecImmolatorTarget = targetPos;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Weapon firing conditions
//-----------------------------------------------------------------------------
int CWeaponImmolator::WeaponRangeAttack1Condition(float flDot, float flDist)
{
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
	{
		// Too soon to attack!
		return COND_NONE;
	}

	if (IsImmolating())
	{
		// Once is enough!
		return COND_NONE;
	}

	if (m_vecImmolatorTarget == IMMOLATOR_TARGET_INVALID)
	{
		// No target!
		return COND_NONE;
	}

	if (flDist > m_fMaxRange1)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.5f)	// UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
}

void CWeaponImmolator::UpdateThink(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner && !pOwner->IsAlive())
	{
		StopImmolating();
		return;
	}

	// The flame stream particle updates at 40 ticks/sec, but we only fire 20 flames per second serverside. We double the think rate
	// and fire a flame every even think, whilst updating the particle every odd think.
	if ( m_iUpdateCounter == 0 )
	{
		Update();
		m_iUpdateCounter = 1;
	}
	else
	{	
		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
		if ( !pPlayer )
			return;

		m_iUpdateCounter = 0;
	}
	SetNextThink(gpGlobals->curtime + 0.025);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponImmolator::Update()
{
	if (sk_immolator_rtbr_plasma_stream.GetBool())
	{
		CBasePlayer *pOwner = ToBasePlayer(GetOwner());

		if (pOwner == NULL)
			return;

		Vector	vForward, vRight, vUp;

		pOwner->EyeVectors(&vForward, &vRight, &vUp);

		Vector vecAiming = pOwner->GetAutoaimVector(0);

		// send over coordinates + angles for the clientside immolator flame stream
		m_vPlayerMuzzleVector = GetMuzzleVector();

		// Create a new entity with CCrossbowBolt private data
		CImmolatorPlasmaBall *pPlasmaBall = (CImmolatorPlasmaBall *)CBaseEntity::Create("immolator_plasma_ball", m_vPlayerMuzzleVector, QAngle(0,0,0), pOwner);
		//UTIL_SetOrigin(pPlasmaBall, vecSrc);
		//pPlasmaBall->SetLocalOrigin(vecShootOrigin2);
		//pPlasmaBall->SetAbsAngles(vecAngles);
		pPlasmaBall->Spawn();
		pPlasmaBall->SetOwnerEntity(pOwner);
		pPlasmaBall->SetBaseVelocity((vecAiming * 800));
		pPlasmaBall->SetContextThink(&CImmolatorPlasmaBall::SUB_Remove, gpGlobals->curtime + 0.75, "KillBoltThink");
		pPlasmaBall->SetCollisionGroup(COLLISION_GROUP_NONE);

		GetOwner()->RemoveAmmo(1, m_iPrimaryAmmoType);

		m_vAiming = Vector(vecAiming.x, vecAiming.y, vecAiming.z);
		
		// If the gun runs out of ammo, stop firing
		if (pOwner && pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		{
			StopImmolating();
			// Can't use the immolator again for a while
			m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
		}

		return;
	}


	float flDuration = gpGlobals->curtime - m_flTimeLastUpdatedRadius;
	if (flDuration != 0.0)
	{
		m_flBurnRadius += RADIUS_GROW_RATE * flDuration;
		// Decrement ammo - the Immolator doesn't use clips
		GetOwner()->RemoveAmmo(1, m_iPrimaryAmmoType);
	}

	// Clamp
	m_flBurnRadius = MIN(m_flBurnRadius, MAX_BURN_RADIUS);

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	Vector vecSrc;
	Vector vecAiming;

	if (pOwner)
	{
		vecSrc = pOwner->Weapon_ShootPosition();
		vecAiming = pOwner->GetAutoaimVector(AUTOAIM_2DEGREES);
	}
	else
	{
		CBaseCombatCharacter *pOwner = GetOwner();

		vecSrc = pOwner->Weapon_ShootPosition();
		vecAiming = m_vecImmolatorTarget - vecSrc;
		VectorNormalize(vecAiming);
	}

	// Calculate damage
#ifndef RTBMP
	CTakeDamageInfo info = CTakeDamageInfo(this, this, sk_plr_dmg_immolator.GetFloat(), DMG_BURN | DMG_DIRECT);
#else
	CTakeDamageInfo info = CTakeDamageInfo( this, this, GetHL2MPWpnData().m_iPlayerDamage, DMG_BURN | DMG_DIRECT );
#endif

	trace_t	tr;
	UTIL_TraceLine(vecSrc, vecSrc + vecAiming * MAX_TRACE_LENGTH, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

	int brightness;
	brightness = 255 * (m_flBurnRadius / MAX_BURN_RADIUS);
	CBaseViewModel *pBeamEnt = pOwner->GetViewModel();
	CBeam *pBeam = CBeam::BeamCreate("sprites/bluelaser1.vmt", 1);

	pBeam->PointEntInit(tr.endpos, pBeamEnt);

	int    endAttachment = LookupAttachment("muzzle"); // change to what you use

	pBeam->SetEndAttachment(endAttachment);
	pBeam->SetNoise(1);
	pBeam->SetColor(0, 255, 0);
	pBeam->SetScrollRate(100);

	brightness = 255 * (m_flBurnRadius / MAX_BURN_RADIUS);
	pBeam->SetBrightness(brightness);

	pBeam->SetWidth(1);
	pBeam->SetEndWidth(20);
	pBeam->LiveForTime(0.1f);
	pBeam->RelinkBeam();


	if (tr.DidHitWorld())
	{
		int beams;

		for (beams = 0; beams < 5; beams++)
		{
			Vector vecDest;

			// Random unit vector
			vecDest.x = random->RandomFloat(-1, 1);
			vecDest.y = random->RandomFloat(-1, 1);
			vecDest.z = random->RandomFloat(0, 1);

			// Push out to radius dist.
			vecDest = tr.endpos + vecDest * m_flBurnRadius;

			CBaseViewModel *pBeamEnt = pOwner->GetViewModel();
			CBeam *pBeam = CBeam::BeamCreate("sprites/bluelaser1.vmt", 1);

			pBeam->PointEntInit(tr.endpos, pBeamEnt);

			int    endAttachment = LookupAttachment("muzzle"); // change to what you use

			pBeam->SetEndAttachment(endAttachment);
			pBeam->SetNoise(1);
			pBeam->SetColor(0, 255, 0);
			pBeam->SetScrollRate(100);

			int brightness;
			brightness = 255 * (m_flBurnRadius / MAX_BURN_RADIUS);
			pBeam->SetBrightness(brightness);

			pBeam->SetWidth(1);
			pBeam->SetEndWidth(20);
			pBeam->LiveForTime(0.1f);
			pBeam->RelinkBeam();
		}

		// Immolator starts to hurt a few seconds after the effect is seen
		if (m_flBurnRadius > 64.0)
		{
			ImmolationDamage(info, tr.endpos, m_flBurnRadius, CLASS_NONE);
		}
	}
	else
	{
		// The attack beam struck some kind of entity directly.
		ImmolateEntity(info, tr.m_pEnt->MyCombatCharacterPointer(), CLASS_NONE);
	}

	m_flTimeLastUpdatedRadius = gpGlobals->curtime;

	// If the immolator sphere reaches critical radius or if the gun runs out of ammo, stop firing
	if (m_flBurnRadius >= MAX_BURN_RADIUS || (pOwner && pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0))
	{
		StopImmolating();
		// Can't use the immolator again for a while
		m_flNextPrimaryAttack = gpGlobals->curtime + 5.0;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	// If the immolator's animation completes, play the fidget animation
	if (IsImmolating() && IsViewModelSequenceFinished())
	{
		SendWeaponAnim(ACT_VM_FIRE_IDLE);
	}

	// 1upD - If the mouse isn't held down and the immolator is firing, stop
	if (IsImmolating() && !(pOwner->m_nButtons & IN_ATTACK))
	{
		StopImmolating();
	}

	Vector vecAiming = pOwner->GetAutoaimVector( 0 );

	m_vPlayerMuzzleVector = GetMuzzleVector();
	m_vAiming = Vector( vecAiming.x, vecAiming.y, vecAiming.z );

	BaseClass::ItemPostFrame();
}

#ifndef CLIENT_DLL
void CWeaponImmolator::ImmolationDamage(const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore)
{
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	// iterate on all entities in the vicinity.
	for (CEntitySphereQuery sphere(vecSrc, flRadius); (pEntity = sphere.GetCurrentEntity()) != nullptr; sphere.NextEntity())
	{
		CBaseCombatCharacter *pBCC;

		pBCC = pEntity->MyCombatCharacterPointer();

		ImmolateEntity(info, pBCC, iClassIgnore);
	}
}

// Ignite this entity if possible
// 1upD
bool CWeaponImmolator::ImmolateEntity(const CTakeDamageInfo &info, CBaseCombatCharacter *pBCC, int iClassIgnore)
{
	if (pBCC)
	{
		pBCC->TakeDamage(info);

		if (!pBCC->IsOnFire())
		{
			// UNDONE: this should check a damage mask, not an ignore
			if (iClassIgnore != CLASS_NONE && pBCC->Classify() == iClassIgnore)
			{
				return false;
			}

			if (pBCC == GetOwner())
			{
				return false;
			}

			pBCC->Ignite(random->RandomFloat(15, 20));
			return true;
		}
	}

	return false;
}
#endif

Vector CWeaponImmolator::GetMuzzleVector( void )
{
	CRTBMP_Player *pPlayer = ToRTBMPPlayer( GetOwner() );
	if (!pPlayer)
		return vec3_origin;

	Vector muzzleVector = vec3_origin;
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );
	Vector vecOffset = Vector(40, 8, -15);
	muzzleVector = pPlayer->EyePosition() + vecOffset.x * vecForward + vecOffset.y * vecRight + vecOffset.z * vecUp;
	return muzzleVector;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CImmolatorPlasmaBall::CreateVPhysics(void)
{
	// Create the object in the physics system
	VPhysicsInitNormal(SOLID_BBOX, FSOLID_NOT_STANDABLE, false);

	return true;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
unsigned int CImmolatorPlasmaBall::PhysicsSolidMaskForEntity() const
{
	return (BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX) & ~CONTENTS_GRATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImmolatorPlasmaBall::Spawn(void)
{
	Precache();
	SetModel("models/immolator_bolt.mdl");
	PrecacheParticleSystem("burning_character_immo");
	SetMoveType(MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM);
	UTIL_SetSize(this, -Vector( 9.0f, 9.0f, 9.0f ), Vector( 9.0f, 9.0f, 9.0f ));
	SetSolid(SOLID_VPHYSICS);

	SetTouch(&CImmolatorPlasmaBall::BoltTouch);
}


void CImmolatorPlasmaBall::Precache(void)
{
	PrecacheModel("models/immolator_bolt.mdl");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CImmolatorPlasmaBall::BoltTouch(CBaseEntity *pOther)
{
	Assert(pOther);
	if (!pOther->IsSolid())
		return;

	//If the flare hit a person or NPC, do damage here.
	if (pOther && pOther->m_takedamage)
	{
		IgniteOtherIfAllowed(pOther);
		SUB_Remove();
		return;
	}

	// hit the world, check the material type here, see if the flare should stick.
	trace_t tr;
	Vector traceDir = GetAbsVelocity();
	//tr = BaseClass::GetTouchTrace();
	UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + traceDir * 64, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	//Scorch decal
	if (GetAbsVelocity().LengthSqr() > (250 * 250))
	{
		/*int index = decalsystem->GetDecalIndexForName("FadingScorch");
		if (index >= 0)
		{
			CBroadcastRecipientFilter filter;
			te->Decal(filter, 0.0, &tr.endpos, &tr.startpos, ENTINDEX(tr.m_pEnt), tr.hitbox, index);
			
		}*/
		//DecalTrace(&tr, "FadingScorch");
		UTIL_DecalTrace(&tr, "GreenScorch");
	}

	StopParticleEffects(this);
	SUB_Remove();
}

void CImmolatorPlasmaBall::IgniteOtherIfAllowed(CBaseEntity * pOther)
{
	// Burn the player if we're also a player and on a different team to them
	bool bOtherTeam = pOther->IsPlayer() &&
					( !HL2MPRules()->IsCoOp() ||
					( HL2MPRules()->IsTeamplay() ) ) && GetOwnerEntity()->GetTeamNumber() != pOther->GetTeamNumber();

	// Teamplay case.
	if ( pOther->IsPlayer() && ( !HL2MPRules()->IsCoOp() ) && GetOwnerEntity()->GetTeamNumber() != pOther->GetTeamNumber() )
	{
		bOtherTeam = true;
	}
	// DM/No team case.
	else if ( ( !HL2MPRules()->IsCoOp() ) && pOther->GetTeamNumber() == GetOwnerEntity()->GetTeamNumber() && pOther->GetTeamNumber() == TEAM_UNASSIGNED )
	{
		bOtherTeam = true;
	}
	// Coop case.
	else if ( HL2MPRules()->IsCoOp() && pOther->IsPlayer() &&
			( GetOwnerEntity()->GetTeamNumber() == pOther->GetTeamNumber() ) )
	{
		bOtherTeam = false;
	}
	// TODO: Default case/NPC logic?
	
	if ( bOtherTeam )
	{
		CRTBMP_Player *pPlayer = ToRTBMPPlayer(pOther);
		if (pPlayer && !pPlayer->IsSpawnProtected())
		{
			pPlayer->SetIgniteBegin( ToRTBMPPlayer(GetOwnerEntity()) );
			pPlayer->IgniteLifetimeGreen( IMMOLATOR_BURN_TIME );
		}
	}
	else if ( pOther->IsNPC() ) {
		CAI_BaseNPC *pNPC = pOther->MyNPCPointer();
		// Don't burn friendly NPCs
		if (pNPC->IsPlayerAlly())
			return;

		// Don't burn boss enemies
		if (FStrEq(STRING(pNPC->m_iClassname), "npc_combinegunship")
			|| FStrEq(STRING(pNPC->m_iClassname), "npc_combinedropship")
			|| FStrEq(STRING(pNPC->m_iClassname), "npc_strider")
			|| FStrEq(STRING(pNPC->m_iClassname), "npc_helicopter")
			|| FStrEq(STRING(pNPC->m_iClassname), "npc_cremator")
			)
			return;

		// Burn this NPC
		if (FStrEq( pNPC->GetClassname(), "npc_zombie" ))
		{
			// Zombies should stay ignited until they die
			pNPC->IgniteLifetimeGreen( 30 );
		}
		else
		{
			pNPC->IgniteLifetimeGreen( IMMOLATOR_BURN_TIME );
		}
	}

	CBaseCombatCharacter *pBCC = pOther->MyCombatCharacterPointer();
	if ( pBCC )
	{
		pBCC->SetImmolatorBurning( true );
		pBCC->SetLastImmolatorAttacker( GetOwnerEntity()->MyCombatCharacterPointer() );
	}


	// If this is a breakable prop, ignite it!
	CBreakableProp *pBreakable;
	pBreakable = dynamic_cast<CBreakableProp*>(pOther);
	if (pBreakable && pBreakable->GetHealth() > 0)
	{
		pBreakable->IgniteLifetimeGreen( IMMOLATOR_BURN_TIME );
		// Don't do damage to props that are on fire
		/*if (pBreakable->IsOnFire())
			return;*/
		if (pBreakable->GetHealth() < 3)
		{
#ifndef RTBMP
			pBreakable->Break( this, CTakeDamageInfo( this, this, sk_plr_dmg_immolator.GetFloat(), (DMG_BURN) ) );
#else
			pBreakable->Break( this, CTakeDamageInfo( this, this, IMMOLATOR_PLASMABALL_DMG, (DMG_BURN) ) );
#endif
		}
	}

	// Do damage
	//if (!FStrEq(STRING(pOther->m_iClassname), "prop_physics")) {
#ifndef RTBMP
		pOther->TakeDamage(CTakeDamageInfo(this, this, sk_plr_dmg_immolator.GetFloat(), (DMG_BURN | DMG_DIRECT | DMG_SLOWBURN)));
#else
	pOther->TakeDamage( CTakeDamageInfo( this, GetOwnerEntity(), IMMOLATOR_PLASMABALL_DMG, (DMG_BURN | DMG_DIRECT | DMG_SLOWBURN) ) );
#endif
	//}

}
#endif

#ifdef CLIENT_DLL
void CWeaponImmolator::OnDataChanged( DataUpdateType_t updateType )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		ParticleProp()->StopEmission( m_hImmolatorFlame );
		m_hImmolatorFlame = NULL;
		return;
	}

	if ( m_bImmolating && !m_hImmolatorFlame){
		m_hImmolatorFlame = ParticleProp()->Create( "weapon_immolator_flame", PATTACH_CUSTOMORIGIN, 0, m_vPlayerMuzzleVector );
	}
	else if (!m_bImmolating && m_hImmolatorFlame){
		ParticleProp()->StopEmission( m_hImmolatorFlame );
		m_hImmolatorFlame = NULL;
	}

	// set position & angles of the flame stream as a workaround for the weird attachment we have on the viewmodel; this also ensures consistency with the flame entities' paths
	if ( m_hImmolatorFlame ){
		m_hImmolatorFlame->SetControlPoint( 0, m_vPlayerMuzzleVector );
		QAngle angAiming;
		VectorAngles( m_vAiming, angAiming );
		Quaternion qAiming;
		AngleQuaternion( angAiming, qAiming );
		m_hImmolatorFlame->SetControlPointOrientation( 0, qAiming );
	}

	BaseClass::OnDataChanged( updateType );
}
#endif