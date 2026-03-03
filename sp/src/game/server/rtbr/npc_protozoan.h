//=============================================================================
//
// Purpose: protozoan
//
//=============================================================================

#ifndef NPC_PROTOZOAN_H
#define NPC_PROTOZOAN_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "player_pickup.h"
#include "weapon_physcannon.h"
#include "hl2_player.h"
#include "ai_basenpc_physicsflyer.h"


#define PROTOZONAN_DEFAULT_MAX_WANDER_DISTANCE		350.0f
#define PROTOZONAN_DEFAULT_MAX_PERSUE_DISTANCE		1250.0f
#define PROTOZONAN_DEFAULT_MIN_GROUND_HEIGHT		48.0f

ConVar	sk_protozoan_health("sk_protozoan_health", "0");
ConVar	sk_protozoan_heal_range("sk_protozoan_heal_range", "0");
ConVar	sk_protozoan_min_heal_amount("sk_protozoan_min_heal_amount", "0");
ConVar	sk_protozoan_max_heal_amount("sk_protozoan_max_heal_amount", "0");
ConVar	sk_protozoan_heal_interval("sk_protozoan_heal_interval", "0");

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_Protozoan : public CAI_BasePhysicsFlyingBot, public CDefaultPlayerPickupVPhysics
{
	DECLARE_CLASS(CNPC_Protozoan, CAI_BasePhysicsFlyingBot);
	DECLARE_SERVERCLASS();

public:
	IMotionEvent::simresult_e Simulate(IPhysicsMotionController* pController, IPhysicsObject* pObject, float deltaTime, Vector& linear, AngularImpulse& angular);

	void			Precache(void);
	void			Spawn(void);

	Class_T			Classify(void) { return(CLASS_PROTOZOAN); }

    float           GetMaxSpeed(void) { return 5.0f; }
	void			MaintainGroundHeight(void);

	int				OnTakeDamage_Alive(const CTakeDamageInfo& info);
	void			Event_Killed( const CTakeDamageInfo &info );

	float			GetMaxWanderDistance() { return ((m_flMaxWanderDistanceOverride == -1.0f) ? PROTOZONAN_DEFAULT_MAX_WANDER_DISTANCE : m_flMaxWanderDistanceOverride); }
	float			GetMaxPersueDistance() { return ((m_flMaxPersueDistanceOverride == -1.0f) ? PROTOZONAN_DEFAULT_MAX_PERSUE_DISTANCE : m_flMaxPersueDistanceOverride); }
	float			GetMinGroundHeight() { return PROTOZONAN_DEFAULT_MIN_GROUND_HEIGHT; }

	float			GetHealAmount() {

		float flHealMin;
		flHealMin = sk_protozoan_min_heal_amount.GetFloat();

		float flHealMax;
		flHealMax = sk_protozoan_max_heal_amount.GetFloat();

		return random->RandomFloat(flHealMin, flHealMax);
	
	}
	float			GetHealRange() { return sk_protozoan_heal_range.GetFloat(); }
	float			GetHealInterval() { return sk_protozoan_heal_interval.GetFloat(); }

	virtual void	NPCThink(void);
	virtual void	GatherConditions(void);
	virtual int		SelectSchedule(void);
	virtual int		SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode);
	virtual void	OnStartSchedule(int scheduleType);
    int	        	TranslateSchedule(int scheduleType);
    void			StartTask(const Task_t* pTask);
    void			RunTask(const Task_t* pTask);

	bool			OverrideMove(float flInterval);
	bool			OverridePathMove(CBaseEntity* pMoveTarget, float flInterval);
    void			MoveToTarget(float flInterval, const Vector& vecMoveTarget);
    bool			CanBePickedUpByPhyscannon(CBasePlayer* pPlayer) { return true; };
    void			ClampMotorForces(Vector& linear, AngularImpulse& angular);

	virtual void	OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason);
	virtual void	OnPhysGunDrop(CBasePlayer* pPhysGunUser, PhysGunDrop_t Reason);

protected:

    virtual void	MoveExecute_Alive(float flInterval);

	DEFINE_CUSTOM_AI;

	// Custom interrupt conditions
	enum
	{
		COND_PROTOZOAN_FLY_CLEAR = BaseClass::NEXT_CONDITION,
		COND_PROTOZOAN_FLY_BLOCKED,
		COND_PROTOZOAN_GRABBED_BY_PHYSCANNON,
		COND_PROTOZOAN_FORCE_MOVE,
		COND_PROTOZAN_OUTSIDE_MAX_WANDER_RANGE,
		COND_PROTOZAN_TARGET_OUTSIDE_MAX_PERSUE_RANGE,
		COND_PROTOZAN_SHOULD_IDLE,

		NEXT_CONDITION,
	};

	// Custom schedules
	enum
	{
		SCHED_PROTOZOAN_IDLE = BaseClass::NEXT_SCHEDULE,
		SCHED_PROTOZOAN_WANDER,
		SCHED_PROTOZOAN_APPROACH_TARGET,
		SCHED_PROTOZOAN_RETURN_TO_ORIGIN, // makes the protozoan make its way back to where it spawned so it doesnt wander too far off from where its placed.
		SCHED_PROTOZOAN_FORCE_MOVE_TO_TARGET, // "target" in this sense is contextual, if m_hForcedEntity is set, it'll go there, if not, it'll go to its spawn

		NEXT_SCHEDULE,
	};

	// Custom tasks
	enum
	{
		TASK_PROTOZOAN_GET_PATH_TO_SPAWN = BaseClass::NEXT_TASK,
		TASK_PROTOZOAN_GET_PATH_TO_CONTEXTUAL_TARGET,
		TASK_PROTOZOAN_FORGET_ENEMIES,

		NEXT_TASK,
	};

	DECLARE_DATADESC();

private:

	// input functions
	void			InputMoveToLocation(inputdata_t& inputdata);
	void			InputMoveToEntity(inputdata_t& inputdata);

	// Output Events
	COutputEHANDLE	m_OnHealEntityOutput;

	EHANDLE			m_hPrevOwner;
	EHANDLE			m_hForcedEntity; // entity ive been forced to move to
	float			m_flNextHealCheck; // stores the next time to check if a player is healable
	Vector			m_vSpawnLocation; // remembers this so it doesnt wander too far off from where the mapper placed it


	float			m_flMaxWanderDistanceOverride;
	float			m_flMaxPersueDistanceOverride;

	//float			m_flHealAmountMinOverride;
	//float			m_flHealAmountMaxOverride;
	//float			m_flHealIntervalOverride;
	//float			m_flHealRangeOverride;
	float			m_flTimeOfDeath;

	float			m_flNextHealSoundTime;
	bool			m_bIsHealing;
	bool			m_bIsPlayingIdleSound;

	// NOTE: We could probably handle the death particles on the server,
	// but since we need the client for idle vfx, might as well handle all particles over there.
	CNetworkVar( bool, m_bDispatchDeathVFX );
};
#endif // NPC_PROTOZOAN_H
