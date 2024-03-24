//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements a sniper rifle weapon.
//			
//			Primary attack: fires a single high-powered shot, then reloads.
//			Secondary attack: cycles sniper scope through zoom levels.
//
// TODO: Circular mask around crosshairs when zoomed in.
// TODO: Shell ejection.
// TODO: Finalize kickback.
// TODO: Animated zoom effect?
//
//=============================================================================//

#include "cbase.h"
#include "NPCEvent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "gamerules.h"				// For g_pGameRules
#include "in_buttons.h"
#include "soundent.h"
#include "vstdlib/random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SNIPER_CONE_PLAYER					vec3_origin	// Spread cone when fired by the player.
#define SNIPER_CONE_NPC						vec3_origin	// Spread cone when fired by NPCs.
#define SNIPER_BULLET_COUNT_PLAYER			1			// Fire n bullets per shot fired by the player.
#define SNIPER_BULLET_COUNT_NPC				1			// Fire n bullets per shot fired by NPCs.
#define SNIPER_TRACER_FREQUENCY_PLAYER		0			// Draw a tracer every nth shot fired by the player.
#define SNIPER_TRACER_FREQUENCY_NPC			0			// Draw a tracer every nth shot fired by NPCs.
#define SNIPER_KICKBACK						45			// Range for punchangle when firing. 3 by default ~TheZealot

#define SNIPER_SCOPE_RATE					0.2			// Interval between zoom levels in seconds.


//-----------------------------------------------------------------------------
// Discrete zoom levels for the scope.
//-----------------------------------------------------------------------------
static int g_nScopeFOV[] =
{
	20,
	5
};

#ifdef MAPBASE
extern acttable_t *GetAR2Acttable();
extern int GetAR2ActtableCount();
#endif

class CWeaponSniperRifle : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS(CWeaponSniperRifle, CBaseHLCombatWeapon);

	CWeaponSniperRifle(void);

	DECLARE_SERVERCLASS();

	void Precache(void);

	int	CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	const Vector &GetBulletSpread(void);

	bool Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	void ItemPostFrame(void);
	void PrimaryAttack(void);
	bool Reload(void);
	void Scope(void);
	virtual float GetFireRate(void) { return 1; };

	void FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles);

	void Equip(CBaseCombatCharacter *pOwner);

	void Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary);

	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

#ifdef MAPBASE
	virtual acttable_t		*GetBackupActivityList() { return GetAR2Acttable(); }
	virtual int				GetBackupActivityListCount() { return GetAR2ActtableCount(); }
#endif

	DECLARE_ACTTABLE();

protected:

	float m_fNextScope;
	int m_nScopeLevel;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponSniperRifle, DT_WeaponSniperRifle)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_sniperrifle, CWeaponSniperRifle);
PRECACHE_WEAPON_REGISTER(weapon_sniperrifle);

BEGIN_DATADESC(CWeaponSniperRifle)

DEFINE_FIELD(m_fNextScope, FIELD_FLOAT),
DEFINE_FIELD(m_nScopeLevel, FIELD_INTEGER),

END_DATADESC()

