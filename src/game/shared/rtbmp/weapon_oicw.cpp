//========= Copyright (c) RTBR Team, 2018 ============//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "ai_basenpc_shared.h"

#ifdef CLIENT_DLL
	#include "c_rtbmp_player.h"
	#include "prediction.h"
#else
	#include "rtbmp_player.h"
	#include "explode.h"
	#include "basecombatcharacter.h"
	#include "grenade_ar2.h"
	#include "soundent.h"
	#include "ai_memory.h"
	#include "rtbmp_player.h"
#endif

#include "weapon_hl2mpbase_machinegun.h"

#include "rumble_shared.h"
#include "gamestats.h"
#include "steam/isteamuser.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
	#define CWeaponOICW C_WeaponOICW
#endif

#define OICW_GRENADE_DAMAGE 100.0f
#define OICW_GRENADE_RADIUS 250.0f
#define OICW_ACCURACY_RESET_TIME 0.75f
#define OICW_MIN_CROSSHAIR_SCALE 0.5f
#define OICW_MED_CROSSHAIR_SCALE1 0.65f
#define OICW_MED_CROSSHAIR_SCALE2 0.8f
#define OICW_MAX_CROSSHAIR_SCALE 1.0f
#define OICW_MIN_SCALE_TIME 0.15f
#define OICW_NORM_SCALE_TIME 0.05f

#ifdef MAPBASE
extern acttable_t *GetAR2Acttable();
extern int GetAR2ActtableCount();
#endif

class CWeaponOICW : public CHL2MPMachineGun {
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponOICW, CHL2MPMachineGun);
	CWeaponOICW();

	void Precache(void);

	void ItemPostFrame(void);

	bool Holster(CBaseCombatWeapon * pSwitchingTo);

	void	OnPickedUp(CBaseCombatCharacter *pNewOwner);

#ifndef CLIENT_DLL
	void FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir);
	void FireNPCSecondaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire(CBaseCombatCharacter  *pOperator, bool bSecondary) OVERRIDE;

	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator) OVERRIDE;

	int CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
#endif

	virtual void SecondaryAttack(void);

	virtual bool Reload();

	void	AddViewKick(void);

	virtual void	PrimaryAttack();

	float	GetFireRate(void);

	void ToggleScope( void );

	void Scope( void );

	void Unscope( void );

	//virtual bool ShouldDisplayAltFireHUDHint();

	Vector GetWeaponInaccuracy( void );

	bool ShouldWeaponFidget( void );

#ifdef CLIENT_DLL
	virtual void GetWeaponCrosshairScale( float &flScale );
#endif
	void SetDesiredCrosshairScale( float scale, float deltaTime );
	void UpdateCrosshairScale( void );

	float	m_flScopeTime;

	//bool m_bSecondaryReloadHintDisplayed;

	float	m_flLastShot;
	int m_iShotsFired;
#ifdef CLIENT_DLL
	float m_flCrosshairScale;
	float m_flDesiredCrosshairScale;
	float m_flOldCrosshairScale;
	float m_flCrosshairScaleStartTime;
	float m_flCrosshairScaleEndTime;
#endif
	CNetworkVar(bool, m_bIsPlayer);

	CNetworkVar(bool, m_bIsScoped);

	virtual const Vector& GetBulletSpread(void) {
		static const Vector cone = VECTOR_CONE_3DEGREES;
		static Vector coneScoped;
		if (m_bIsScoped)
			coneScoped = GetWeaponInaccuracy();

		return m_bIsScoped ? coneScoped : cone;
	}

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef CLIENT_DLL
	virtual void OnDataChanged( DataUpdateType_t updateType );

	void EnableScope();

	void DisableScope();
#endif

#ifdef MAPBASE
	virtual acttable_t		*GetBackupActivityList() { return GetAR2Acttable(); }
	virtual int				GetBackupActivityListCount() { return GetAR2ActtableCount(); }
#endif

	DECLARE_ACTTABLE();

private:
	CWeaponOICW( const CWeaponOICW & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponOICW, DT_WeaponOICW )

