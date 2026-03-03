//=============================================================================
//
// Purpose: protozoan
//
//=============================================================================

#include "cbase.h"
#include "npcevent.h"
#include "ai_basenpc_physicsflyer.h"
#include "weapon_physcannon.h"
#include "hl2_player.h"
#include "npc_protozoan.h"
#include "IEffects.h"
#include "explode.h"
#include "ai_route.h"
#include "ai_memory.h"

#include "movevars_shared.h"

#define PROTOZOAN_MODEL "models/protozoan.mdl"

#define PROTOZOAN_ACCELERATION						0.5f
#define PROTOZOAN_Z_ACCELERATION					1.25f
#define PROTOZOAN_DECAY								0.1f

#define PROTOZOAN_DEATH_SEQUENCE_DURATION			1.25f
#define PROTOZOAN_DEATH_FINAL_ANIM_SPEED			5.0f

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar g_debug_protozoan_pathing("g_debug_protozoan_pathing", "0", FCVAR_CHEAT);

BEGIN_DATADESC(CNPC_Protozoan)

DEFINE_FIELD(m_hPrevOwner, FIELD_EHANDLE),
DEFINE_FIELD(m_flNextHealCheck, FIELD_FLOAT),
DEFINE_FIELD(m_vSpawnLocation, FIELD_VECTOR),
DEFINE_FIELD(m_hForcedEntity, FIELD_EHANDLE),
DEFINE_FIELD(m_bDispatchDeathVFX, FIELD_BOOLEAN),

DEFINE_KEYFIELD(m_flMaxWanderDistanceOverride, FIELD_FLOAT, "MaxWanderDistanceOverride"),
DEFINE_KEYFIELD(m_flMaxPersueDistanceOverride, FIELD_FLOAT, "MaxPersueDistanceOverride"),

// disabled as they were problematic with protozoans which are not spawned with the map
//DEFINE_KEYFIELD(m_flHealAmountMinOverride, FIELD_FLOAT, "HealAmountMinOverride"),
//DEFINE_KEYFIELD(m_flHealAmountMaxOverride, FIELD_FLOAT, "HealAmountMaxOverride"),
//DEFINE_KEYFIELD(m_flHealIntervalOverride, FIELD_FLOAT, "HealIntervalOverride"),
//DEFINE_KEYFIELD(m_flHealRangeOverride, FIELD_FLOAT, "HealRangeOverride"),

DEFINE_INPUTFUNC(FIELD_VECTOR, "MoveToLocation", InputMoveToLocation),
DEFINE_INPUTFUNC(FIELD_EHANDLE, "MoveToEntity", InputMoveToEntity),

DEFINE_OUTPUT(m_OnHealEntityOutput, "OnHealEntity"),

END_DATADESC()

LINK_ENTITY_TO_CLASS(npc_protozoan, CNPC_Protozoan);

//---------------------------------------------------------
// Networking
//---------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CNPC_Protozoan, DT_NPC_Protozoan )
	SendPropBool( SENDINFO( m_bDispatchDeathVFX ) ),
END_SEND_TABLE()

void CNPC_Protozoan::InputMoveToLocation(inputdata_t& inputdata)
{
	inputdata.value.Vector3D(m_vSpawnLocation);
	SetCondition(COND_PROTOZOAN_FORCE_MOVE);
}

void CNPC_Protozoan::InputMoveToEntity(inputdata_t& inputdata)
{
	m_hForcedEntity = inputdata.value.Entity();
	if (m_hForcedEntity.Get())
	{
		m_vSpawnLocation = m_hForcedEntity.Get()->GetAbsOrigin();
		SetCondition(COND_PROTOZOAN_FORCE_MOVE);
	}
	else {
		Warning("Warning! Protozoan: '%s' received an invalid entity named '%s' in 'MoveToEntity' input from '%s'\n",
			GetDebugName(),
			inputdata.value.String(),
			inputdata.pCaller->GetDebugName());
	}
}

void CNPC_Protozoan::Precache(void)
{
	PrecacheScriptSound("NPC_Protozoan.Idle");
	PrecacheScriptSound("NPC_Protozoan.Speak");
	PrecacheScriptSound("NPC_Protozoan.Die");

	PrecacheModel(PROTOZOAN_MODEL);
	BaseClass::Precache();
}

