//========= Copyright (c) RTBR Team, 2022 ============//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "rumble_shared.h"
#include "in_buttons.h"
#include "gamestats.h"

#ifdef CLIENT_DLL
	#include "c_rtbmp_player.h"
#else
	#include "rtbmp_player.h"
	#include "basecombatcharacter.h"
	#include "ai_basenpc.h"
	#include "ai_memory.h"
	#include "soundent.h"
	#include "ai_basenpc.h"
	#include "game.h"
#endif

#include "weapon_hl2mpbase_machinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
	#define CWeaponHMG C_WeaponHMG
#endif

#define HMG_BURST_TIME 0.50f
#define HMG_BURST_SIZE 5
#define HMG_BURST_SPEED 3.0f

class CWeaponHMG : public CHL2MPMachineGun {
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponHMG, CHL2MPMachineGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponHMG();

	virtual void	Precache( void );

	float	GetFireRate( void );

	void	AddViewKick( void );
	virtual void PrimaryAttack( void );
	virtual void SecondaryAttack( void );
	void	BurstAttack( int burstSize, float cycleRate );

	float GetBurstSpeed( void );
	virtual float	GetBurstCycleRate( void );
	virtual int GetBurstSize( void );
	float	GetBurstFireRate( void ) { return (float)GetFireRate() / (float)HMG_BURST_SPEED; }

	virtual bool Reload( void );
	void ToggleScope( void );
	void Scope( void );
	void Unscope( void );
	bool Holster( CBaseCombatWeapon* pSwitchingTo );
	void ItemPostFrame( void );

#ifndef CLIENT_DLL
	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int	WeaponRangeAttack2Condition( float flDot, float flDist );

	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary ) OVERRIDE;
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator ) OVERRIDE;
#endif

	bool ShouldWeaponFidget( void );
	virtual bool ShouldSecondaryAttackRecoil() const { return true; }

	CNetworkVar( bool, m_bIsScoped );

	virtual const Vector& GetBulletSpread( void );

	float	m_flScopeTime;

#ifdef CLIENT_DLL
	virtual void OnDataChanged( DataUpdateType_t updateType );

	void EnableScope();

	void DisableScope();
#endif

	DECLARE_ACTTABLE();

private:
	int		m_iBurstSize;
	float	m_flLastPrimaryAttack;
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponHMG, DT_WeaponHMG )