BEGIN_NETWORK_TABLE( CWeaponOICW, DT_WeaponOICW )
#ifdef CLIENT_DLL
RecvPropBool( RECVINFO( m_bIsScoped ) ),
RecvPropBool( RECVINFO( m_bIsPlayer ) ),
#else
SendPropBool( SENDINFO( m_bIsScoped ) ),
SendPropBool( SENDINFO( m_bIsPlayer ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponOICW )
DEFINE_PRED_FIELD( m_bIsScoped, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_bIsPlayer, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD_TOL( m_flScopeTime, FIELD_FLOAT, 0, TD_MSECTOLERANCE ),
DEFINE_PRED_FIELD_TOL( m_flLastShot, FIELD_FLOAT, 0, TD_MSECTOLERANCE ),
DEFINE_PRED_FIELD( m_iShotsFired, FIELD_INTEGER, 0 ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_oicw, CWeaponOICW);
PRECACHE_WEAPON_REGISTER(weapon_oicw);

BEGIN_DATADESC(CWeaponOICW)
DEFINE_FIELD(m_bIsScoped, FIELD_BOOLEAN),
DEFINE_FIELD(m_flScopeTime, FIELD_TIME),
//DEFINE_FIELD(m_bSecondaryReloadHintDisplayed, FIELD_BOOLEAN),
DEFINE_FIELD( m_flLastShot, FIELD_TIME ),
DEFINE_FIELD( m_iShotsFired, FIELD_INTEGER ),
END_DATADESC()

acttable_t	CWeaponOICW::m_acttable[] = {
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_AR2,				true },
	{ ACT_IDLE,						ACT_IDLE_AR2,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_AR2,			true },

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },
	
// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_AR2_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_AR2_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_AR2,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_AR2_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_AR2,			false },//always aims

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
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_AR2_LOW,		true },
	{ ACT_COVER_LOW,				ACT_COVER_AR2_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_AR2_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_AR2,		true },

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
	{ ACT_HL2MP_IDLE,                    ACT_HL2MP_IDLE_AR2,                    false },
	{ ACT_HL2MP_RUN,                    ACT_HL2MP_RUN_AR2,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,            ACT_HL2MP_IDLE_CROUCH_AR2,            false },
	{ ACT_HL2MP_WALK_CROUCH,            ACT_HL2MP_WALK_CROUCH_AR2,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,    ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,            ACT_HL2MP_GESTURE_RELOAD_AR2,        false },
	{ ACT_HL2MP_JUMP,                    ACT_HL2MP_JUMP_AR2,                    false },
#if EXPANDED_HL2DM_ACTIVITIES
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_AR2,					false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_AR2,	false },
#endif
#endif
};

IMPLEMENT_ACTTABLE(CWeaponOICW);

