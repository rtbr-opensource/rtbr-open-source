//========= Copyright (c) RTBR Team, 2018 ============//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "npcevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "in_buttons.h"
#include "ai_memory.h"
#include "grenade_ar2.h"
#include "soundent.h"
#include "rumble_shared.h"
#include "gamestats.h"
#include "steam/isteamuser.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define OICW_ACCURACY_RESET_TIME 0.75f
#define OICW_MIN_CROSSHAIR_SCALE 0.5f
#define OICW_MED_CROSSHAIR_SCALE1 0.65f
#define OICW_MED_CROSSHAIR_SCALE2 0.8f
#define OICW_MAX_CROSSHAIR_SCALE 1.0f
#define OICW_MIN_SCALE_TIME 0.15f
#define OICW_NORM_SCALE_TIME 0.05f

extern ConVar    sk_plr_dmg_smg1_grenade;

class CWeaponOICW : public CHLSelectFireMachineGun {
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponOICW, CHLSelectFireMachineGun);
	DECLARE_SERVERCLASS();
	CWeaponOICW();

	void Precache(void);
	void ItemPostFrame(void);

	bool Holster(CBaseCombatWeapon * pSwitchingTo);
	void OnPickedUp(CBaseCombatCharacter *pNewOwner);

	void FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, Vector& vecShootOrigin, Vector& vecShootDir);
	void FireNPCSecondaryAttack(CBaseCombatCharacter* pOperator, Vector& vecShootOrigin, Vector& vecShootDir);
	void Operator_ForceNPCFire(CBaseCombatCharacter  *pOperator, bool bSecondary) OVERRIDE;

	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator) OVERRIDE;

	int CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	void PrimaryAttack();
	void SecondaryAttack(void);

	bool Reload();
	bool SecondaryReload(void);
	void ReloadGrenades(void) { SecondaryReload(); };

	void AddViewKick(void);

	float GetFireRate(void);

	void ToggleScope( void );
	void Scope(void);
	void Unscope( void );

	virtual bool ShouldDisplayAltFireHUDHint();

	Vector GetWeaponInaccuracy( void );

	bool ShouldWeaponFidget(void);
	bool ShouldWeaponAutoAim( void );

	void SetDesiredCrosshairScale( float scale, float deltaTime );
	void UpdateCrosshairScale( void );

	float	m_flScopeTime;
	bool m_bInSecondaryReload;

	bool m_bSecondaryReloadHintDisplayed;

	float	m_flLastShot;
	int m_iShotsFired;
	float m_flDesiredCrosshairScale;
	float m_flOldCrosshairScale;
	float m_flCrosshairScaleStartTime;
	float m_flCrosshairScaleEndTime;

	CNetworkVar(bool, m_bIsPlayer);

	CNetworkVar(bool, m_bIsScoped);

	virtual const Vector& GetBulletSpread(void) {
		static const Vector cone = VECTOR_CONE_3DEGREES;
		static Vector coneScoped;
		if (m_bIsScoped)
			coneScoped = GetWeaponInaccuracy();
		return m_bIsScoped ? coneScoped : cone;
	}

	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponOICW, DT_WeaponOICW)
SendPropBool(SENDINFO(m_bIsScoped)),
SendPropBool(SENDINFO(m_bIsPlayer)),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_oicw, CWeaponOICW);
PRECACHE_WEAPON_REGISTER(weapon_oicw);

BEGIN_DATADESC(CWeaponOICW)
DEFINE_FIELD(m_bIsScoped, FIELD_BOOLEAN),
DEFINE_FIELD(m_flScopeTime, FIELD_TIME),
DEFINE_FIELD(m_bInSecondaryReload, FIELD_BOOLEAN),
DEFINE_FIELD(m_bSecondaryReloadHintDisplayed, FIELD_BOOLEAN),
DEFINE_FIELD( m_flLastShot, FIELD_TIME ),
DEFINE_FIELD( m_iShotsFired, FIELD_INTEGER ),
DEFINE_FIELD( m_flDesiredCrosshairScale, FIELD_FLOAT ),
DEFINE_FIELD( m_flOldCrosshairScale, FIELD_FLOAT ),
DEFINE_FIELD( m_flCrosshairScaleStartTime, FIELD_TIME ),
DEFINE_FIELD( m_flCrosshairScaleEndTime, FIELD_TIME ),
END_DATADESC()

