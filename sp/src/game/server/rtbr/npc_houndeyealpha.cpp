//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Houndeye Alpha - the spooky sonic dog's angry dad.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npc_houndeyealpha.h"
#include "ai_squad.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern int g_interactionHoundeyeGroupRetreat;

LINK_ENTITY_TO_CLASS(npc_houndeyealpha, CNPC_HoundeyeAlpha);

// constructor
CNPC_HoundeyeAlpha::CNPC_HoundeyeAlpha()
{
}

//=========================================================
// Spawn
//=========================================================
void CNPC_HoundeyeAlpha::Spawn()
{
	m_bIsAlpha = true;
	BaseClass::Spawn();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_HoundeyeAlpha::Precache()
{
	PrecacheModel("models/houndeye_alpha.mdl");

	PrecacheScriptSound("NPC_HoundeyeAlpha.Anger1");
	PrecacheScriptSound("NPC_HoundeyeAlpha.Anger2");
	PrecacheScriptSound("NPC_HoundeyeAlpha.SpeakSentence");
	PrecacheScriptSound("NPC_HoundeyeAlpha.Idle");
	PrecacheScriptSound("NPC_HoundeyeAlpha.WarmUp");
	PrecacheScriptSound("NPC_HoundeyeAlpha.Warn");
	PrecacheScriptSound("NPC_HoundeyeAlpha.Alert");
	PrecacheScriptSound("NPC_HoundeyeAlpha.Die");
	PrecacheScriptSound("NPC_HoundeyeAlpha.Pain");
	PrecacheScriptSound("NPC_HoundeyeAlpha.Retreat");
	PrecacheScriptSound("NPC_HoundeyeAlpha.SonicAttack");

	PrecacheScriptSound("NPC_HoundeyeAlpha.GroupAttack");
	PrecacheScriptSound("NPC_HoundeyeAlpha.GroupFollow");

	BaseClass::Precache();
}

void CNPC_HoundeyeAlpha::SpeakSentence(int sentenceType)
{
	if (gpGlobals->curtime > m_flSoundWaitTime)
	{
		EmitSound("NPC_HoundeyeAlpha.SpeakSentence");
		m_flSoundWaitTime = gpGlobals->curtime + 1.0;
	}
}

void CNPC_HoundeyeAlpha::IdleSound(void)
{
	if (!FOkToMakeSound())
	{
		return;
	}
	CPASAttenuationFilter filter(this, "NPC_HoundeyeAlpha.Idle");
	EmitSound(filter, entindex(), "NPC_HoundeyeAlpha.Idle");
}

void CNPC_HoundeyeAlpha::WarmUpSound(void)
{
	EmitSound("NPC_HoundeyeAlpha.WarmUp");
}

void CNPC_HoundeyeAlpha::WarnSound(void)
{
	EmitSound("NPC_HoundeyeAlpha.Warn");
}

void CNPC_HoundeyeAlpha::ThumpSound(void)
{
	EmitSound("NPC_HoundeyeAlpha.SonicAttack");
}

void CNPC_HoundeyeAlpha::AlertSound(void)
{
	// only first squad member makes ALERT sound.
	if (m_pSquad && !m_pSquad->IsLeader(this))
	{
		return;
	}

	EmitSound("NPC_HoundeyeAlpha.Alert");
}

void CNPC_HoundeyeAlpha::DeathSound(const CTakeDamageInfo &info)
{
	EmitSound("NPC_HoundeyeAlpha.Die");
}

void CNPC_HoundeyeAlpha::PainSound(const CTakeDamageInfo &info)
{
	EmitSound("NPC_HoundeyeAlpha.Pain");
}

//------------------------------------------------------------------------------
// Purpose : Broadcast retreat when member of squad killed
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_HoundeyeAlpha::Event_Killed(const CTakeDamageInfo& info)
{
	m_flSoundWaitTime = gpGlobals->curtime + 1.0;
	EmitSound("NPC_HoundeyeAlpha.Retreat");

	if (m_pSquad)
	{
		AISquadIter_t iter;
		CAI_BaseNPC* pSquadmate = m_pSquad ? m_pSquad->GetFirstMember(&iter) : NULL;
		bool bSquadHasAlpha = false;
		while (pSquadmate)
		{
			CNPC_Houndeye* pHoundeye = dynamic_cast<CNPC_Houndeye*>(pSquadmate);

			// i just died, dont count me
			if (pHoundeye == this)
			{
				pSquadmate = m_pSquad->GetNextMember(&iter);
				continue;
			}

			if (pHoundeye->IsAlpha()) {
				bSquadHasAlpha = true;
				break;
			}
			pSquadmate = m_pSquad->GetNextMember(&iter);
		}

		if (!bSquadHasAlpha)
			m_pSquad->BroadcastInteraction(g_interactionHoundeyeGroupRetreat, NULL, this);

	}

	BaseClass::Event_Killed(info);
}