//-----------------------------------------------------------------------------
// Purpose: Precaches weapon
//-----------------------------------------------------------------------------
void CWeaponOICW::Precache(void)
{
#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "grenade_ar2" );
#endif
	PrecacheModel( "models/weapons/oicw/m_oicw.mdl" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Makes sure first time pickup anim isn't overridden
//-----------------------------------------------------------------------------
void CWeaponOICW::OnPickedUp(CBaseCombatCharacter *pNewOwner)
{
	BaseClass::OnPickedUp(pNewOwner);
}

//-----------------------------------------------------------------------------
// Purpose: Deals with secondary reload and teritary attack
//-----------------------------------------------------------------------------
void CWeaponOICW::ItemPostFrame(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	m_bIsPlayer = GetOwner()->IsPlayer();
	
	//if (!m_bSecondaryReloadHintDisplayed && Clip2() < GetMaxClip2() && pPlayer->GetAmmoCount(m_iSecondaryAmmoType) > 0)
	//{
	//	UTIL_HudHintText( GetOwner(), "#RTBR_OICW_SecondaryReload" );
	//	m_bSecondaryReloadHintDisplayed = true;
	//}

	if ( m_iShotsFired > 0 && gpGlobals->curtime > m_flLastShot + OICW_ACCURACY_RESET_TIME && m_bIsScoped ) {
		m_iShotsFired = 0;
		SetDesiredCrosshairScale( OICW_MIN_CROSSHAIR_SCALE, OICW_MIN_SCALE_TIME );
	}

	if (GetActivity() != ACT_VM_FIRSTDRAW) {
		if (pPlayer->m_nButtons & IN_ATTACK3)
		{
			if (m_flScopeTime <= gpGlobals->curtime)
			{
				ToggleScope();
			}
		}
	}

	UpdateCrosshairScale();

	BaseClass::ItemPostFrame();

}

//-----------------------------------------------------------------------------
// Purpose: Descope weapon on holster
//-----------------------------------------------------------------------------
bool CWeaponOICW::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	if (GetOwner()->IsPlayer()) {
		if (m_bIsScoped)
		{
			Unscope();
		}
		SetDesiredCrosshairScale( OICW_MAX_CROSSHAIR_SCALE, 0.0f );
	}

	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Set some OICW variables
//-----------------------------------------------------------------------------
CWeaponOICW::CWeaponOICW() {
	m_fMinRange1 = 0; m_fMaxRange1 = 2000;
	m_bAltFiresUnderwater = m_bFiresUnderwater = false;
	m_bIsScoped = false;
	m_flScopeTime = 0.0f;
	m_flLastShot = 0.0f;
	m_iShotsFired = 0;
#ifdef CLIENT_DLL
	m_flCrosshairScale = 1.0f;
	m_flOldCrosshairScale = 1.0f;
	m_flDesiredCrosshairScale = 1.0f;
#endif
	//m_bSecondaryReloadHintDisplayed = false;
}

//-----------------------------------------------------------------------------
// Purpose: Set OICW fire rate
//-----------------------------------------------------------------------------
float CWeaponOICW::GetFireRate(void)
{
	if (!GetOwner()->IsNPC()) {
		if (m_bIsScoped) {
			return 0.25f;
		}
		else {
			return 0.1f;
		}
	}
	else {
		return 0.1f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set custom scope fire sound
//-----------------------------------------------------------------------------
void CWeaponOICW::PrimaryAttack() {
	if (GetActivity() == ACT_VM_SECONDARYRELOAD)
		return;
	if (!GetOwner()->IsNPC()) {
		if (m_bIsScoped) {
			BaseClass::PrimaryAttack(BURST);
			m_flLastShot = gpGlobals->curtime;
			m_iShotsFired++; // increase shots fired after primary attack, so it doesn't mess up the inaccuracy
		}
		else {
			BaseClass::PrimaryAttack();
		}
	}
	else {
		BaseClass::PrimaryAttack();
	}
	
}
//-----------------------------------------------------------------------------
// Purpose: Add view kick with scope effects
//-----------------------------------------------------------------------------
void CWeaponOICW::AddViewKick(void)
{
	float	EASY_DAMPEN;
	float	MAX_VERTICAL_KICK;
	float	SLIDE_LIMIT;

	if (m_bIsScoped) {
		EASY_DAMPEN = 0.5f;
		MAX_VERTICAL_KICK = 0.1f;	//Degrees
		SLIDE_LIMIT = 0.2f;	//Seconds
	}
	else {
		EASY_DAMPEN = 0.5f;
		MAX_VERTICAL_KICK = 1.0f;	//Degrees
		SLIDE_LIMIT = 2.0f;	//Seconds
	}

	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	DoMachineGunKick(pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT);
}

//-----------------------------------------------------------------------------
// Purpose: De-scope OICW on reload, and handle reloading both magazines.
//-----------------------------------------------------------------------------
bool CWeaponOICW::Reload() {
	bool reloadBool = BaseClass::Reload();
	if (GetOwner()->IsPlayer()) 
	{
		if (m_bIsScoped && reloadBool) 
		{
			Unscope();
		}
	}

	return reloadBool;
}

//-----------------------------------------------------------------------------
// Purpose: Fire grenade
//-----------------------------------------------------------------------------
void CWeaponOICW::SecondaryAttack(void)
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
	return;

	if (GetActivity() == ACT_VM_SECONDARYRELOAD || m_bIsScoped)
		return;
	
	//Must have ammo
	if ( (pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) <= 0) || (pPlayer->GetWaterLevel() == 3) )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		BaseClass::WeaponSound( EMPTY );
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		return;
	}

	if( m_bInReload )
		m_bInReload = false;
	
	// MUST call sound before removing a round from the clip of a CMachineGun
	BaseClass::WeaponSound( WPN_DOUBLE );

#ifndef CLIENT_DLL
	pPlayer->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAGS_NONE );
#endif

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecThrow;
	// Don't autoaim on grenade tosses
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow );
	VectorScale( vecThrow, 1000.0f, vecThrow );

#ifndef CLIENT_DLL
	//Create the grenade
	QAngle angles;
	VectorAngles( vecThrow, angles );
	CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create( "grenade_ar2", vecSrc, angles, pPlayer );
	pGrenade->SetAbsVelocity( vecThrow );
	
	pGrenade->SetLocalAngularVelocity( RandomAngle( -400, 400 ) );
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE ); 
	pGrenade->SetThrower( GetOwner() );
	pGrenade->SetDamage( OICW_GRENADE_DAMAGE );
	pGrenade->SetDamageRadius( OICW_GRENADE_RADIUS );