acttable_t	CWeaponOICW::m_acttable[] = {
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

IMPLEMENT_ACTTABLE(CWeaponOICW);
//-----------------------------------------------------------------------------
// Purpose: Precaches weapon
//-----------------------------------------------------------------------------
void CWeaponOICW::Precache(void)
{
	PrecacheModel( "models/weapons/OICW/m_OICW.mdl" );
	PrecacheModel( "models/weapons/OICW/m_oicw_Grenade.mdl" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Makes sure first time pickup anim isn't overridden
//-----------------------------------------------------------------------------
void CWeaponOICW::OnPickedUp(CBaseCombatCharacter *pNewOwner)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer != NULL && pPlayer->GetAmmoCount(m_iSecondaryAmmoType) > 0) {
		if (pPlayer->GetAmmoCount(m_iSecondaryAmmoType) < GetMaxClip2()) {
			m_iClip2 = pPlayer->GetAmmoCount(m_iSecondaryAmmoType);
		}
		else {
			m_iClip2 = GetMaxClip2();
		}
	}

	BaseClass::OnPickedUp(pNewOwner);
}

//-----------------------------------------------------------------------------
// Purpose: Deals with secondary reload and teritary attack
//-----------------------------------------------------------------------------
void CWeaponOICW::ItemPostFrame(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	m_bIsPlayer = GetOwner()->IsPlayer();
	
	if (!m_bSecondaryReloadHintDisplayed && Clip2() < GetMaxClip2() && pPlayer->GetAmmoCount(m_iSecondaryAmmoType) > 0)
	{
		UTIL_HudHintText( GetOwner(), "#RTBR_OICW_SecondaryReload" );
		m_bSecondaryReloadHintDisplayed = true;
	}

	if (pPlayer->m_nButtons & IN_ALT1 && !m_bInSecondaryReload)
	{
		SecondaryReload();
	}

	if (gpGlobals->curtime > m_flLastShot + OICW_ACCURACY_RESET_TIME) {
		m_iShotsFired = 0;
		if (m_bIsScoped && pPlayer->m_Local.m_flCrosshairScale == m_flDesiredCrosshairScale && m_flDesiredCrosshairScale != 0.5f)
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
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (pPlayer) {
		if (m_bIsScoped)
		{
			Unscope();
		}
		SetDesiredCrosshairScale( OICW_MAX_CROSSHAIR_SCALE, 0.0f );
	}

	m_bInSecondaryReload = false;

	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Set some OICW variables
//-----------------------------------------------------------------------------
CWeaponOICW::CWeaponOICW() {
	m_fMinRange1 = 0; m_fMaxRange1 = 2000;
	m_bAltFiresUnderwater = m_bFiresUnderwater = false;
	m_bInSecondaryReload = false;
	m_bSecondaryReloadHintDisplayed = false;
}

//-----------------------------------------------------------------------------
// Purpose: Set OICW fire rate
//-----------------------------------------------------------------------------
float CWeaponOICW::GetFireRate(void)
{
	if (!GetOwner()->IsNPC()) {
		if (m_bIsScoped) {
			return GetRTBRWpnData().m_flFireRateScoped;
		}
		else {
			return GetRTBRWpnData().m_flFireRate;
		}
	}
	else {
		return GetRTBRWpnData().m_flFireRate;
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
	if (m_bInSecondaryReload)
		return false;

	bool reloadBool;
	
	// Allow the player to reload both magazines using +reload.
	if (Clip1() < GetMaxClip1() || Clip2() < GetMaxClip2())
	{
		// First check for a primary reload.
		reloadBool = BaseClass::Reload();
		if (!reloadBool) // if that fails, try to reload the secondary mag
			reloadBool = SecondaryReload();

		// if either mag can be reloaded, unscope
		if (GetOwner()->IsPlayer()) {
			if (m_bIsScoped && reloadBool) {
				Unscope();
			}
		}
	}
	else
	{
		// Then do our normal fidgeting.
		reloadBool = BaseClass::Reload();
	}

	return reloadBool;
}

//-----------------------------------------------------------------------------
// Purpose: Secondary reload function
//-----------------------------------------------------------------------------
bool CWeaponOICW::SecondaryReload(void) {
	if (m_bInSecondaryReload || m_bInReload)
		return false;

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer == NULL || m_iClip2 == GetMaxClip2() || pPlayer->GetAmmoCount(m_iSecondaryAmmoType) == 0 || pPlayer->GetActiveWeapon() != this) {
		return false;
	}

	if (GetOwner()->IsPlayer()) {
		if (m_bIsScoped) {
			Unscope();
		}
	}

	SendWeaponAnim(ACT_VM_SECONDARYRELOAD);
	m_bInSecondaryReload = true;
	return true;
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
	if ( ( pPlayer->GetWaterLevel() == 3 ) || Clip2() == 0 )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		BaseClass::WeaponSound( EMPTY );
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		SecondaryReload();
		return;
	}

	if( m_bInReload )
		m_bInReload = false;
	
	// MUST call sound before removing a round from the clip of a CMachineGun
	BaseClass::WeaponSound( WPN_DOUBLE );
	
	pPlayer->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAGS_NONE );
	
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecThrow;
	// Don't autoaim on grenade tosses
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow );
	VectorScale( vecThrow, 1000.0f, vecThrow );
	
	//Create the grenade
	QAngle angles;
	VectorAngles( vecThrow, angles );
	CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create( "grenade_ar2", vecSrc, angles, pPlayer );
	pGrenade->SetAbsVelocity( vecThrow );
	
	pGrenade->SetLocalAngularVelocity( RandomAngle( -400, 400 ) );
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE ); 
	pGrenade->SetThrower( GetOwner() );
	pGrenade->SetDamage( sk_plr_dmg_smg1_grenade.GetFloat() );
	
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 1000, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON );
	
	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	
	// Decrease clip
	m_iClip2--;
	
	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
	
	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
	
	// Register a muzzleflash for the AI.
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );	
	
	m_iSecondaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, false, GetClassname() );

	if (Clip2() < 1) {
		SetContextThink(&CWeaponOICW::ReloadGrenades, gpGlobals->curtime + 0.5f, "ReloadGrenades");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Scope and de-scope the weapon
//-----------------------------------------------------------------------------
void CWeaponOICW::ToggleScope(void) {
	if (GetOwner()->IsPlayer()) {
		if (GetActivity() == ACT_VM_SECONDARYRELOAD || GetActivity() == ACT_VM_RELOAD || GetOwner() != UTIL_GetLocalPlayer())
			return;

		if (m_flScopeTime >= gpGlobals->curtime) {
			return;
		}
		CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
		if (!pPlayer)
			return;

		if (m_bIsScoped)
			Unscope();
		else
			Scope();

		m_flScopeTime = gpGlobals->curtime + 0.4f;
	}
}

void CWeaponOICW::FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir)
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());
	pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1, entindex(), 0);

	pOperator->DoMuzzleFlash();
	m_iClip1--;
}

