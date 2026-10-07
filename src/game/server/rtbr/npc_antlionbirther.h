//========= Copyright (c) RTBR Team, 2024 ============//
//
// Purpose: Antlion Birther
//
//====================================================//

#ifndef NPC_ANTLIONBIRTHER_H
#define NPC_ANTLIONBIRTHER_H
#pragma once

#include "ai_basenpc.h"

// Forward declaration
class CNPC_AntlionRollerGrub;

class CFastHeadcrab;
class CBaseHeadcrab;

//==================================================
// CNPC_AntlionBirther - Large Antlion that births rollergrubs
//==================================================
class CNPC_AntlionBirther : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_AntlionBirther, CAI_BaseNPC );
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

public:
	CNPC_AntlionBirther( void );

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	Activate( void );
	virtual Class_T Classify( void ) { return CLASS_ANTLION; }

	virtual void	PrescheduleThink( void );
	virtual int		SelectSchedule( void );
	virtual int		MeleeAttack1Conditions( float flDot, float flDist );
	virtual int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual int		RangeAttack1Conditions( float flDot, float flDist );
	virtual void	HandleAnimEvent( animevent_t *pEvent );
	virtual bool	OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	virtual float	MaxYawSpeed( void );
	virtual void	GatherConditions( void ) OVERRIDE;
	virtual void	BuildScheduleTestBits( void ) OVERRIDE;

	CBaseEntity	   *MeleeAttack( void );

	void			FlingRollerGrub( void );
	void			Event_RollerGrubKilled( CNPC_AntlionRollerGrub* pRollergrub );

	void			SpitAttack( void );

	virtual bool	CanBeStunnedBySteambow( void ) const { return false; }

	// Input handlers
	void			Input_EnableSackDamage( inputdata_t &data );
	void			Input_DisableSackDamage( inputdata_t &data );

	// -------------
	// Sounds
	// -------------
	void			IdleSound( void );
	void			AlertSound( void );
	void			DeathSound( void );
	void			PainSound( void );

private:
	void			GetThrowVector( const Vector& vecStartPos, const Vector& vecTarget, float flSpeed, Vector* vecOutbool, bool bSpit = false );

private:
	int		m_iNumRollerGrubsActive;
	int		m_iRollerGrubCapacity;
	float	m_flTimeLastRollergrubThrown;
	float	m_iSackHealth;
	bool	m_bSackDamageEnabled;
	bool	m_bSpittingAcid;

	float	m_flNextIdleSoundTime;
	float	m_flNextPainSoundTime;
	float	m_flNextAlertSoundTime;

	CNetworkVar( bool, m_bSackKilled );

	CUtlVector<CHandle<CNPC_AntlionRollerGrub>> m_hRollerGrubs;
	
private:

	//==================================================
	// AntlionFlinger Conditions
	//==================================================

	enum
	{
		COND_ANTLIONBIRTHER_HAS_ACTIVE_GRUBS = LAST_SHARED_CONDITION,
		COND_ANTLIONBIRTHER_SACK_BURST,
	};

	//==================================================
	// AntlionFlinger Schedules
	//==================================================

	enum
	{
		SCHED_ANTLIONBIRTHER_SHOOT_GRUBS = LAST_SHARED_SCHEDULE,
		SCHED_ANTLIONBIRTHER_MOVE_RANDOM_PATH,
		SCHED_ANTLIONBIRTHER_FINAL_RUSH_ENEMY,
		SCHED_ANTLIONBIRTHER_SPIT_ATTACK,
	};

	//==================================================
	// AntlionFlinger Tasks
	//==================================================

	enum
	{
		TASK_ANTLIONBIRTHER_SHOOT_GRUBS = LAST_SHARED_TASK,
	};

};

#endif // NPC_ANTLIONBIRTHER_H
