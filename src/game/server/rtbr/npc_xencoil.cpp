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


class CNPC_XenCoil : public CNPC_BaseFlora
{
	DECLARE_CLASS(CNPC_XenCoil, CNPC_BaseFlora);

public:
	void			Spawn();
	void			Precache(void);
	void			NPCThink(void);

	void			Event_Killed(const CTakeDamageInfo& info);

	void			OnChangeActivity(Activity eNewActivity);
	void			HandleAnimEvent(animevent_t* pEvent);
	virtual bool CanBoogie() OVERRIDE { return false; }

protected:

	int		StimulusMask() {
		return bits_REACT_XENFLORA_ENTITY_APPROACH |
			bits_REACT_XENFLORA_HURT |
			/*bits_REACT_XENFLORA_HEAR_DANGER |*/
			bits_REACT_XENFLORA_NEARBY_GUNSHOT/* |
			bits_REACT_XENFLORA_ENEMY_IN_HIT_RANGE*/;
	}

	float GetViewDistance() { return 80.0f; }
	float GetFieldOfView() { return 0.9; }

	virtual float	GetReactionDistance() { return 48.0f; }

	DECLARE_DATADESC();

private:
	float	m_flNextIdleSoundTime;
};

BEGIN_DATADESC(CNPC_XenCoil)
DEFINE_FIELD(m_flNextIdleSoundTime, FIELD_TIME),
END_DATADESC()

LINK_ENTITY_TO_CLASS(npc_xencoil, CNPC_XenCoil);

#define AE_COIL_ATTACK			( 1 )

ConVar sk_xencoil_health("sk_xencoil_health", "40");
ConVar sk_xencoil_dmg("sk_xencoil_dmg", "10");

void CNPC_XenCoil::Spawn()
{
	BaseClass::Spawn();
	// Xen trees are bigger than other flora - hunter sized
	SetHullType(HULL_SMALL_CENTERED);

	// Xen trees have an innate melee attack
	CapabilitiesAdd(bits_CAP_INNATE_MELEE_ATTACK1);

	m_iMaxHealth = sk_xencoil_health.GetFloat();
	m_iHealth = m_iMaxHealth;
	m_flNextIdleSoundTime = gpGlobals->curtime;
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_XenCoil::Precache()
{
	if (GetModelName() == NULL_STRING)
	{
		SetModelName(AllocPooledString("models/fauna/fauna_coil.mdl"));
	}

	PrecacheScriptSound("whipfauna.idle");
	PrecacheScriptSound("whipfauna.die");
	PrecacheScriptSound("whipfauna.attack");

	PrecacheScriptSound("NPC_HeadCrab.Bite"); // no dedicated hit sound, using headcrab bite for now

	BaseClass::Precache();
}

void CNPC_XenCoil::NPCThink(void)
{
	if (gpGlobals->curtime >= m_flNextIdleSoundTime)
	{
		m_flNextIdleSoundTime = gpGlobals->curtime + RandomFloat(4.0f, 6.0f);
		EmitSound("whipfauna.idle");
	}

	BaseClass::NPCThink();
}

void CNPC_XenCoil::Event_Killed(const CTakeDamageInfo& info)
{
	EmitSound("whipfauna.die");
	BaseClass::Event_Killed(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_XenCoil::OnChangeActivity(Activity eNewActivity)
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
void CNPC_XenCoil::HandleAnimEvent(animevent_t* pEvent)
{
	switch (pEvent->event)
	{
	case AE_COIL_ATTACK:
	{
		trace_t tr;
		Vector local_trace_offset = Vector(0.507f, 0.0f, 0.862f); // 60 degrees upward, about the same direction the model faces
		local_trace_offset = local_trace_offset * (48.0f);

		Vector global_trace_offset;
		matrix3x4_t rotationMatrix;

		QAngle rotator = GetAbsAngles();

		AngleMatrix(rotator, rotationMatrix);
		VectorTransform(local_trace_offset, rotationMatrix, global_trace_offset);

		UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + global_trace_offset, MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr);

		if (tr.fraction < 1.0)
		{
			CBaseEntity* pEntity = tr.m_pEnt;
			CBaseCombatCharacter* pHurt = dynamic_cast<CBaseCombatCharacter*>(pEntity);
			if (pHurt)
			{
				CTakeDamageInfo info = CTakeDamageInfo(this, this, sk_xencoil_dmg.GetFloat(), DMG_SLASH);
				pHurt->TakeDamage(info);

				EmitSound("NPC_HeadCrab.Bite");

				if (pHurt->GetFlags() & (FL_NPC | FL_CLIENT))
					pHurt->ViewPunch(QAngle(20, 0, -20));
			}
		}
	}
	break;

	default:
		BaseClass::HandleAnimEvent(pEvent);
	}
}
