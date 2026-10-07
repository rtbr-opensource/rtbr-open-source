//=============================================================================//
//
// Purpose:		Base class for plant NPCs, specifically Xen plants
//
// Author:		1upD
//
//=============================================================================//

#include "ai_basenpc.h"

class CNPC_BaseFlora : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_BaseFlora, CAI_BaseNPC );
	DECLARE_DATADESC();

public:

	void				Spawn();
	void				Precache( void );
	virtual void		PostNPCInit(void);
	virtual void		OnChangeActivity(Activity eNewActivity);

	virtual Class_T Classify() { return CLASS_ALIEN_FLORA; }
	Disposition_t	IRelationType( CBaseEntity *pTarget );

	virtual int		OnTakeDamage_Alive( const CTakeDamageInfo &info );

	// Plants can't turn
	float			MaxYawSpeed ( void ) { return 0.0f; }
	float			CalcIdealYaw(const Vector& vecTarget) { return UTIL_AngleMod(GetLocalAngles().y); } // Plants can't turn, so the ideal yaw is the current yaw

	void			BuildScheduleTestBits();
	CBaseEntity*	GetRetractActivator();
	int				GetSoundInterests ( void );
	void			GatherConditions();

	void			StartTask(const Task_t* pTask);
	void			RunTask(const Task_t* pTask);

protected:

	DEFINE_CUSTOM_AI;

	enum XenFlora_ReactionType
	{
		bits_REACT_XENFLORA_NO_REPONSE			= 0,
		bits_REACT_XENFLORA_ENTITY_APPROACH		= 1 << 0,
		bits_REACT_XENFLORA_HURT				= 1 << 1,
		bits_REACT_XENFLORA_HEAR_DANGER			= 1 << 2,
		bits_REACT_XENFLORA_NEARBY_GUNSHOT		= 1 << 3,
		bits_REACT_XENFLORA_ENEMY_IN_HIT_RANGE	= 1 << 4,

		NEXT_REACTION							= 1 << 5,
	};

	enum
	{
		SCHED_XENFLORA_IDLE = BaseClass::NEXT_SCHEDULE, // wait until COND_XENLIGHT_SHOULD_REACT is active

		NEXT_SCHEDULE,

	};

	enum
	{
		TASK_XENFLORA_IDLE_LOOP = BaseClass::NEXT_TASK,

		NEXT_TASK,
	};

	enum XenFLora_Conds
	{
		COND_XENFLORA_SHOULD_REACT = BaseClass::NEXT_CONDITION,

		NEXT_CONDITION
	};

	int		GetEnvironmentalResponse(); // 

	virtual int		StimulusMask() {
		return bits_REACT_XENFLORA_ENTITY_APPROACH |
			bits_REACT_XENFLORA_HURT |
			bits_REACT_XENFLORA_HEAR_DANGER |
			bits_REACT_XENFLORA_NEARBY_GUNSHOT |
			bits_REACT_XENFLORA_ENEMY_IN_HIT_RANGE;
	}

	virtual float	GetViewDistance()		{ return 64.0f; }
	virtual float	GetFieldOfView()		{ return 0.35f; }

	int				SelectSchedule();

	virtual float	GetReactionDistance()	{ return 256.0f; } // used for bits_REACT_XENFLORA_ENTITY_APPROACH

	// This is used if the one who made us retract is not in enemy memory
	EHANDLE m_hLastAttacker;

private:

	float		m_flNextTriggerCheck;			// Used to limit the amount of trigger checking we're doing as an optimization.
	float		m_flNextEnvironmentCheck;

	bool		m_bRecentlyDetectedHostile; // remembers if there was an enemy within range the last time we checked, used to avoid running UTIL_EntitiesInSphere every think

	float		m_flAnimSpeedOffset; // stores an random float generated at spawn to determine how much to offset the animation, to create visual variation in groups

};