void CNPC_Protozoan::Spawn(void)
{
	Precache();

	SetModel(PROTOZOAN_MODEL);

	m_iHealth = sk_protozoan_health.GetFloat();
	m_iMaxHealth = m_iHealth;

	SetHullType(HULL_TINY_CENTERED);
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);

	SetMoveType(MOVETYPE_VPHYSICS);

	m_bloodColor = BLOOD_COLOR_GREEN;
	SetViewOffset(Vector(0, 0, 10));		// Position of the eyes relative to NPC's origin.
	m_flFieldOfView = VIEW_FIELD_FULL;
	m_NPCState = NPC_STATE_NONE;

	SetNavType(NAV_FLY);

	AddFlag(FL_FLY);

	// This entity cannot be dissolved by the combine balls,
	// nor does it get killed by the mega physcannon.
	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL);

	//AngleVectors(GetLocalAngles(), &m_vCurrentBanking);
	m_fHeadYaw = 0;

	SetCurrentVelocity(vec3_origin);

	// Noise modifier
	Vector	bobAmount;
	bobAmount.x = random->RandomFloat(-1.0f, 1.0f);
	bobAmount.y = random->RandomFloat(-1.0f, 1.0f);
	bobAmount.z = random->RandomFloat(-1.0f, 1.0f);
	SetNoiseMod(bobAmount);

	// set flight speed
	//m_flSpeed = GetMaxSpeed();

	CapabilitiesAdd(bits_CAP_MOVE_FLY | bits_CAP_TURN_HEAD | bits_CAP_SKIP_NAV_GROUND_CHECK);

	NPCInit();

	m_vSpawnLocation = GetAbsOrigin();

	m_flSpeed = GetMaxSpeed();

	IPhysicsObject* pPhysics = VPhysicsGetObject();
	if (pPhysics)
	{
		pPhysics->EnableGravity(false);
		pPhysics->SetMass(0.5);
	}

	m_flNextHealCheck = 0.0;

	// We need to send our state over often to keep our VFX in sync.
	SetTransmitState( FL_EDICT_PVSCHECK );
	m_bDispatchDeathVFX = false;
}

void CNPC_Protozoan::MoveToTarget(float flInterval, const Vector& vecMoveTarget)
{
	// -------------------------------------
	// Move towards our target
	// -------------------------------------
	bool gDebugPathing = g_debug_protozoan_pathing.GetBool();

	float myAccel = ((!gDebugPathing) ? PROTOZOAN_ACCELERATION : 5.0);
	float myZAccel = ((!gDebugPathing) ? PROTOZOAN_Z_ACCELERATION : 5.0);
	float myDecay = PROTOZOAN_DECAY;

	Vector vecCurrentDir;

	// Get the relationship between my current velocity and the way I want to be going.
	vecCurrentDir = GetCurrentVelocity();
	VectorNormalize(vecCurrentDir);

	Vector targetDir = vecMoveTarget - GetAbsOrigin();

	if (m_flSpeedModifier != 1.0f)
	{
		myAccel *= m_flSpeedModifier;
		myZAccel *= m_flSpeedModifier;
	}

	MoveInDirection(flInterval, targetDir, myAccel, myZAccel, myDecay);

	// calc relative banking targets
	Vector forward, right, up;
	GetVectors(&forward, &right, &up);

	m_vCurrentBanking.x = targetDir.x;
	m_vCurrentBanking.z = 120.0f * DotProduct(right, targetDir);
	m_vCurrentBanking.y = 0;

	float speedPerc = SimpleSplineRemapVal(GetCurrentVelocity().Length(), 0.0f, GetMaxSpeed(), 0.0f, 1.0f);

	speedPerc = clamp(speedPerc, 0.0f, 1.0f);

	m_vCurrentBanking *= speedPerc;
}

bool CNPC_Protozoan::OverrideMove(float flInterval)
{
	Vector vMoveTargetPos(0, 0, 0);
	CBaseEntity* pMoveTarget = NULL;

	// Select move target 
	if (GetTarget() != NULL)
	{
		pMoveTarget = GetTarget();
	}
	else if (GetEnemy() != NULL)
	{
		pMoveTarget = GetEnemy();
	}

	// Select move target position 
	if (GetEnemy() != NULL)
	{
		vMoveTargetPos = GetEnemy()->GetAbsOrigin();
	}

	ClearCondition(COND_PROTOZOAN_FLY_CLEAR);
	ClearCondition(COND_PROTOZOAN_FLY_BLOCKED);

	// See if we can fly there directly
	if (pMoveTarget)
	{
		trace_t tr;
		AI_TraceHull(GetAbsOrigin(), vMoveTargetPos, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);

		float fTargetDist = (1.0f - tr.fraction) * (GetAbsOrigin() - vMoveTargetPos).Length();

		if ((tr.m_pEnt == pMoveTarget) || (fTargetDist < 50))
		{
			SetCondition(COND_PROTOZOAN_FLY_CLEAR);
		}
		else
		{
			SetCondition(COND_PROTOZOAN_FLY_BLOCKED);
		}
	}

	// If I have a route, keep it updated and move toward target
	if (GetNavigator()->IsGoalActive())
	{
		if (OverridePathMove(pMoveTarget, flInterval))
		{
			return true;
		}
	}
	// -----------------------------------------------------------------
	// If I don't have a route, just decelerate
	// -----------------------------------------------------------------
	else
	{
		float	myDecay = 7.5;
		Decelerate(flInterval, myDecay);
	}

	MoveExecute_Alive(flInterval);

	return true;
}