//-----------------------------------------------------------------------------
// Maps base activities to weapons-specific ones so our characters do the right things.
//-----------------------------------------------------------------------------
acttable_t	CWeaponSniperRifle::m_acttable[] =
{
	{	ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SNIPER_RIFLE, true },

#if EXPANDED_HL2_UNUSED_WEAPON_ACTIVITIES
	// Optional new NPC activities
	// (these should fall back to AR2 animations when they don't exist on an NPC)
	{ ACT_RELOAD,					ACT_RELOAD_SNIPER_RIFLE,			true },
	{ ACT_IDLE,						ACT_IDLE_SNIPER_RIFLE,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SNIPER_RIFLE,		true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SNIPER_RIFLE_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SNIPER_RIFLE_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SNIPER_RIFLE,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_SNIPER_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_SNIPER_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_SNIPER_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_SNIPER_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_SNIPER_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_SNIPER_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SNIPER_RIFLE_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_SNIPER_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SNIPER_RIFLE,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_SNIPER_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_SNIPER_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_SNIPER_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_SNIPER_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_SNIPER_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_SNIPER_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK,						ACT_WALK_SNIPER_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_SNIPER_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,					true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,				true },
	{ ACT_RUN,						ACT_RUN_SNIPER_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_SNIPER_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,					true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,				true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SNIPER_RIFLE,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SNIPER_RIFLE_LOW,		true },
	{ ACT_COVER_LOW,				ACT_COVER_SNIPER_RIFLE_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SNIPER_RIFLE_LOW,			false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SNIPER_RIFLE_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SNIPER_RIFLE,		true },

	{ ACT_ARM,						ACT_ARM_RIFLE,					false },
	{ ACT_DISARM,					ACT_DISARM_RIFLE,				false },

#if EXPANDED_HL2_COVER_ACTIVITIES
	{ ACT_RANGE_AIM_MED,			ACT_RANGE_AIM_SNIPER_RIFLE_MED,			false },
	{ ACT_RANGE_ATTACK1_MED,		ACT_RANGE_ATTACK_SNIPER_RIFLE_MED,		false },
#endif

#if EXPANDED_HL2DM_ACTIVITIES
	// HL2:DM activities (for third-person animations in SP)
	{ ACT_HL2MP_IDLE,                    ACT_HL2MP_IDLE_SNIPER_RIFLE,                    false },
	{ ACT_HL2MP_RUN,                    ACT_HL2MP_RUN_SNIPER_RIFLE,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,            ACT_HL2MP_IDLE_CROUCH_SNIPER_RIFLE,            false },
	{ ACT_HL2MP_WALK_CROUCH,            ACT_HL2MP_WALK_CROUCH_SNIPER_RIFLE,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,    ACT_HL2MP_GESTURE_RANGE_ATTACK_SNIPER_RIFLE,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,            ACT_HL2MP_GESTURE_RELOAD_SNIPER_RIFLE,        false },
	{ ACT_HL2MP_JUMP,                    ACT_HL2MP_JUMP_SNIPER_RIFLE,                    false },
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_SNIPER_RIFLE,					false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_SNIPER_RIFLE,    false },
#endif
#endif
};

IMPLEMENT_ACTTABLE(CWeaponSniperRifle);


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CWeaponSniperRifle::CWeaponSniperRifle(void)
{
	m_fNextScope = gpGlobals->curtime;
	m_nScopeLevel = 0;

	m_bReloadsSingly = true;

	m_fMinRange1 = 0;
	m_fMinRange2 = 0;
	m_fMaxRange1 = 2048;
	m_fMaxRange2 = 2048;
}

//-----------------------------------------------------------------------------
// Purpose: Turns off the zoom when the rifle is holstered.
//-----------------------------------------------------------------------------
bool CWeaponSniperRifle::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer != NULL)
	{
		if (m_nScopeLevel != 0)
		{
			if (pPlayer->SetFOV(this, 0))
			{
				pPlayer->ShowViewModel(true);
				m_nScopeLevel = 0;
			}
		}
	}
	return BaseClass::Holster(pSwitchingTo);
}