void CWeaponOICW::FireNPCSecondaryAttack(CBaseCombatCharacter* pOperator, Vector& vecShootOrigin, Vector& vecShootDir)
{
	Msg("OICW NPC Secondary attack triggered\n");

	// MUST call sound before removing a round from the clip of a CMachineGun
	BaseClass::WeaponSound(WPN_DOUBLE);

	//Create the grenade
	QAngle angles;
	//VectorAngles(vecShootDir, angles);
	CGrenadeAR2* pGrenade = (CGrenadeAR2*)Create("grenade_ar2", vecShootOrigin, angles, GetOwner());
	pGrenade->SetAbsVelocity(vecShootDir);

	pGrenade->SetLocalAngularVelocity(RandomAngle(-400, 400));
	pGrenade->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
	pGrenade->SetThrower(GetOwner());
	pGrenade->SetDamage(sk_plr_dmg_smg1_grenade.GetFloat());

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 1000, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON);

	// Decrease clip
	m_iClip2--;

	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;

	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());

	pOperator->DoMuzzleFlash();
}

void CWeaponOICW::Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary)
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
	AngleVectors(angShootDir, &vecShootDir);
	if (bSecondary)
	{
		FireNPCSecondaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}
	else
	{
		FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}
}

void CWeaponOICW::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{

	switch (pEvent->event)
	{
	case EVENT_WEAPON_SMG1:
	case EVENT_WEAPON_AR2_ALTFIRE:
	case EVENT_WEAPON_PISTOL_FIRE:	// NOTE: This is actually for the OICW.
	{
		Vector vecShootOrigin, vecShootDir, vecShootPos;
		QAngle angDiscard;

		// Support old style attachment point firing
		if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
		{
			vecShootOrigin = pOperator->Weapon_ShootPosition();
		}

		CAI_BaseNPC *npc = pOperator->MyNPCPointer();
		ASSERT(npc != NULL);

		if (pEvent->event == EVENT_WEAPON_SMG1 || pEvent->event == EVENT_WEAPON_PISTOL_FIRE)
		{
			vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);
			FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
		}
		else if (pEvent->event == EVENT_WEAPON_AR2_ALTFIRE)
		{
			vecShootPos = npc->GetAltFireTarget();
			vecShootDir = VecCheckThrow(pOperator, vecShootOrigin, vecShootPos, 1000.0f, 0.75f);
			FireNPCSecondaryAttack(pOperator, vecShootOrigin, vecShootDir);
		}
		break;
	}
	case AE_WPN_RELOAD:
	{
		// reload frames handling for secondary reload
		if (m_bInSecondaryReload)
		{
			FinishReloadSecondary();
			m_bInSecondaryReload = false;
		}
		else
		{
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator ); // defer to base class for any other reloads
		}
		break;
	}

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}

