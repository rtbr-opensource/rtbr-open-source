//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "player.h"
#include "soundent.h"
#include "te_particlesystem.h"
#include "ndebugoverlay.h"
#include "in_buttons.h"
#include "ai_basenpc.h"
#include "ai_memory.h"
#include "particle_parse.h"
#include "decals.h"
#include "props.h"
#include "baseentity.h"
#include "beam_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_BURN_RADIUS		256
#define RADIUS_GROW_RATE	50.0	// units/sec 

#define IMMOLATOR_TARGET_INVALID Vector( FLT_MAX, FLT_MAX, FLT_MAX )

ConVar sk_plr_dmg_immolator("sk_plr_dmg_immolator", "3", FCVAR_REPLICATED);
ConVar sk_immolator_rtbr_plasma_stream("sk_immolator_rtbr_plasma_stream", "1", FCVAR_REPLICATED);
ConVar sk_immolator_rtbr_burn_seconds("sk_immolator_rtbr_burn_seconds", "3");

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


class CWeaponImmolator : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponImmolator, CBaseHLCombatWeapon);

	DECLARE_SERVERCLASS();

	CWeaponImmolator(void);



	void Precache(void);
	void PrimaryAttack(void);
	void ItemPostFrame(void);
	bool Holster(CBaseCombatWeapon * pSwitchingTo);
	int CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	void ImmolationDamage(const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore);
	bool ImmolateEntity(const CTakeDamageInfo &info, CBaseCombatCharacter *pBCC, int iClassIgnore); // 1upD - Do damage to one entity in particular
	virtual bool WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions);
	virtual int	WeaponRangeAttack1Condition(float flDot, float flDist);

	void	OnPickedUp(CBaseCombatCharacter *pNewOwner);

	void Update();
	void UpdateThink();

	void StartImmolating();
	void StopImmolating();
	void StopImmolatingSilent();
	bool IsImmolating() { return m_flBurnRadius != 0.0; }

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();

	int	m_beamIndex;

	float m_flBurnRadius;
	float m_flTimeLastUpdatedRadius;

	Vector  m_vecImmolatorTarget;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponImmolator, DT_WeaponImmolator)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(info_target_immolator, CPointEntity);
LINK_ENTITY_TO_CLASS(weapon_immolator, CWeaponImmolator);
PRECACHE_WEAPON_REGISTER(weapon_immolator);

BEGIN_DATADESC(CWeaponImmolator)

DEFINE_FIELD(m_beamIndex, FIELD_INTEGER),
DEFINE_FIELD(m_flBurnRadius, FIELD_FLOAT),
DEFINE_FIELD(m_flTimeLastUpdatedRadius, FIELD_TIME),
DEFINE_FIELD(m_vecImmolatorTarget, FIELD_VECTOR),

DEFINE_ENTITYFUNC(UpdateThink),
END_DATADESC()


//-----------------------------------------------------------------------------
// Maps base activities to weapons-specific ones so our characters do the right things.
//-----------------------------------------------------------------------------
acttable_t CWeaponImmolator::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SNIPER_RIFLE, true }
};

IMPLEMENT_ACTTABLE(CWeaponImmolator);

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
	SetThink(&CWeaponImmolator::UpdateThink);
	SetNextThink(gpGlobals->curtime);

	WeaponSound(SINGLE);
	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	DispatchParticleEffect("immo_beam_muzzle01", PATTACH_POINT_FOLLOW, ToBasePlayer(GetOwner())->GetViewModel(), "muzzle", true);

	CSoundEnt::InsertSound(SOUND_DANGER, m_vecImmolatorTarget, 256, 5.0, GetOwner());
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
	PrecacheParticleSystem("immo_beam_muzzle01");
	m_beamIndex = PrecacheModel("sprites/bluelaser1.vmt");

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
	}
}

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

	Update();
	SetNextThink(gpGlobals->curtime + 0.05);
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


		// Create a new entity with CCrossbowBolt private data
		CImmolatorPlasmaBall *pPlasmaBall = (CImmolatorPlasmaBall *)CBaseEntity::Create("immolator_plasma_ball", playerMuzzleVector, QAngle(0,0,0), pOwner);
		//UTIL_SetOrigin(pPlasmaBall, vecSrc);
		//pPlasmaBall->SetLocalOrigin(vecShootOrigin2);
		//pPlasmaBall->SetAbsAngles(vecAngles);
		pPlasmaBall->Spawn();
		pPlasmaBall->SetOwnerEntity(pOwner);
		pPlasmaBall->SetBaseVelocity((vecAiming * 512) + GetAbsVelocity());
		pPlasmaBall->SetContextThink(&CImmolatorPlasmaBall::SUB_Remove, gpGlobals->curtime + 0.75, "KillBoltThink");
		pPlasmaBall->SetCollisionGroup(COLLISION_GROUP_NONE);

		GetOwner()->RemoveAmmo(1, m_iPrimaryAmmoType);

		// If the gun runs out of ammo, stop firing
		if (pOwner && pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		{
			StopImmolating();
			// Can't use the immolator again for a while
			m_flNextPrimaryAttack = gpGlobals->curtime + 5.0;
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
	CTakeDamageInfo info = CTakeDamageInfo(this, this, sk_plr_dmg_immolator.GetFloat(), DMG_BURN | DMG_DIRECT);

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

	BaseClass::ItemPostFrame();
}



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
	SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM);
	UTIL_SetSize(this, -Vector(0.3f, 0.3f, 0.3f), Vector(0.3f, 0.3f, 0.3f));
	SetCollisionBounds(-Vector(7.0f, 7.0f, 7.0f), Vector(7.0f, 7.0f, 7.0f)); // make immo hits easier to land
	SetSolid(SOLID_VPHYSICS);
	SetGravity(0.05f);

	DispatchParticleEffect("immo_beam_fire01", PATTACH_ABSORIGIN_FOLLOW, this);
	SetTouch(&CImmolatorPlasmaBall::BoltTouch);
}


void CImmolatorPlasmaBall::Precache(void)
{
	PrecacheParticleSystem("immo_beam_fire01");
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
	// Don't burn the player
	if (pOther->IsPlayer())
		return;

	CAI_BaseNPC *pNPC;
	pNPC = dynamic_cast<CAI_BaseNPC*>(pOther);
	if (pNPC) {
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
		pNPC->IgniteLifetimeGreen(sk_immolator_rtbr_burn_seconds.GetInt());
	}

	// If this is a breakable prop, ignite it!
	CBreakableProp *pBreakable;
	pBreakable = dynamic_cast<CBreakableProp*>(pOther);
	if (pBreakable && pBreakable->GetHealth() > 0)
	{
		pBreakable->IgniteLifetimeGreen(sk_immolator_rtbr_burn_seconds.GetInt());
		// Don't do damage to props that are on fire
		/*if (pBreakable->IsOnFire())
			return;*/
		if (pBreakable->GetHealth() < 3) 
			pBreakable->Break(this, CTakeDamageInfo(this, this, sk_plr_dmg_immolator.GetFloat(), (DMG_BURN)));
	}

	// Do damage
	//if (!FStrEq(STRING(pOther->m_iClassname), "prop_physics")) {
		pOther->TakeDamage(CTakeDamageInfo(this, this, sk_plr_dmg_immolator.GetFloat(), (DMG_BURN | DMG_DIRECT | DMG_SLOWBURN)));
	//}

}