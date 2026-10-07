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
//=============================================================================//

#ifndef NPC_HOUNDEYE_H
#define NPC_HOUNDEYE_H
#pragma once


#include	"ai_basenpc.h"

#include	"energy_wave.h"
#include	"grenade_energy.h"

class CNPC_Houndeye : public CAI_BaseNPC
{
	DECLARE_CLASS(CNPC_Houndeye, CAI_BaseNPC);

public:
	void			Spawn(void);
	void			Precache(void);
	Class_T			Classify(void);

	float			MaxYawSpeed(void);

	void			StartTask(const Task_t* pTask);
	void			RunTask(const Task_t* pTask);
	void			HandleAnimEvent(animevent_t *pEvent);
	void			PrescheduleThink(void);
	void			GatherConditions();

	int				RangeAttack1Conditions(float flDot, float flDist);
	virtual int		TranslateSchedule(int scheduleType);
	Activity		NPC_TranslateActivity(Activity eNewActivity);
	bool			HandleInteraction(int interactionType, void* data, CBaseCombatCharacter* sourceEnt);
	void			NPCThink(void);
	int				OnTakeDamage_Alive(const CTakeDamageInfo& info);
	void			Event_Killed(const CTakeDamageInfo& info);

	bool			IsAlpha(void) { return m_bIsAlpha; }

protected:
	void			SonicAttack(void);

	void			WarmUpSound(void);
	void			AlertSound(void);
	void			DeathSound(const CTakeDamageInfo &info);
	void			WarnSound(void);
	void			PainSound(const CTakeDamageInfo &info);
	void			IdleSound(void);
	void			ThumpSound(void);
	void			SpeakSentence(int sentenceType);

	int				GetSoundInterests(void);
	void			WriteBeamColor(void);
	bool			FCanActiveIdle(void);
	virtual int		SelectSchedule(void);
	bool			IsAnyoneInSquadAttacking(void);

	bool			m_bIsAlpha;

	float			m_flNextSecondaryAttack;
	bool			m_bLoopClockwise;

private:

	CHandle< CGrenadeEnergy >	m_hEnergyWave;
	float			m_flEndEnergyWaveTime;

	bool			m_fAsleep;// some houndeyes sleep in idle mode if this is set, the houndeye is lying down
	bool			m_fDontBlink;// don't try to open/close eye if this bit is set!

	DEFINE_CUSTOM_AI;

	DECLARE_DATADESC();
};


#endif // NPC_HOUNDEYE_H