void CNPC_Protozoan::MoveExecute_Alive(float flInterval)
{
	IPhysicsObject* pPhysics = VPhysicsGetObject();

	if (pPhysics && pPhysics->IsAsleep())
	{
		pPhysics->Wake();
	}

	//SetCurrentVelocity(GetCurrentVelocity() + VelocityToAvoidObstacles(flInterval));
	MaintainGroundHeight();


	if (!g_debug_protozoan_pathing.GetBool())
	{
		float flNoiseAmount = (GetCurrentVelocity().Length() / 20.0);
		flNoiseAmount = clamp(flNoiseAmount, 2.5, 4.5);

		AddNoiseToVelocity(flNoiseAmount);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Protozoan::MaintainGroundHeight(void)
{
	trace_t	tr;
	AI_TraceHull(GetAbsOrigin(),
		GetAbsOrigin() - Vector(0, 0, GetMinGroundHeight()),
		GetHullMins(),
		GetHullMaxs(),
		(MASK_NPCSOLID_BRUSHONLY),
		this,
		COLLISION_GROUP_NONE,
		&tr);

	if (tr.fraction != 1.0f)
	{
		float flUpwardForce = powf(1.0 - tr.fraction, 0.65) * 125.0;

		SetCurrentVelocity(GetCurrentVelocity() + Vector(0.0, 0.0, flUpwardForce));
	}
}

bool CNPC_Protozoan::OverridePathMove(CBaseEntity* pMoveTarget, float flInterval)
{
	// Continue on our path
	if (ProgressFlyPath(flInterval, pMoveTarget, (MASK_NPCSOLID | CONTENTS_WATER), false, 64) == AINPP_COMPLETE)
	{
		return true;
	}

	return false;
}

int	CNPC_Protozoan::OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
	// Hafta make a copy of info cause we might need to scale damage.(sjb)
	CTakeDamageInfo tdInfo = info;

	if (tdInfo.GetDamageType() & DMG_CLUB)
	{
		// nerf the crowbars damage force
		tdInfo.ScaleDamageForce(0.4f);
	}

	// making the thing hop slightly when taking damage looks good
	float flDamageMagnitude = tdInfo.GetDamageForce().Length();
	Vector vNewDamageForce = tdInfo.GetDamageForce().Normalized();
	vNewDamageForce.z += 0.35;
	vNewDamageForce *= flDamageMagnitude;

	tdInfo.SetDamageForce(vNewDamageForce);

	VPhysicsTakeDamage(tdInfo);

	// manually sets the spin to a more pleasant looking spin
	AngularImpulse	angVel;
	angVel.Random(-500, 500);
	VPhysicsGetObject()->SetVelocity(NULL, &angVel);

	return BaseClass::OnTakeDamage_Alive(info);
}

//---------------------------------------------------------
// Purpose: Death Event Handing.
//---------------------------------------------------------
void CNPC_Protozoan::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );
	StopSound(entindex(), CHAN_BODY, "NPC_Protozoan.Idle");
	m_flTimeOfDeath = gpGlobals->curtime;

	// Tell the client that we're done.
	m_bDispatchDeathVFX = true;
	SetModelScale(0.0, PROTOZOAN_DEATH_SEQUENCE_DURATION);
	SetSolid(SOLID_NONE);

	if (AI_GetSinglePlayer())
		AI_GetSinglePlayer()->SetXenHealing(false);

	EmitSound("NPC_Protozoan.Die");
}

