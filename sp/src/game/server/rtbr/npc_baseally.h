//========= Copyright (c) RTBR Team, 2022 ============//
//
// Purpose: All allies derive from this base class
//
//====================================================//

#include "cbase.h"
#include "npc_playercompanion.h"
#include "sceneentity.h"

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
