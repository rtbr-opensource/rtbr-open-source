//=//=============================================================================//
//
//
//=============================================================================//


#include "cbase.h"
#include "ai_squad.h"
#include "npcevent.h"
#include "ez2/npc_baseflora.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CNPC_XenSack : public CNPC_BaseFlora
{
	DECLARE_CLASS(CNPC_XenSack, CNPC_BaseFlora);

public:
	void Spawn();
	void Precache(void);
	void NPCThink(void);

	void Event_Killed(const CTakeDamageInfo& info);

	void OnChangeActivity(Activity eNewActivity);
	void HandleAnimEvent(animevent_t* pEvent);
	virtual bool CanBoogie() OVERRIDE { return false; }

protected:

	int		StimulusMask() {
		return /*bits_REACT_XENFLORA_ENTITY_APPROACH |*/
			bits_REACT_XENFLORA_HURT |
			/*bits_REACT_XENFLORA_HEAR_DANGER |*/
			//bits_REACT_XENFLORA_NEARBY_GUNSHOT |
			bits_REACT_XENFLORA_ENEMY_IN_HIT_RANGE;
	}

	float GetViewDistance() { return 64.0f; }
	float GetFieldOfView() { return 0.35f; }

	DECLARE_DATADESC();

private:
	float	m_flNextIdleSoundTime;
};

BEGIN_DATADESC(CNPC_XenSack)
DEFINE_FIELD(m_flNextIdleSoundTime, FIELD_TIME),
END_DATADESC()

LINK_ENTITY_TO_CLASS(npc_xensack, CNPC_XenSack);

#define AE_SACK_ATTACK			( 1 )

ConVar sk_xensack_health("sk_xensack_health", "40");
ConVar sk_xensack_dmg("sk_xensack_dmg", "10");

void CNPC_XenSack::Spawn()
{
	BaseClass::Spawn();
	// Xen trees are bigger than other flora - hunter sized
	SetHullType(HULL_SMALL_CENTERED);

	// Xen trees have an innate melee attack
	CapabilitiesAdd(bits_CAP_INNATE_MELEE_ATTACK1);

	m_iMaxHealth = sk_xensack_health.GetFloat();
	m_iHealth = m_iMaxHealth;
	m_flNextIdleSoundTime = gpGlobals->curtime;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_XenSack::Precache()
{
	if (GetModelName() == NULL_STRING)
	{
		SetModelName(AllocPooledString("models/fauna/fauna_sack.mdl"));
	}

	PrecacheScriptSound("whipfauna.idle");
	PrecacheScriptSound("whipfauna.die");
	PrecacheScriptSound("whipfauna.attack");

	PrecacheScriptSound("Zombie.AttackHit"); // no dedicated hit sound, using zombie hit for now

	BaseClass::Precache();
}

void CNPC_XenSack::NPCThink(void)
{
	if (gpGlobals->curtime >= m_flNextIdleSoundTime)
	{
		m_flNextIdleSoundTime = gpGlobals->curtime + RandomFloat(4.0f, 6.0f);
		EmitSound("whipfauna.idle");
	}

	BaseClass::NPCThink();
}

void CNPC_XenSack::Event_Killed(const CTakeDamageInfo& info)
{
	EmitSound("whipfauna.die");
	BaseClass::Event_Killed(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_XenSack::OnChangeActivity(Activity eNewActivity)
{
	BaseClass::OnChangeActivity(eNewActivity);

	switch (eNewActivity)
	{
	case ACT_MELEE_ATTACK1:
	{
		EmitSound("whipfauna.attack");
	}
	break;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_XenSack::HandleAnimEvent(animevent_t* pEvent)
{
	switch (pEvent->event)
	{
	case AE_SACK_ATTACK:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(0, Vector(-32, -32, -32), Vector(32, 32, 32), sk_xensack_dmg.GetFloat(), DMG_SLASH);
		if (pHurt)
		{
			EmitSound("Zombie.AttackHit");
			// Vector right, up;
			// AngleVectors( GetAbsAngles(), NULL, &right, &up );

			if (pHurt->GetFlags() & (FL_NPC | FL_CLIENT))
				pHurt->ViewPunch(QAngle(20, 0, -20));

			// pHurt->ApplyAbsVelocityImpulse( 100 * (up+2*right) * GetModelScale() );
		}
	}
	break;

	default:
		BaseClass::HandleAnimEvent(pEvent);
	}
}