void CWeaponOICW::Scope( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (pPlayer->SetFOV( this, 30, 0.2f ))
	{
		UTIL_ScreenFade( pPlayer, { 0, 0, 0, 255 }, 0.4, 0, FFADE_IN );
		if (!m_bIsScoped)
		{
			pPlayer->ShowViewModel( false );
		}

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
		pPlayer->ShowViewModel( true );

		// Zoom out to the default zoom level
		WeaponSound( SPECIAL2 );
		m_bIsScoped = false;
		SetDesiredCrosshairScale( OICW_MAX_CROSSHAIR_SCALE, 0.0f );
	}
}

bool CWeaponOICW::ShouldDisplayAltFireHUDHint()
{
	if (GetHudHintCount() >= WEAPON_ALTFIRE_HUD_HINT_COUNT)
		return false;

	if (HasPrimaryAmmo())
	{
		return true;
	}

	return false;
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

bool CWeaponOICW::ShouldWeaponAutoAim()
{
	return !m_bIsScoped;
}

void CWeaponOICW::SetDesiredCrosshairScale( float scale, float deltaTime )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (deltaTime <= 0)
	{
		pPlayer->m_Local.m_flCrosshairScale = m_flDesiredCrosshairScale = scale;
		m_flCrosshairScaleStartTime = m_flCrosshairScaleEndTime = gpGlobals->curtime;
	}
	else
	{
		if (pPlayer->m_Local.m_flCrosshairScale != m_flDesiredCrosshairScale)
			pPlayer->m_Local.m_flCrosshairScale = m_flDesiredCrosshairScale;
		m_flOldCrosshairScale = pPlayer->m_Local.m_flCrosshairScale;
		m_flDesiredCrosshairScale = scale;
		m_flCrosshairScaleStartTime = gpGlobals->curtime;
		m_flCrosshairScaleEndTime = gpGlobals->curtime + deltaTime;
	}
}