IMotionEvent::simresult_e CNPC_Protozoan::Simulate(IPhysicsMotionController* pController, IPhysicsObject* pObject, float deltaTime, Vector& linear, AngularImpulse& angular)
{
	if (m_flTimeOfDeath) // if protozoan hasnt died yet, assume this'll be 0.0f
	{
		float flDeathSequencePlayback = (gpGlobals->curtime - m_flTimeOfDeath) / PROTOZOAN_DEATH_SEQUENCE_DURATION;
		float flAnimSpeed = 1.0f + (PROTOZOAN_DEATH_FINAL_ANIM_SPEED - 1.0f) * flDeathSequencePlayback;
		SetPlaybackRate(flAnimSpeed);

		if (flDeathSequencePlayback >= 1.0f)
		{
			if (AI_GetSinglePlayer())
				AI_GetSinglePlayer()->SetXenHealing(false);

			UTIL_Remove(this);
		}
	}
	return BaseClass::Simulate(pController, pObject, deltaTime, linear, angular);
}

void CNPC_Protozoan::ClampMotorForces(Vector& linear, AngularImpulse& angular)
{
	if (g_debug_protozoan_pathing.GetBool())
		return;

	linear.x = clamp(linear.x, -750, 750);
	linear.y = clamp(linear.y, -750, 750);
	linear.z = clamp(linear.z, -750, 750);

	angular.x = clamp(angular.x, -500, 500);
	angular.y = clamp(angular.y, -500, 500);
	angular.z = clamp(angular.z, -500, 500);
}

void CNPC_Protozoan::OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason)
{
	// do this to prevent collision with the player when being held with the physgun
	m_hPrevOwner.Set(GetOwnerEntity());
	SetOwnerEntity(pPhysGunUser);
}

void CNPC_Protozoan::OnPhysGunDrop(CBasePlayer* pPhysGunUser, PhysGunDrop_t Reason)
{
	SetOwnerEntity(m_hPrevOwner);
	m_hPrevOwner = NULL;
}

int CNPC_Protozoan::TranslateSchedule(int scheduleType)
{
	return BaseClass::TranslateSchedule(scheduleType);
}

void CNPC_Protozoan::GatherConditions()
{
	ClearCondition(COND_PROTOZAN_OUTSIDE_MAX_WANDER_RANGE);
	ClearCondition(COND_PROTOZAN_TARGET_OUTSIDE_MAX_PERSUE_RANGE);
	ClearCondition(COND_PROTOZAN_SHOULD_IDLE);

	BaseClass::GatherConditions();

	float flDistToSpawn = (m_vSpawnLocation - GetAbsOrigin()).Length();

	if (flDistToSpawn > GetMaxWanderDistance())
		SetCondition(COND_PROTOZAN_OUTSIDE_MAX_WANDER_RANGE);

	if (GetEnemy())
	{
		float flDistToTarget = (m_vSpawnLocation - GetEnemy()->GetAbsOrigin()).Length();
		if (flDistToTarget > GetMaxPersueDistance())
			SetCondition(COND_PROTOZAN_TARGET_OUTSIDE_MAX_PERSUE_RANGE);
	}

	if (GetMaxWanderDistance() < 64.0f)
		SetCondition(COND_PROTOZAN_SHOULD_IDLE);
}

int CNPC_Protozoan::SelectSchedule(void)
{
	if (HasCondition(COND_PROTOZOAN_FORCE_MOVE))
	{
		return SCHED_PROTOZOAN_FORCE_MOVE_TO_TARGET;
		ClearCondition(COND_PROTOZOAN_FORCE_MOVE);
	}

	if (GetEnemy())
	{
		if (HasCondition(COND_PROTOZAN_TARGET_OUTSIDE_MAX_PERSUE_RANGE))
		{
			// theyre outside my range, forget about persuing them
			GetEnemies()->ClearMemory(GetEnemy());
		}
		else
		{
			return SCHED_PROTOZOAN_APPROACH_TARGET;
		}
	}

	if (HasCondition(COND_PROTOZAN_OUTSIDE_MAX_WANDER_RANGE))
		return SCHED_PROTOZOAN_RETURN_TO_ORIGIN;

	if (HasCondition(COND_PROTOZAN_SHOULD_IDLE))
		return SCHED_PROTOZOAN_IDLE;

	return SCHED_PROTOZOAN_WANDER;
}

void CNPC_Protozoan::OnStartSchedule(int scheduleType)
{
	BaseClass::OnStartSchedule(scheduleType);
}

