//========= Copyright (c) RTBR Team, 2024 ============//
//
// Purpose: Antlion Rollergrub, general melee nuisance
//
//====================================================//

#ifndef NPC_ANTLIONROLLERGRUB_H
#define NPC_ANTLIONROLLERGRUB_H
#pragma once

#include "ai_basenpc.h"

// Forward declaration
class CNPC_AntlionBirther;

//==================================================
// CNPC_AntlionRollerGrub - Antlion Roller Grub
//==================================================
class CNPC_AntlionRollerGrub : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_AntlionRollerGrub, CAI_BaseNPC );
	DECLARE_DATADESC();

	DEFINE_CUSTOM_AI;

public:
	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual Class_T Classify( void ) { return CLASS_ANTLION; }
	virtual float	InnateRange1MinRange( void ) { return 56.0f; }
	virtual float	InnateRange1MaxRange( void ) { return 300.0f; }
	virtual int		RangeAttack1Conditions( float flDot, float flDist );
	void			GatherConditions(void);
	void			PrescheduleThink();

	virtual int		TranslateSchedule( int scheduleType );
	virtual Activity TranslateActivity( Activity eNewActivity );
	virtual void	OnChangeActivity( Activity eNewActivity );
	virtual int		SelectSchedule( void );
	virtual void	HandleAnimEvent( animevent_t *pEvent );
	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );
	virtual void	Event_Killed( const CTakeDamageInfo &info );

	void		TraceAttack(const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr, CDmgAccumulator* pAccumulator);

	void			FlingFromBirther( CNPC_AntlionBirther *pMommaBirther );
	void			Event_BirtherKilled( void );
	void			TouchDamage( CBaseEntity *pOther );
	
	virtual bool		IsPhyscannonPuntable( void ) { return true; }

	//TODO: find a better way of doing footstep sounds other than NPCThink(), perhaps anim events
	void				NPCThink(void);

	// temp, its way too slow with the current animations
	float				MaxYawSpeed() { return BaseClass::MaxYawSpeed() * 2.5f; }
	float				GetIdealSpeed() const { return BaseClass::GetIdealSpeed() * 4.5f; }

	void				PainSound(const CTakeDamageInfo& info);
	void				DeathSound(const CTakeDamageInfo& info);
	void				IdleSound(void);
	void				AlertSound(void);
	void				BiteSound(void);
	void				AttackSound(void);

	Vector				ConstructFlingVector(CBaseEntity* pEnemy, Vector vFrom);

private:

	float			m_flNextFootstepSoundTime;

	//==================================================
	// RollerGrub Tasks
	//==================================================
	enum
	{
		TASK_ROLLERGRUB_FLING = LAST_SHARED_TASK,
	};

	//==================================================
	// RollerGrub Schedules
	//==================================================
	enum
	{
		SCHED_ROLLERGRUB_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE,
		SCHED_ROLLERGRUB_FLING_FROM_BIRTHER,
		SCHED_ROLLERGRUB_RELAXED_WANDER
	};

private:
	bool						 m_bFlinging;
	CHandle<CNPC_AntlionBirther> m_hMommaBirther;

	// will be set if the rollergrub has just spawned from a birther
	bool						 m_bFlingedFromBirther;
};

#endif // NPC_ANTLIONROLLERGRUB_H
