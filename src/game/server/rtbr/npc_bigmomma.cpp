//========= Copyright © 2022, Raising the Bar Redux, All rights reserved. ============//
// NPC: Big Momma
// Creator: Iridium
//
// Purpose of the NPC:
// Big momma NPC appears near the end of the mine section of Quarrytown, a one time
// NPC that spawns headcrabs.
//
// Notes:
// NPC has no motion due to requirement to be entirely contained within a prop.
//
//====================================================================================//
#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_squad.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "activitylist.h"
#include "ai_basenpc.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
//=========================================================
class CNPC_BigMomma : public CAI_BaseNPC
{
	DECLARE_CLASS(CNPC_BigMomma, CAI_BaseNPC);
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

public:
	void	Precache(void);
	void	Spawn(void);

	void HandleAnimEvent(animevent_t* pEvent);
	Class_T Classify(void);
	virtual float	MaxYawSpeed(void);
    void CrabThink(void);
	virtual Activity NPC_TranslateActivity(Activity eNewActivity);

	void InputEnableFear(inputdata_t& inputdata);
	void InputDisableFear(inputdata_t& inputdata);
	void InputEnableHeadcrab(inputdata_t& inputdata);
	void InputDisableHeadcrab(inputdata_t& inputdata);

	void DeathSound(const CTakeDamageInfo& info);
	void IdleSound();

	bool m_bCanSpawnCrabs = true;
	bool m_bIsScared = false;

private:
};

enum
{
	SCHED_MOMMA_BABY = LAST_SHARED_SCHEDULE,

};

LINK_ENTITY_TO_CLASS(npc_bigmomma, CNPC_BigMomma);

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_BigMomma)
DEFINE_INPUTFUNC(FIELD_VOID, "EnableFear", InputEnableFear),
DEFINE_INPUTFUNC(FIELD_VOID, "DisableFear", InputDisableFear),
DEFINE_INPUTFUNC(FIELD_VOID, "EnableHeadcrab", InputEnableHeadcrab),
DEFINE_INPUTFUNC(FIELD_VOID, "DisableHeadcrab", InputDisableHeadcrab),
DEFINE_FIELD(m_bCanSpawnCrabs, FIELD_BOOLEAN),
DEFINE_FIELD(m_bIsScared, FIELD_BOOLEAN),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_BigMomma::Precache(void)
{
	PrecacheModel("models/props_rtbr/bigmommapod.mdl");

	PrecacheScriptSound("BigMomma.IdleNormal");
	PrecacheScriptSound("BigMomma.IdleScared");
	PrecacheScriptSound("BigMomma.Die");

	UTIL_PrecacheOther("npc_babycrab");

	BaseClass::Precache();
}

float CNPC_BigMomma::MaxYawSpeed(void) {
	return 0.0f;
}

void CNPC_BigMomma::CrabThink() {
	if (m_bCanSpawnCrabs) {
		SetSchedule(SCHED_MOMMA_BABY);
		SetCycle(1.0F);
	}
		SetNextThink(gpGlobals->curtime + random->RandomFloat(5.0f, 30.0f), "CrabContext");
		SetNextThink(gpGlobals->curtime);
}


Activity CNPC_BigMomma::NPC_TranslateActivity(Activity eNewActivity)
{
	if (eNewActivity == ACT_IDLE)
	{
		if (m_bIsScared) {
			return (Activity)ACT_COWER;
		}
		else {
			return (Activity)ACT_IDLE;
		}
	}
	return BaseClass::NPC_TranslateActivity(eNewActivity);
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_BigMomma::Spawn(void)
{
	Precache();

	SetModel("models/props_rtbr/bigmommapod.mdl");
	SetHullType(HULL_LARGE_CENTERED);
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_NONE);
	SetBloodColor(BLOOD_COLOR_GREEN);
	m_iHealth = 20000;
	m_flFieldOfView = 0.5; // indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState = NPC_STATE_IDLE;

	m_bIsScared = false;

	IdleSound();

	CapabilitiesClear();
	//CapabilitiesAdd();

	NPCInit();
	RegisterThinkContext("CrabContext");
	SetContextThink(&CNPC_BigMomma::CrabThink, gpGlobals->curtime + random->RandomFloat(2.0F, 20.0F), "CrabContext");

}

//=========================================================
// HandleAnimEvent 
//=========================================================
void CNPC_BigMomma::HandleAnimEvent(animevent_t* pEvent)
{
	if (pEvent->type & AE_TYPE_NEWEVENTSYSTEM)
	{
		if (pEvent->event == AE_NPC_WEAPON_FIRE) {
			Create("npc_babycrab", GetAbsOrigin() + Vector(0, 0, -72), GetAbsAngles(), NULL);
		}
		else {
			BaseClass::HandleAnimEvent(pEvent);
		}
	}
}

//=========================================================
// IdleSound 
//=========================================================
#define MOMMA_ATTN_IDLE	(float)1.5
void CNPC_BigMomma::IdleSound(void)
{
}

//=========================================================
// DeathSound
//=========================================================
void CNPC_BigMomma::DeathSound(const CTakeDamageInfo& info)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_BigMomma::Classify(void)
{
	return	CLASS_HEADCRAB;
}

void CNPC_BigMomma::InputEnableFear(inputdata_t& inputdata) {
	m_bIsScared = true;
	IdleSound();
}

void CNPC_BigMomma::InputDisableFear(inputdata_t& inputdata) {
	m_bIsScared = false;
	IdleSound();
}

void CNPC_BigMomma::InputEnableHeadcrab(inputdata_t& inputdata) {
	m_bCanSpawnCrabs = true;
}

void CNPC_BigMomma::InputDisableHeadcrab(inputdata_t& inputdata) {
	m_bCanSpawnCrabs = false;
}



AI_BEGIN_CUSTOM_NPC(npc_bigmomma, CNPC_BigMomma)
DEFINE_SCHEDULE
(
	SCHED_MOMMA_BABY,
	"	Tasks"
	"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_SPECIAL_ATTACK1" // keep doing it
	""
	"	Interrupts"

)

AI_END_CUSTOM_NPC()