void CWeaponOICW::UpdateCrosshairScale( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (pPlayer->m_Local.m_flCrosshairScale == m_flDesiredCrosshairScale)
		return;

	pPlayer->m_Local.m_flCrosshairScale = RemapValClamped(	gpGlobals->curtime,
															m_flCrosshairScaleStartTime,
															m_flCrosshairScaleEndTime,
															m_flOldCrosshairScale,
															m_flDesiredCrosshairScale );
}

/*//========= Copyright (c) RTBR Team, 2018 ============//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "npcevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "game.h"
#include "in_buttons.h"
#include "grenade_ar2.h"
#include "ai_memory.h"
#include "soundent.h"
#include "rumble_shared.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar    sk_plr_dmg_smg1_grenade;


class CWeaponOICW : public CHLSelectFireMachineGun {
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponOICW, CHLSelectFireMachineGun);
	DECLARE_SERVERCLASS();
	CWeaponOICW();
	
	//First time Pickup Anim function
	void	OnPickedUp(CBaseCombatCharacter *pNewOwner);

	void Precache( void );

	void ItemPostFrame( void );
	float	GetFireRate( void );
	void	AddViewKick( void );

	virtual void SecondaryAttack(void);
	void Scope( void );
	bool Reload( void );

	bool Holster( CBaseCombatWeapon * pSwitchingTo );

	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int	WeaponRangeAttack2Condition( float flDot, float flDist );
	Activity GetPrimaryAttackActivity( void );

	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary ) OVERRIDE;
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator ) OVERRIDE;

	virtual const Vector& GetBulletSpread(void) {
		static const Vector cone = VECTOR_CONE_5DEGREES;
		static const Vector coneScoped = VECTOR_CONE_1DEGREES;
		return m_nScopeLevel > 0 ? coneScoped : cone;
	}

	DECLARE_ACTTABLE();

protected:
	// SMG1 Grenade Tertiary Fire
	Vector	m_vecTossVelocity;
	float	m_flNextGrenadeCheck;
	// Scope Secondary Fire
	float m_fNextScope;
	int m_nScopeLevel;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponOICW, DT_WeaponOICW)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_oicw, CWeaponOICW);
PRECACHE_WEAPON_REGISTER(weapon_oicw);
BEGIN_DATADESC( CWeaponOICW )
	DEFINE_FIELD( m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( m_fNextScope, FIELD_FLOAT ),
	DEFINE_FIELD( m_nScopeLevel, FIELD_INTEGER ),
END_DATADESC()

acttable_t	CWeaponOICW::m_acttable[] = {
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

IMPLEMENT_ACTTABLE(CWeaponOICW);

//-----------------------------------------------------------------------------
// First pickup anim experiment from Iridium
//-----------------------------------------------------------------------------

void CWeaponOICW::OnPickedUp(CBaseCombatCharacter *pNewOwner)
{
	BaseClass::OnPickedUp(pNewOwner);
	//SendWeaponAnim(ACT_VM_FIRSTDRAW);
}


//-----------------------------------------------------------------------------
// Discrete zoom levels for the OICW.
//-----------------------------------------------------------------------------
static int g_nOICWScopeFOV[] =
{
	20,
};

CWeaponOICW::CWeaponOICW() {
	m_fMinRange1 = 0; m_fMaxRange1 = 2000;
	m_bAltFiresUnderwater = m_bFiresUnderwater = false;
}

float CWeaponOICW::GetFireRate( void )
{
	return m_nScopeLevel > 0 ? GetRTBRWpnData().m_flFireRateScoped : GetRTBRWpnData().m_flFireRate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponOICW::Precache( void )
{
	PrecacheModel("models/weapons/oicw/m_oicw.mdl");
	UTIL_PrecacheOther("grenade_ar2");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponOICW::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	1.0f	//Degrees
	#define	SLIDE_LIMIT			2.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

//-----------------------------------------------------------------------------
// Purpose: Overloaded to handle the zoom functionality.
//-----------------------------------------------------------------------------
void CWeaponOICW::ItemPostFrame( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (pPlayer == NULL)
	{
		return;
	}

	if ((m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		FinishReload();
		m_bInReload = false;
	}

	bool bFired = false;

	// Secondary fire zooms in the OICW
	// This is the top priority
	if (pPlayer->m_nButtons & IN_ATTACK2)
	{
		if (m_fNextScope <= gpGlobals->curtime)
		{
			Scope();
			pPlayer->m_nButtons &= ~IN_ATTACK2;
		}
	}
	// Tertiary fire shoots grenades
	else if ((pPlayer->m_nButtons & IN_ATTACK3) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		if (UsesSecondaryAmmo() && pPlayer->GetAmmoCount( m_iSecondaryAmmoType )<=0)
		{
			if (m_flNextEmptySoundTime < gpGlobals->curtime)
			{
				WeaponSound( EMPTY );
				m_flNextSecondaryAttack = m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
			}
		}
		else if (pPlayer->GetWaterLevel() == 3 && m_bAltFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			bFired = ShouldBlockPrimaryFire();

			SecondaryAttack();

			// Secondary ammo doesn't have a reload animation
			if (UsesClipsForAmmo2())
			{
				// reload clip2 if empty
				if (m_iClip2 < 1)
				{
					pPlayer->RemoveAmmo( 1, m_iSecondaryAmmoType );
					m_iClip2 = m_iClip2 + 1;
				}
			}
		}
	}
	if (!bFired && (pPlayer->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		// Clip empty? Or out of ammo on a no-clip weapon?
		if (((UsesClipsForAmmo1() && m_iClip1 <= 0) || (!UsesClipsForAmmo1() && pPlayer->GetAmmoCount( m_iPrimaryAmmoType )<=0)))
		{
			HandleFireOnEmpty();
		}
		else if (pPlayer->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			//NOTENOTE: There is a bug with this code with regards to the way machine guns catch the leading edge trigger
			//			on the player hitting the attack key.  It relies on the gun catching that case in the same frame.
			//			However, because the player can also be doing a secondary attack, the edge trigger may be missed.
			//			We really need to hold onto the edge trigger and only clear the condition when the gun has fired its
			//			first shot.  Right now that's too much of an architecture change -- jdw

			// If the firing button was just pressed, or the alt-fire just released, reset the firing time
			if ((pPlayer->m_afButtonPressed & IN_ATTACK) || (pPlayer->m_afButtonReleased & IN_ATTACK2))
			{
				m_flNextPrimaryAttack = gpGlobals->curtime;
			}

			PrimaryAttack();

			if (AutoFiresFullClip())
			{
				m_bFiringWholeClip = true;
			}

#ifdef CLIENT_DLL
			pOwner->SetFiredWeapon( true );
#endif
		}
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	// -----------------------
	if (pPlayer->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload)
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}

	// -----------------------
	//  No buttons down
	// -----------------------
	if (!((pPlayer->m_nButtons & IN_ATTACK) || (pPlayer->m_nButtons & IN_ATTACK2) || (pPlayer->m_nButtons & IN_RELOAD)))
	{
		// no fire buttons down
		m_bFireOnEmpty = false;

		if (!HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime)
		{
			// weapon isn't useable, switch.
			if (!(GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && pPlayer->SwitchToNextBestWeapon( this ))
			{
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
				return;
			}
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if (m_iClip1 == 0 && !(GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < gpGlobals->curtime)
			{
				Reload();
				return;
			}
		}

		WeaponIdle();
		return;
	}
}

void CWeaponOICW::SecondaryAttack(void) 
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	//Must have ammo
	if ( ( pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) <= 0 ) || ( pPlayer->GetWaterLevel() == 3 ) )
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

	pPlayer->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAGS_NONE );

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecThrow;
	// Don't autoaim on grenade tosses
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow );
	VectorScale( vecThrow, 1000.0f, vecThrow );
	
	//Create the grenade
	QAngle angles;
	VectorAngles( vecThrow, angles );
	CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create( "grenade_ar2", vecSrc, angles, pPlayer );
	pGrenade->SetAbsVelocity( vecThrow );

	pGrenade->SetLocalAngularVelocity( RandomAngle( -400, 400 ) );
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE ); 
	pGrenade->SetThrower( GetOwner() );
	pGrenade->SetDamage( sk_plr_dmg_smg1_grenade.GetFloat() );

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 1000, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Decrease ammo
	pPlayer->RemoveAmmo( 1, m_iSecondaryAmmoType );

	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;

	// Register a muzzleflash for the AI.
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );	

	m_iSecondaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, false, GetClassname() );
}

#define OICW_SCOPE_RATE					0.2			// Interval between zoom levels in seconds.
//-----------------------------------------------------------------------------
// Purpose: Zooms in using the sniper rifle scope.
//-----------------------------------------------------------------------------
void CWeaponOICW::Scope( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
	{
		return;
	}

	if (m_nScopeLevel >= sizeof( g_nOICWScopeFOV ) / sizeof( g_nOICWScopeFOV[0] ))
	{
		if (pPlayer->SetFOV( this, 0 ))
		{
			pPlayer->ShowViewModel( true );

			// Zoom out to the default zoom level
			WeaponSound( SPECIAL2 );
			m_nScopeLevel = 0;
		}
	}
	else
	{
		if (pPlayer->SetFOV( this, g_nOICWScopeFOV[m_nScopeLevel] ))
		{
			if (m_nScopeLevel == 0)
			{
				pPlayer->ShowViewModel( false );
			}

			WeaponSound( SPECIAL1 );

			m_nScopeLevel++;
		}
	}

	m_fNextScope = gpGlobals->curtime + OICW_SCOPE_RATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponOICW::Reload( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (pPlayer != NULL && m_nScopeLevel >= sizeof( g_nOICWScopeFOV ) / sizeof( g_nOICWScopeFOV[0] ))
	{
		if (pPlayer->SetFOV( this, 0 ))
		{
			pPlayer->ShowViewModel( true );

			// Zoom out to the default zoom level
			WeaponSound( SPECIAL2 );
			m_nScopeLevel = 0;
		}
		m_fNextScope = gpGlobals->curtime + OICW_SCOPE_RATE;
	}
	return BaseClass::Reload();
}

//-----------------------------------------------------------------------------
// Purpose: Turns off the zoom when the rifle is holstered.
//-----------------------------------------------------------------------------
bool CWeaponOICW::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (pPlayer != NULL)
	{
		if (m_nScopeLevel != 0)
		{
			if (pPlayer->SetFOV( this, 0 ))
			{
				pPlayer->ShowViewModel( true );
				m_nScopeLevel = 0;
			}
		}
	}
	return BaseClass::Holster( pSwitchingTo );
}

int CWeaponOICW::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	return COND_NONE;
}

Activity CWeaponOICW::GetPrimaryAttackActivity( void )
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_RECOIL1;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

void CWeaponOICW::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );
	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), 0 );

	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;
}

void CWeaponOICW::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
}

void CWeaponOICW::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_SMG1:
		{
			Vector vecShootOrigin, vecShootDir;
			QAngle angDiscard;

			// Support old style attachment point firing
			if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
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
}*/