BEGIN_NETWORK_TABLE( CWeaponHMG, DT_WeaponHMG )
#ifdef CLIENT_DLL
RecvPropBool( RECVINFO( m_bIsScoped ) ),
#else
SendPropBool( SENDINFO( m_bIsScoped ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponHMG )
DEFINE_PRED_FIELD( m_bIsScoped, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_iBurstSize, FIELD_INTEGER, 0 ),
DEFINE_PRED_FIELD( m_flLastPrimaryAttack, FIELD_FLOAT, 0 ),
DEFINE_PRED_FIELD_TOL( m_flScopeTime, FIELD_FLOAT, 0, TD_MSECTOLERANCE ),
END_PREDICTION_DATA()
#endif

BEGIN_DATADESC( CWeaponHMG )
DEFINE_FIELD( m_bIsScoped, FIELD_BOOLEAN ),
DEFINE_FIELD( m_flScopeTime, FIELD_FLOAT ),
END_DATADESC()

LINK_ENTITY_TO_CLASS(weapon_hmg, CWeaponHMG);
PRECACHE_WEAPON_REGISTER(weapon_hmg);

acttable_t	CWeaponHMG::m_acttable[] = {
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

IMPLEMENT_ACTTABLE(CWeaponHMG);

CWeaponHMG::CWeaponHMG() {
	m_fMinRange1 = 0; m_fMaxRange1 = 2000;
	m_bAltFiresUnderwater = m_bFiresUnderwater = false;
	m_iBurstSize = 0;
}

void CWeaponHMG::Precache( void )
{
	PrecacheModel( "models/weapons/hmg/m_hmg.mdl" );

	BaseClass::Precache();
}

float CWeaponHMG::GetFireRate( void )
{
	return 0.12f;
}

float CWeaponHMG::GetBurstCycleRate( void )
{
	return HMG_BURST_TIME;
}

int CWeaponHMG::GetBurstSize( void )
{
	return HMG_BURST_SIZE;
}

float CWeaponHMG::GetBurstSpeed( void )
{
	return HMG_BURST_SPEED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHMG::AddViewKick( void )
{
#define	EASY_DAMPEN			0.5f
#define	MAX_VERTICAL_KICK	8.0f	//Degrees
#define	SLIDE_LIMIT			5.0f	//Seconds

	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (pPlayer == NULL)
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

void CWeaponHMG::PrimaryAttack( void ) {
	if (m_flNextSecondaryAttack < gpGlobals->curtime){
		m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate() * 1.5f; // cooldown on m2 so we can't fire 6 at once by m1+release+m2
	}
	BaseClass::PrimaryAttack();
}

void CWeaponHMG::SecondaryAttack( void ) {
	BurstAttack( GetBurstSize(), GetBurstCycleRate() );
}

void CWeaponHMG::BurstAttack( int burstSize, float cycleRate )
{
	// Bursts always use the weapon's fire rate
	float fireRate = GetBurstFireRate();

	if (m_bFireOnEmpty)
	{
		return;
	}

	if (!GetOwner())
	{
		Msg( "Error: Pistol has NULL owner!" );
		return;
	}

	// Only the player fires this way so we can cast
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetOwner() );
	if (!pPlayer)
		return;

	m_nShotsFired++;

	// Send the animation early to properly handle recoil animation
	SendWeaponAnim( GetPrimaryAttackActivity() );

	pPlayer->DoMuzzleFlash();

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	int iBulletsToFire = 0;

	// If the last time fired was longer ago than the cycle rate, reset the burst count
	if (gpGlobals->curtime - m_flLastPrimaryAttack >= cycleRate)
	{
		m_iBurstSize = 0;
	}

	// MUST call sound before removing a round from the clip of a CHLMachineGun
	while (m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		WeaponSound( SINGLE, m_flNextPrimaryAttack );

		// Add the bullets to fire to the burst count
		m_iBurstSize++;

		// If the burst count is greater than the burst size, wait for the cycle rate and adjust
		if (m_iBurstSize >= burstSize) {
			m_iBurstSize = 0;
			m_nShotsFired = burstSize > 1 ? 0 : m_nShotsFired; // Reset the shots fired counter so the correct activity plays
			m_flNextPrimaryAttack = gpGlobals->curtime + cycleRate - fireRate;
			m_flNextSecondaryAttack = gpGlobals->curtime + cycleRate - fireRate; // Pistol shares primary attack between primary and secondary
		}
		else {
			m_flNextPrimaryAttack = gpGlobals->curtime + fireRate;
			m_flNextSecondaryAttack = gpGlobals->curtime + fireRate; // Pistol shares primary attack between primary and secondary
		}

		iBulletsToFire++;
	}

	// Make sure we don't fire more than the amount in the clip, if this weapon uses clips
	if (UsesClipsForAmmo1())
	{
		if (iBulletsToFire > m_iClip1)
			iBulletsToFire = m_iClip1;
		m_iClip1 -= MIN( iBulletsToFire, m_iClip1 );
	}

#ifndef CLIENT_DLL
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
#endif

	// Fire the bullets
	FireBulletsInfo_t info;
	info.m_iShots = iBulletsToFire;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition();
	info.m_vecDirShooting = ToBasePlayer( GetOwner() )->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
	info.m_vecSpread = pPlayer->GetAttackSpread( this );
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 1;
	pPlayer->FireBullets( info );

	// Update last attack time - need to do this after calculating weapon spread
	m_flLastPrimaryAttack = gpGlobals->curtime;

	//Factor in the view kick
	AddViewKick();

#ifndef CLIENT_DLL
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer );
#endif
	if (!m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	}

	pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
	// Register a muzzleflash for the AI
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
#endif
	SetWeaponIdleTime( gpGlobals->curtime + 3.0f );

#ifndef CLIENT_DLL
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (pOwner)
	{
		gamestats->Event_WeaponFired( pOwner, true, GetClassname() );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: De-scope HMG on reload
//-----------------------------------------------------------------------------
bool CWeaponHMG::Reload( void ){
	bool reloadBool = BaseClass::Reload();
	if (!GetOwner()->IsNPC()) {
		if (m_bIsScoped && reloadBool) {
			Unscope();
		}
	}
	return reloadBool;
}

void CWeaponHMG::ItemPostFrame( void ){
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (m_iClip1 <= 0 && pPlayer->m_nButtons & IN_ATTACK2)
	{
		pPlayer->m_nButtons &= ~IN_ATTACK2;
	}

	BaseClass::ItemPostFrame();

	UpdateAutoFire();

	m_fFireDuration = (pPlayer->m_nButtons & IN_ATTACK) || (pPlayer->m_nButtons & IN_ATTACK2) ? (m_fFireDuration + gpGlobals->frametime) : 0.0f;
	CheckReload();

	if (pPlayer->m_nButtons & IN_ATTACK3)
	{
		if (m_flScopeTime <= gpGlobals->curtime)
		{
			ToggleScope();
			pPlayer->m_nButtons &= ~IN_ATTACK3;
		}
	}
}

void CWeaponHMG::ToggleScope( void ){
	if (GetActivity() == ACT_VM_RELOAD)
		return;

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
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

//-----------------------------------------------------------------------------
// Purpose: Descope weapon on holster
//-----------------------------------------------------------------------------
bool CWeaponHMG::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if (GetOwner()->IsPlayer()) {
		if (m_bIsScoped) {
			Unscope();
		}
	}

	return BaseClass::Holster( pSwitchingTo );
}

const Vector& CWeaponHMG::GetBulletSpread( void ){
	static Vector cone;
	if (!m_iBurstSize){
		cone = VECTOR_CONE_3DEGREES;
	}
	else {
		cone = VECTOR_CONE_8DEGREES;
	}

	if (m_bIsScoped)
		cone *= 0.75f;

	return cone;
}

#ifndef CLIENT_DLL
int CWeaponHMG::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	return COND_NONE;
}

void CWeaponHMG::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );
	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), 0 );

	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;
}