void CNPC_Protozoan::StartTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_PROTOZOAN_GET_PATH_TO_CONTEXTUAL_TARGET:
	{
		if (m_hForcedEntity.Get())
		{
			if (IsUnreachable(GetEnemy()))
			{
				TaskFail(FAIL_NO_ROUTE);
				return;
			}

			CBaseEntity *pTarget = m_hForcedEntity.Get();

			if (pTarget == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}

			SetEnemy(m_hForcedEntity.Get());
			if ( GetNavigator()->SetGoal( GOALTYPE_ENEMY ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =( 
				TaskFail(FAIL_NO_ROUTE);
			}
			break;
		}
		// fallthrough here is intentional
		// if there is no forced entity it will get the path to its spawn instead
	}
	case TASK_PROTOZOAN_GET_PATH_TO_SPAWN:
	{
		GetNavigator()->SetGoal(m_vSpawnLocation);
		TaskComplete();
		return;
	}
	case TASK_PROTOZOAN_FORGET_ENEMIES:
	{
		AIEnemiesIter_t iter;
		for (AI_EnemyInfo_t* pEMemory = GetEnemies()->GetFirst(&iter); pEMemory != NULL; pEMemory = GetEnemies()->GetNext(&iter))
		{
			CBaseEntity* pEnemy = pEMemory->hEnemy;
			if (pEnemy)
			{
				GetEnemies()->ClearMemory(pEnemy);
			}
		}
		m_hForcedEntity.Set(NULL);

		TaskComplete();
		break;
	}
	case TASK_WANDER:
	{
		{
			if (GetNavigator()->SetWanderGoal(0.0f, GetMaxWanderDistance()))
				TaskComplete();
			else
				TaskFail(FAIL_NO_REACHABLE_NODE);
		}
		break;
	}
	default:
	{
		BaseClass::StartTask(pTask);
	}
	}
}

void CNPC_Protozoan::RunTask(const Task_t* pTask)
{
	BaseClass::RunTask(pTask);
}

void CNPC_Protozoan::NPCThink()
{
	BaseClass::NPCThink();

	// done here because the sound stops playing after saving and loading
	if (!m_bIsPlayingIdleSound)
	{
		EmitSound("NPC_Protozoan.Idle", entindex());
		m_bIsPlayingIdleSound = true;
	}

	SetNextThink(gpGlobals->curtime + 0.05);

	bool bHasHealedEntity = false;

	// heal nearby player
	if (gpGlobals->curtime >= m_flNextHealCheck)
	{
		bool bHealedPlayer = false;

		m_bIsHealing = false;
		CBaseEntity* pEntity = NULL;
		for (CEntitySphereQuery sphere(GetAbsOrigin(), GetHealRange()); (pEntity = sphere.GetCurrentEntity()) != nullptr; sphere.NextEntity())
		{
			if (pEntity->GetFlags() & FL_NOTARGET)
				continue;

			int iRelationship = IRelationType(pEntity);
			if (iRelationship != D_HT)
				continue;

			if (pEntity == AI_GetSinglePlayer())
			{
				bHealedPlayer = true;
				AI_GetSinglePlayer()->SetXenHealing( true );
			}

			bHasHealedEntity = pEntity->TakeHealth(GetHealAmount(), DMG_GENERIC) != 0 || bHasHealedEntity;
			m_OnHealEntityOutput.Set(pEntity, pEntity, this);
		}
		m_bIsHealing = bHasHealedEntity;

		if (!bHealedPlayer && AI_GetSinglePlayer())
		{
			AI_GetSinglePlayer()->SetXenHealing( false );
		}

		// the sphere trace is expensive, so it should ideally be performed sparingly
		// ideally it should only be done when we can comfortably assume we will heal the player in the trace.
		// if the player was within range last time we checked, we should be able to check again quickly
		// if they werent in the trace, we will wait longer before the next trace since repeated checks are less likely to succeed immediately
		m_flNextHealCheck = gpGlobals->curtime + (bHasHealedEntity ? GetHealInterval() : 2.5);
	}

	// do heal sound
	if (m_bIsHealing)
	{
		if (gpGlobals->curtime >= m_flNextHealSoundTime)
		{
			//EmitSound("NPC_Protozoan.Speak");
			m_flNextHealSoundTime = gpGlobals->curtime + random->RandomFloat(1.5f, 3.0f);
		}
	}
}

int CNPC_Protozoan::SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode)
{
	//DevMsg("failed sched: %i\n", failedSchedule);
	//DevMsg("failed task: %i\n", failedTask);
	//DevMsg("failed code: %i\n", taskFailCode);
	return BaseClass::SelectFailSchedule(failedSchedule, failedTask, taskFailCode);
}


