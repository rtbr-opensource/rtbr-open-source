#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "ai_hint.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_behavior.h"
#include "ai_baseactor.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "npc_playercompanion.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "activitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "sceneentity.h"
#include "ai_behavior_functank.h"

//=========================================================
// Barney activities
//=========================================================

class CNPC_BaseAlly : public CNPC_PlayerCompanion
{
public:
	DECLARE_CLASS(CNPC_BaseAlly, CNPC_PlayerCompanion);
	//DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Precache()
	{
		// Prevents a warning
		SelectModel();
		BaseClass::Precache();

		PrecacheScriptSound("NPC_Barney.FootstepLeft");
		PrecacheScriptSound("NPC_Barney.FootstepRight");
		PrecacheScriptSound("NPC_Barney.Die");

		PrecacheInstancedScene("scenes/Expressions/BarneyIdle.vcd");
		PrecacheInstancedScene("scenes/Expressions/BarneyAlert.vcd");
		PrecacheInstancedScene("scenes/Expressions/BarneyCombat.vcd");
	}

	void	Spawn(void);
	void	SelectModel();
	Class_T Classify(void);
	void	Weapon_Equip(CBaseCombatWeapon* pWeapon);

	bool CreateBehaviors(void);

	void HandleAnimEvent(animevent_t* pEvent);

	bool ShouldLookForBetterWeapon() { return false; }

	void OnChangeRunningBehavior(CAI_BehaviorBase* pOldBehavior, CAI_BehaviorBase* pNewBehavior);

	void DeathSound(const CTakeDamageInfo& info);
	void GatherConditions();
	void UseFunc(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	CAI_FuncTankBehavior		m_FuncTankBehavior;
	COutputEvent				m_OnPlayerUse;
};
#pragma once