#endif

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

#ifndef CLIENT_DLL
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 1000, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON );
#endif

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	
	// Decrease ammo
	pPlayer->RemoveAmmo( 1, m_iSecondaryAmmoType );

	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
	
	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
	
#ifndef CLIENT_DLL
	// Register a muzzleflash for the AI.
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );	
	
	gamestats->Event_WeaponFired( pPlayer, false, GetClassname() );

	CRTBMP_Player *pMPPlayer = ToRTBMPPlayer( pPlayer );
	if (pMPPlayer)
		pMPPlayer->DisableSpawnProtection();
#endif
	//m_iSecondaryAttacks++;
}

//-----------------------------------------------------------------------------
// Purpose: Scope and de-scope the weapon
//-----------------------------------------------------------------------------
void CWeaponOICW::ToggleScope(void) {
	if (GetOwner()->IsPlayer()) {
		if (GetActivity() == ACT_VM_SECONDARYRELOAD || GetActivity() == ACT_VM_RELOAD)
			return;

		if (m_flScopeTime >= gpGlobals->curtime) {
			return;
		}

		if (m_bIsScoped)
			Unscope();
		else
			Scope();

		m_flScopeTime = gpGlobals->curtime + 0.4f;
	}
}

#ifndef CLIENT_DLL
void CWeaponOICW::FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir)
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());

	FireBulletsInfo_t info( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType );

	pOperator->FireBullets( info );

	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;
}

void CWeaponOICW::FireNPCSecondaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	Msg( "OICW NPC Secondary attack triggered\n" );

	// MUST call sound before removing a round from the clip of a CMachineGun
	BaseClass::WeaponSound( WPN_DOUBLE );

	//Create the grenade
	QAngle angles;
	//VectorAngles(vecShootDir, angles);
	CGrenadeAR2 *pGrenade = (CGrenadeAR2 *)Create( "grenade_ar2", vecShootOrigin, angles, GetOwner() );
	pGrenade->SetAbsVelocity( vecShootDir );

	pGrenade->SetLocalAngularVelocity( RandomAngle( -400, 400 ) );
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	pGrenade->SetThrower( GetOwner() );
	pGrenade->SetDamage( OICW_GRENADE_DAMAGE );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 1000, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON );

	// Decrease clip
	m_iClip2--;

	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;

	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

	pOperator->DoMuzzleFlash();
}

void CWeaponOICW::Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary)
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	if (bSecondary)
	{
		FireNPCSecondaryAttack( pOperator, vecShootOrigin, vecShootDir );
	}
	else
	{
		FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
	}
}

void CWeaponOICW::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_AR2:
	case EVENT_WEAPON_AR2_ALTFIRE:
	case EVENT_WEAPON_PISTOL_FIRE:	// NOTE: This is actually for the OICW.
	{
		Vector vecShootOrigin, vecShootDir, vecShootPos;
		QAngle angDiscard;

		// Support old style attachment point firing
		if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment( pEvent->options, vecShootOrigin, angDiscard )))
		{
			vecShootOrigin = pOperator->Weapon_ShootPosition();
		}

		CAI_BaseNPC *npc = pOperator->MyNPCPointer();
		ASSERT( npc != NULL );

		if (pEvent->event == EVENT_WEAPON_AR2 || pEvent->event == EVENT_WEAPON_PISTOL_FIRE)
		{
			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
			FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
		}
		else if (pEvent->event == EVENT_WEAPON_AR2_ALTFIRE)
		{
			vecShootPos = npc->GetAltFireTarget();
			vecShootDir = VecCheckThrow( pOperator, vecShootOrigin, vecShootPos, 1000.0f, 0.75f );
			FireNPCSecondaryAttack( pOperator, vecShootOrigin, vecShootDir );
		}
		break;
	}

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}
#endif

void CWeaponOICW::Scope( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (pPlayer->SetFOV( this, 30, 0.2f ))
	{
		pPlayer->m_Local.m_bDrawViewmodel = false;
#ifndef CLIENT_DLL
		UTIL_ScreenFade( pPlayer, { 0, 0, 0, 255 }, 0.4, 0, FFADE_IN );
#endif
		WeaponSound( SPECIAL1 );

		m_bIsScoped = true;
		SetDesiredCrosshairScale( OICW_MIN_CROSSHAIR_SCALE, OICW_MIN_SCALE_TIME );
	}
}

