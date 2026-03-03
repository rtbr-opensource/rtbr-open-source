//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
// 
//=============================================================================//

#ifndef NPC_HOUNDEYEALPHA_H
#define NPC_HOUNDEYEALPHA_H
#pragma once

#include	"npc_houndeye.h"

#include	"energy_wave.h"
#include	"grenade_energy.h"

class CNPC_HoundeyeAlpha : public CNPC_Houndeye
{
	DECLARE_CLASS(CNPC_HoundeyeAlpha, CNPC_Houndeye);

public:
	CNPC_HoundeyeAlpha();

	void			Spawn(void);
	void			Precache(void);

protected:
	void			WarmUpSound(void);
	void			AlertSound(void);
	void			DeathSound(const CTakeDamageInfo &info);
	void			WarnSound(void);
	void			PainSound(const CTakeDamageInfo &info);
	void			IdleSound(void);
	void			ThumpSound(void);
	void			SpeakSentence(int sentenceType);

	void			Event_Killed(const CTakeDamageInfo& info);

};

#endif // NPC_HOUNDEYEALPHA_H
