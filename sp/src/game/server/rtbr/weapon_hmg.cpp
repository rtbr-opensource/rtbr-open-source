//========= Copyright (c) RTBR Team, 2022 ============//

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
#include "soundent.h"
#include "ai_basenpc.h"
#include "game.h"
#include "in_buttons.h"
#include "gamestats.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar  sk_hmg_burst_time( "sk_hmg_burst_time", "0");
ConVar  sk_hmg_burst_size( "sk_hmg_burst_size", "0");
ConVar  sk_hmg_burst_speed( "sk_hmg_burst_speed", "0");

class CWeaponHMG : public CHLSelectFireMachineGun {
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponHMG, CHLSelectFireMachineGun );
	DECLARE_SERVERCLASS();

	CWeaponHMG();

	virtual void	Precache( void );

	float	GetFireRate( void );

	void	AddViewKick( void );
	virtual float	GetBurstCycleRate( void );
	virtual int GetBurstSize( void );
	virtual void PrimaryAttack( void );
	virtual void SecondaryAttack( void );
	float GetBurstSpeed( void );

	virtual bool Reload( void );
	void ToggleScope( void );
	void Scope( void );
	void Unscope( void );
	bool Holster( CBaseCombatWeapon* pSwitchingTo );
	void ItemPostFrame( void );

	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int	WeaponRangeAttack1Condition( float flDot, float flDist );
	int	WeaponRangeAttack2Condition( float flDot, float flDist );

	const WeaponProficiencyInfo_t* GetProficiencyValues();

	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary ) OVERRIDE;
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator ) OVERRIDE;

	bool ShouldWeaponFidget( void );
	virtual bool ShouldSecondaryAttackRecoil() const { return true; }

	CNetworkVar( bool, m_bIsScoped );

	virtual const Vector& GetBulletSpread( void );

	float	m_flScopeTime;

	// Bullet launch information
	int				GetMinBurst(void) { return 4; }
	int				GetMaxBurst(void) { return 12; }

	DECLARE_ACTTABLE();

private:
	float m_flLastShot;
};

IMPLEMENT_SERVERCLASS_ST( CWeaponHMG, DT_WeaponHMG )
SendPropBool( SENDINFO( m_bIsScoped ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponHMG )
DEFINE_FIELD( m_bIsScoped, FIELD_BOOLEAN ),
DEFINE_FIELD( m_flScopeTime, FIELD_TIME ),
DEFINE_FIELD( m_flLastShot, FIELD_TIME ),
END_DATADESC()

LINK_ENTITY_TO_CLASS(weapon_hmg, CWeaponHMG);
PRECACHE_WEAPON_REGISTER(weapon_hmg);

acttable_t	CWeaponHMG::m_acttable[] = {
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

IMPLEMENT_ACTTABLE(CWeaponHMG);

CWeaponHMG::CWeaponHMG() {
	m_fMinRange1 = 0; m_fMaxRange1 = 2000;
	m_bAltFiresUnderwater = m_bFiresUnderwater = false;
	m_flLastShot = 0;
	m_iBurstSize = 0;
}

void CWeaponHMG::Precache( void )
{
	PrecacheModel( "models/weapons/hmg/m_hmg.mdl" );

	BaseClass::Precache();
}

float CWeaponHMG::GetFireRate( void )
{
	return GetRTBRWpnData().m_flFireRate;
}

float CWeaponHMG::GetBurstCycleRate( void )
{
	return sk_hmg_burst_time.GetFloat();
}

int CWeaponHMG::GetBurstSize( void )
{
	return sk_hmg_burst_size.GetInt();
}

float CWeaponHMG::GetBurstSpeed( void )
{
	return sk_hmg_burst_speed.GetFloat();
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
	m_iBurstSize = GetBurstSize();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetBurstCycleRate() * 1.5f;
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
	if (pPlayer->m_nButtons & IN_ATTACK && m_iBurstSize == 0){
		pPlayer->m_nButtons &= ~IN_ATTACK2; // no weird m1+m2 stuff
	}
	else if (m_iBurstSize > 0){
		pPlayer->m_nButtons &= ~IN_ATTACK; // no weird m1+m2 stuff
		if (gpGlobals->curtime >= m_flLastShot + (GetBurstCycleRate() / GetBurstSpeed())){
			// why don't we use a think function here? it's because the gods at valve decided that thinking keeps
			// the random number generator context the same, or deterministic (my theory, anyway), and it eventually stales out, 
			// meaning that for shots 2+, they all land in the exact same group because we have the exact same seed.
			// not thinking here means that the rng never stales out and as such we get a different shot direction each time
			// however this doesn't explain shotgun spread so idfk
			m_flNextPrimaryAttack = gpGlobals->curtime - (GetBurstCycleRate() / GetBurstSpeed());
			BaseClass::PrimaryAttack();
			m_flLastShot = gpGlobals->curtime;
			m_iBurstSize--;
			if (m_iBurstSize == 0){
				m_flNextPrimaryAttack = m_flNextSecondaryAttack; // cooldown so we can't hold m1 straight after m2
			}
		}
	}

	if (pPlayer->m_nButtons & IN_ATTACK3)
	{
		if (m_flScopeTime <= gpGlobals->curtime)
		{
			ToggleScope();
			pPlayer->m_nButtons &= ~IN_ATTACK3;
		}
	}

	BaseClass::ItemPostFrame();
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
		cone = VECTOR_CONE_4DEGREES;
	}
	else {
		cone = VECTOR_CONE_10DEGREES;
	}
	return cone;
}

int CWeaponHMG::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	return (flDot > 0.55f) ? COND_CAN_RANGE_ATTACK1 : COND_NOT_FACING_ATTACK;
}

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
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1, entindex(), 0 );

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

const WeaponProficiencyInfo_t* CWeaponHMG::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0, 0.75 },
		{ 5.00, 0.75 },
		{ 3.0, 0.85 },
		{ 5.0 / 3.0, 0.75 },
		{ 1.00, 1.0 },
	};

	COMPILE_TIME_ASSERT(ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);
	return proficiencyTable;
}

void CWeaponHMG::Scope( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	pPlayer->SetFOV( this, pPlayer->GetFOV() / 1.5f, 0.1f ); // 1.5x zoom
	UTIL_ScreenFade( pPlayer, { 0, 0, 0, 255 }, 0.2, 0, FFADE_IN );
	pPlayer->ShowViewModel( false );
	WeaponSound( SPECIAL1 );
	m_bIsScoped = true;
}

void CWeaponHMG::Unscope( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (pPlayer->SetFOV( this, 0 ))
	{
		pPlayer->ShowViewModel( true );

		// Zoom out to the default zoom level
		WeaponSound( SPECIAL2 );
		m_bIsScoped = false;

	}
}

bool CWeaponHMG::ShouldWeaponFidget()
{
	return !m_bIsScoped;
}