void CWeaponOICW::Unscope( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (pPlayer->SetFOV( this, 0 ))
	{
		pPlayer->m_Local.m_bDrawViewmodel = true;
		// Zoom out to the default zoom level
		WeaponSound( SPECIAL2 );
		m_bIsScoped = false;
		SetDesiredCrosshairScale( OICW_MAX_CROSSHAIR_SCALE, 0.0f );
	}
}

Vector CWeaponOICW::GetWeaponInaccuracy()
{
	Vector cone;
	if (m_iShotsFired == 0)
	{
		// first shot perfect accuracy
		cone = vec3_origin;
		SetDesiredCrosshairScale( OICW_MED_CROSSHAIR_SCALE1, OICW_NORM_SCALE_TIME );
	}
	else if (m_iShotsFired < 3)
	{
		// next two shots very good accuracy
		cone = VECTOR_CONE_1DEGREES * 0.25f; // roughly 0.25 degrees
		if (m_iShotsFired == 2)
			SetDesiredCrosshairScale( OICW_MED_CROSSHAIR_SCALE2, OICW_NORM_SCALE_TIME );
	}
	else if (m_iShotsFired < 5)
	{
		// next two shots very good accuracy
		cone = VECTOR_CONE_1DEGREES * 0.5f; // roughly 0.5 degrees
		if (m_iShotsFired == 4)
			SetDesiredCrosshairScale( OICW_MAX_CROSSHAIR_SCALE, OICW_NORM_SCALE_TIME );
	}
	else
	{
		// all other shots worse accuracy
		cone = VECTOR_CONE_1DEGREES * 1.5f; // roughly 1.5 degrees
		SetDesiredCrosshairScale( OICW_MAX_CROSSHAIR_SCALE, OICW_NORM_SCALE_TIME );
	}

	return cone;
}

bool CWeaponOICW::ShouldWeaponFidget()
{
	return !m_bIsScoped;
}

#ifdef CLIENT_DLL
void CWeaponOICW::GetWeaponCrosshairScale( float &flScale )
{
	flScale = m_flCrosshairScale;
}
#endif

void CWeaponOICW::SetDesiredCrosshairScale( float scale, float deltaTime )
{
#ifdef CLIENT_DLL
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (m_flDesiredCrosshairScale == scale || m_flCrosshairScale == scale)
		return;

	if ( prediction->IsFirstTimePredicted() )
	{
		if ( deltaTime <= 0 )
		{
			m_flCrosshairScale = m_flOldCrosshairScale = m_flDesiredCrosshairScale = scale;
			m_flCrosshairScaleStartTime = m_flCrosshairScaleEndTime = gpGlobals->curtime;
		}
		else
		{
			if ( m_flCrosshairScale != m_flDesiredCrosshairScale )
				m_flCrosshairScale = m_flDesiredCrosshairScale;
			m_flOldCrosshairScale = m_flCrosshairScale;
			m_flDesiredCrosshairScale = scale;
			m_flCrosshairScaleStartTime = gpGlobals->curtime;
			m_flCrosshairScaleEndTime = gpGlobals->curtime + deltaTime;
		}
	}
#endif
}

void CWeaponOICW::UpdateCrosshairScale( void )
{
#ifdef CLIENT_DLL
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (m_flCrosshairScale == m_flDesiredCrosshairScale)
		return;

	m_flCrosshairScale = RemapValClamped(	gpGlobals->curtime,
											m_flCrosshairScaleStartTime,
											m_flCrosshairScaleEndTime,
											m_flOldCrosshairScale,
											m_flDesiredCrosshairScale );
#endif
}

#ifdef CLIENT_DLL
void CWeaponOICW::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	if (m_bIsScoped && m_bIsPlayer) {
		EnableScope();
	}
	else if (!m_bIsScoped && m_bIsPlayer) {
		DisableScope();
	}
}

void CWeaponOICW::EnableScope() {
	CBasePlayer* pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	pPlayer->SetFX( SFX_OICW, true );
}
void CWeaponOICW::DisableScope() {
	CBasePlayer* pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	pPlayer->SetFX( SFX_OICW, false );
}
#endif
