//==============================================================================//
//
//
//=============================================================================//
#include "cbase.h"
#include "ai_squad.h"
#include "npcevent.h"
#include "particle_parse.h"
#include "ez2/npc_baseflora.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define XENMUSHROOM_ATTACK_COOLDOWN	2.5
#define XENMUSHROOM_DAMAGE_RADIUS 128.0f

class CNPC_XenShroom : public CNPC_BaseFlora
{
	DECLARE_CLASS(CNPC_XenShroom, CNPC_BaseFlora);

public:
	void			Spawn();
	void			Precache(void);
	virtual void	NPCThink(void);
	void			Event_Killed(const CTakeDamageInfo& info);
	virtual bool CanBoogie() OVERRIDE { return false; }

	void	OnChangeActivity(Activity eNewActivity);
	void	HandleAnimEvent(animevent_t* pEvent);

	void	ShroomDMG();

	int		SelectSchedule(void);


protected:

	int		StimulusMask() {
		return bits_REACT_XENFLORA_ENTITY_APPROACH |
			bits_REACT_XENFLORA_HURT |
			bits_REACT_XENFLORA_HEAR_DANGER |
			bits_REACT_XENFLORA_NEARBY_GUNSHOT;
	}

	float GetViewDistance() { return 192.0f; }
	float GetFieldOfView() { return DOT_45DEGREE; }

	virtual float	GetReactionDistance()	{ return 64.0f; }

	DECLARE_DATADESC();

private:

	float	m_flFieldEnd;
	float	m_flNextAttackTime;

	float	m_flNextIdleSoundTime;

};

BEGIN_DATADESC(CNPC_XenShroom)
DEFINE_THINKFUNC(ShroomDMG),
END_DATADESC()

LINK_ENTITY_TO_CLASS(npc_xenshroom, CNPC_XenShroom);

#define AE_SHROOM_ATTACK			( 1 )

ConVar sk_xenshroom_health("sk_xenshroom_health", "40");
ConVar sk_xenshroom_dmg("sk_xenshroom_max_dmg", "10");

void CNPC_XenShroom::Spawn()
{
	BaseClass::Spawn();
	SetHullType(HULL_SMALL_CENTERED);

	CapabilitiesAdd(bits_CAP_INNATE_MELEE_ATTACK1);
	
	RegisterThinkContext("ShroomDMG");
	SetContextThink(&CNPC_XenShroom::ShroomDMG, gpGlobals->curtime, "ShroomDMG");

	m_iMaxHealth = sk_xenshroom_health.GetFloat();
	m_iHealth = m_iMaxHealth;
	m_flFieldEnd = 0;
	m_flNextAttackTime = 0;
	m_flNextIdleSoundTime = 0;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_XenShroom::Precache()
{
	if (GetModelName() == NULL_STRING)
	{
		SetModelName(AllocPooledString("models/fauna/fauna_mushroom.mdl"));
	}

	PrecacheScriptSound("Xen_Tube.Fumes");
	PrecacheScriptSound("shroomfauna.attack");
	PrecacheScriptSound("shroomfauna.death");
	PrecacheParticleSystem("npc_shroomfauna_gas");

	BaseClass::Precache();
}

void CNPC_XenShroom::NPCThink()
{
	if (gpGlobals->curtime >= m_flNextIdleSoundTime)
	{
		m_flNextIdleSoundTime = gpGlobals->curtime + RandomFloat(4.0f, 6.0f);
		EmitSound("Xen_Tube.Fumes");
	}

	BaseClass::NPCThink();
}

void CNPC_XenShroom::Event_Killed(const CTakeDamageInfo& info)
{
	EmitSound("shroomfauna.death");
	BaseClass::Event_Killed(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_XenShroom::OnChangeActivity(Activity eNewActivity)
{
	BaseClass::OnChangeActivity(eNewActivity);

	switch (eNewActivity)
	{
	case ACT_MELEE_ATTACK1:
	{
		EmitSound("shroomfauna.attack");
	}
	break;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_XenShroom::HandleAnimEvent(animevent_t* pEvent)
{
	switch (pEvent->event)
	{
	case AE_SHROOM_ATTACK:
	{
		DispatchParticleEffect("npc_shroomfauna_gas", GetAbsOrigin(), QAngle(0, 0, 0), this);
		m_flFieldEnd = gpGlobals->curtime + 6;
	}
	break;

	default:
		BaseClass::HandleAnimEvent(pEvent);
	}
}

int CNPC_XenShroom::SelectSchedule(void)
{
	if (HasCondition(COND_XENFLORA_SHOULD_REACT))
	{
		if (gpGlobals->curtime > m_flNextAttackTime)
		{
			m_flNextAttackTime = gpGlobals->curtime + XENMUSHROOM_ATTACK_COOLDOWN;
			return SCHED_MELEE_ATTACK1;
		}
	}

	return SCHED_XENFLORA_IDLE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_XenShroom::ShroomDMG() {
	if (m_flFieldEnd > gpGlobals->curtime) {
		CTakeDamageInfo info = CTakeDamageInfo(this, GetOwnerEntity(), sk_xenshroom_dmg.GetFloat(), DMG_NERVEGAS);
		RadiusDamage(info, GetAbsOrigin(), XENMUSHROOM_DAMAGE_RADIUS, CLASS_ALIEN_FLORA, NULL);
	}

	SetNextThink(gpGlobals->curtime + .5, "ShroomDMG");
}