AI_BEGIN_CUSTOM_NPC(npc_protozoan, CNPC_Protozoan)

DECLARE_TASK(TASK_PROTOZOAN_GET_PATH_TO_SPAWN)
DECLARE_TASK(TASK_PROTOZOAN_GET_PATH_TO_CONTEXTUAL_TARGET)
DECLARE_TASK(TASK_PROTOZOAN_FORGET_ENEMIES)

DECLARE_CONDITION(COND_PROTOZOAN_FLY_CLEAR)
DECLARE_CONDITION(COND_PROTOZOAN_FLY_BLOCKED)
DECLARE_CONDITION(COND_PROTOZOAN_GRABBED_BY_PHYSCANNON)
DECLARE_CONDITION(COND_PROTOZOAN_FORCE_MOVE)
DECLARE_CONDITION(COND_PROTOZAN_OUTSIDE_MAX_WANDER_RANGE)
DECLARE_CONDITION(COND_PROTOZAN_TARGET_OUTSIDE_MAX_PERSUE_RANGE)
DECLARE_CONDITION(COND_PROTOZAN_SHOULD_IDLE)

DEFINE_SCHEDULE
(
	SCHED_PROTOZOAN_RETURN_TO_ORIGIN,

	"	Tasks"
	"		 TASK_SET_TOLERANCE_DISTANCE		512"
	"		 TASK_PROTOZOAN_GET_PATH_TO_SPAWN	0"
	"		 TASK_RUN_PATH						0"
	"		 TASK_WAIT_FOR_MOVEMENT				0"
	""
	""
	"	Interrupts"
	"		COND_GIVE_WAY"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_PLAYER"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_PROVOKED"
	"		COND_PROTOZOAN_GRABBED_BY_PHYSCANNON"
	"		COND_PROTOZOAN_FORCE_MOVE"
)

DEFINE_SCHEDULE
(
	SCHED_PROTOZOAN_FORCE_MOVE_TO_TARGET,

	"	Tasks"
	"		 TASK_SET_TOLERANCE_DISTANCE					512"
	"		 TASK_PROTOZOAN_GET_PATH_TO_CONTEXTUAL_TARGET	0"
	"		 TASK_RUN_PATH									0"
	"		 TASK_WAIT_FOR_MOVEMENT							0"
	"		 TASK_PROTOZOAN_FORGET_ENEMIES					0"
	""
	""
	"	Interrupts"
	"		COND_PROTOZOAN_FORCE_MOVE"
)

DEFINE_SCHEDULE
(
	SCHED_PROTOZOAN_APPROACH_TARGET,

	"	Tasks"
	"		 TASK_SET_TOLERANCE_DISTANCE		128"
	"		 TASK_GET_PATH_TO_ENEMY				0"
	"		 TASK_RUN_PATH						0"
	"		 TASK_WAIT_FOR_MOVEMENT				0"
	""
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LOST_ENEMY"
	"		COND_PROTOZOAN_GRABBED_BY_PHYSCANNON"
	"		COND_PROTOZOAN_FORCE_MOVE"
	"		COND_PROTOZAN_TARGET_OUTSIDE_MAX_PERSUE_RANGE"
)

DEFINE_SCHEDULE
(
	SCHED_PROTOZOAN_WANDER,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_PROTOZOAN_IDLE"
	"		TASK_SET_TOLERANCE_DISTANCE			128"
	"		TASK_SET_ROUTE_SEARCH_TIME			5"
	"		TASK_WANDER							0"
	"		TASK_RUN_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_WAIT_RANDOM					2"
	""
	"	Interrupts"
	"		COND_GIVE_WAY"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_PLAYER"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_PROVOKED"
	"		COND_PROTOZOAN_GRABBED_BY_PHYSCANNON"
	"		COND_PROTOZOAN_FORCE_MOVE"
	"		COND_PROTOZAN_OUTSIDE_MAX_WANDER_RANGE"
)

DEFINE_SCHEDULE
(
	SCHED_PROTOZOAN_IDLE,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	""
	"	Interrupts"
	"		COND_GIVE_WAY"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_PLAYER"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_PROVOKED"
	"		COND_PROTOZOAN_GRABBED_BY_PHYSCANNON"
	"		COND_PROTOZOAN_FORCE_MOVE"
	"		COND_PROTOZAN_OUTSIDE_MAX_WANDER_RANGE"
)

AI_END_CUSTOM_NPC()