void CWeaponHMG::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
}

void CWeaponHMG::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_SMG1:
	{
		Vector vecShootOrigin, vecShootDir;
		QAngle angDiscard;

		// Support old style attachment point firing
		if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment( pEvent->options, vecShootOrigin, angDiscard )))
		{
			vecShootOrigin = pOperator->Weapon_ShootPosition();
		}

		CAI_BaseNPC *npc = pOperator->MyNPCPointer();
		ASSERT( npc != NULL );
		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

		FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
	}
	break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}
#endif

void CWeaponHMG::Scope( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if ( pPlayer->SetFOV( this, pPlayer->GetFOV() / 1.5f, 0.1f ) ) // 1.5x zoom
	{
		pPlayer->m_Local.m_bDrawViewmodel = false;
#ifndef CLIENT_DLL
		UTIL_ScreenFade( pPlayer, { 0, 0, 0, 255 }, 0.2, 0, FFADE_IN );
#endif
		WeaponSound( SPECIAL1 );
		m_bIsScoped = true;
	}
}

void CWeaponHMG::Unscope( void )
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
	}
}

bool CWeaponHMG::ShouldWeaponFidget()
{
	return !m_bIsScoped;
}

#ifdef CLIENT_DLL
void CWeaponHMG::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	if (m_bIsScoped) {
		EnableScope();
	}
	else {
		DisableScope();
	}
}

void CWeaponHMG::EnableScope() {
	CBasePlayer* pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	pPlayer->SetFX( SFX_HMG, true );
}
void C_WeaponHMG::DisableScope() {
	CBasePlayer* pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	pPlayer->SetFX( SFX_HMG, false );
}
#endif