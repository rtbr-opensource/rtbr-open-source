#include "cbase.h"
#include "npc_baseally.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CNPC_Barney : public CNPC_BaseAlly {
	virtual void Spawn();
	virtual void Precache();

	DEFINE_CUSTOM_AI;
};

LINK_ENTITY_TO_CLASS(npc_barney, CNPC_Barney);
ConVar	sk_barney_health("sk_barney_health", "35");

void CNPC_Barney::Spawn() {
	Precache();

	SetHealth(sk_barney_health.GetInt());
	SetMaxHealth(sk_barney_health.GetInt());

	m_iszIdleExpression = MAKE_STRING("scenes/Expressions/BarneyIdle.vcd");
	m_iszAlertExpression = MAKE_STRING("scenes/Expressions/BarneyAlert.vcd");
	m_iszCombatExpression = MAKE_STRING("scenes/Expressions/BarneyCombat.vcd");

	BaseClass::Spawn();

	NPCInit();
}

void CNPC_Barney::Precache() {
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC(npc_barney, CNPC_Barney)

AI_END_CUSTOM_NPC()