//-----------------------------------------------------------------------------
// Purpose: Overloaded to handle the zoom functionality.
//-----------------------------------------------------------------------------
void CWeaponSniperRifle::ItemPostFrame(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer == NULL)
	{
		return;
	}

	if ((m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		FinishReload();
		m_bInReload = false;
	}

	if (pPlayer->m_nButtons & IN_ATTACK2)
	{
		if (m_fNextScope <= gpGlobals->curtime)
		{
			Scope();
			pPlayer->m_nButtons &= ~IN_ATTACK2;
		}
	}
	else if ((pPlayer->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		if ((m_iClip1 == 0 && UsesClipsForAmmo1()) || (!UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType)))
		{
			m_bFireOnEmpty = true;
		}

		// Fire underwater?
		if (pPlayer->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			if (pPlayer && pPlayer->m_afButtonPressed & IN_ATTACK)
			{
				m_flNextPrimaryAttack = gpGlobals->curtime;
			}

			PrimaryAttack();
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
			if (!(GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && pPlayer->SwitchToNextBestWeapon(this))
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


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperRifle::Precache(void)
{
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: Same as base reload but doesn't change the owner's next attack time. This
//			lets us zoom out while reloading. This hack is necessary because our
//			ItemPostFrame is only called when the owner's next attack time has
//			expired.
// Output : Returns true if the weapon was reloaded, false if no more ammo.
//-----------------------------------------------------------------------------
bool CWeaponSniperRifle::Reload(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
	{
		return false;
	}

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
	{
		int primary = min(GetMaxClip1() - m_iClip1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));
		int secondary = min(GetMaxClip2() - m_iClip2, pOwner->GetAmmoCount(m_iSecondaryAmmoType));

		if (primary > 0 || secondary > 0)
		{
			CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
			if (!pPlayer)
			{
				return false;
			}
			if (pPlayer->SetFOV(this, 0))
			{
				pPlayer->ShowViewModel(true);
				m_nScopeLevel = 0;
			}
			// Play reload on different channel as it happens after every fire
			// and otherwise steals channel away from fire sound
			WeaponSound(RELOAD);
			SendWeaponAnim(ACT_VM_RELOAD);
			m_fNextScope = gpGlobals->curtime + SequenceDuration();

			m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

			m_bInReload = true;
		}

		return true;
	}

	return true;
}

void CWeaponSniperRifle::Equip(CBaseCombatCharacter *pOwner)
{
	if (pOwner->Classify() == CLASS_PLAYER_ALLY)
	{
		m_fMaxRange1 = 3000;
	}
	else
	{
		m_fMaxRange1 = 3000;
	}
	BaseClass::Equip(pOwner);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperRifle::PrimaryAttack(void)
{
	// Only the player fires this way so we can cast safely.
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
	{
		return;
	}

	if (gpGlobals->curtime >= m_flNextPrimaryAttack)
	{
		// If my clip is empty (and I use clips) start reload
		if (!m_iClip1)
		{
			Reload();
			return;
		}

		// MUST call sound before removing a round from the clip of a CMachineGun dvs: does this apply to the sniper rifle? I don't know.
		WeaponSound(SINGLE);

		pPlayer->DoMuzzleFlash();

		SendWeaponAnim(ACT_VM_PRIMARYATTACK);

		// player "shoot" animation
		pPlayer->SetAnimation(PLAYER_ATTACK1);

		// Don't fire again until fire animation has completed
		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
		m_iClip1 = m_iClip1 - 1;

		Vector vecSrc = pPlayer->Weapon_ShootPosition();
		Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

		// Fire the bullets
		pPlayer->FireBullets(SNIPER_BULLET_COUNT_PLAYER, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, SNIPER_TRACER_FREQUENCY_PLAYER);

		CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 600, 0.2);

		QAngle vecPunch(random->RandomFloat(-SNIPER_KICKBACK, SNIPER_KICKBACK), 0, 0);
		pPlayer->ViewPunch(vecPunch);

		// Indicate out of ammo condition if we run out of ammo.
		if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		{
			pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
		}
	}

	// Register a muzzleflash for the AI.
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
}

void CWeaponSniperRifle::FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles)
{
	Vector vecShootOrigin, vecShootDir;

	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT(npc != NULL);

	if (bUseWeaponAngles)
	{
		QAngle	angShootDir;
		GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
		AngleVectors(angShootDir, &vecShootDir);
	}
	else
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);
	}


	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());

	pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2);

	// NOTENOTE: This is overriden on the client-side
	// pOperator->DoMuzzleFlash();

	m_iClip1 = m_iClip1 - 1;

}
void CWeaponSniperRifle::Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary)
{
	//// Ensure we have enough rounds in the clip
	//m_iClip1++;
	//DevMsg("Operator_ForceNPCFire\n");
	//Vector vecShootOrigin, vecShootDir;
	//QAngle	angShootDir;
	//GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
	//AngleVectors(angShootDir, &vecShootDir);

	//FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
}

void CWeaponSniperRifle::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_AR2:
	{
							 Vector vecShootOrigin, vecShootDir;
							 vecShootOrigin = pOperator->Weapon_ShootPosition();

							 CAI_BaseNPC *npc = pOperator->MyNPCPointer();
							 ASSERT(npc != NULL);
							 vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);

							 WeaponSound(SINGLE_NPC);
							 pOperator->FireBullets(3, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2);
							 pOperator->DoMuzzleFlash();
							 m_iClip1 = m_iClip1 - 1;
	}
		break;
	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}



//-----------------------------------------------------------------------------
// Purpose: Zooms in using the sniper rifle scope.
//-----------------------------------------------------------------------------
void CWeaponSniperRifle::Scope(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
	{
		return;
	}

	if (m_nScopeLevel >= sizeof(g_nScopeFOV) / sizeof(g_nScopeFOV[0]))
	{
		if (pPlayer->SetFOV(this, 0))
		{
			pPlayer->ShowViewModel(true);

			// Zoom out to the default zoom level
			WeaponSound(SPECIAL2);
			m_nScopeLevel = 0;
		}
	}
	else
	{
		if (pPlayer->SetFOV(this, g_nScopeFOV[m_nScopeLevel]))
		{
			if (m_nScopeLevel == 0)
			{
				pPlayer->ShowViewModel(false);
			}

			WeaponSound(SPECIAL1);

			m_nScopeLevel++;
		}
	}

	m_fNextScope = gpGlobals->curtime + SNIPER_SCOPE_RATE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : virtual const Vector&
//-----------------------------------------------------------------------------
const Vector &CWeaponSniperRifle::GetBulletSpread(void)
{
	static Vector cone = SNIPER_CONE_PLAYER;
	return cone;
}
