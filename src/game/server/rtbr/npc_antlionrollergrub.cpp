//========= Copyright (c) RTBR Team, 2024 ============//
//
// Purpose: Antlion Rollergrub
//
//====================================================//

#include "cbase.h"
#include "npc_antlionrollergrub.h"
#include "hl2_shareddefs.h"
#include "npcevent.h"
#include "saverestore_utlvector.h"
#include "movevars_shared.h"
#include "particle_parse.h"
#include "physics_prop_ragdoll.h"
#include "grenade_spit.h"

#include "npc_antlionbirther.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(npc_antlionrollergrub, CNPC_AntlionRollerGrub);

#define ANTLIONROLLERGRUB_MODEL "models/antlionrollergrub/antlionrollergrub.mdl"
#define ANTLIONROLLERGRUB_BURN_SOUND_FREQUENCY 10
#define ANTLIONROLLERGRUB_STEPSOUND_FREQUENCY_MIN 0.2
#define ANTLIONROLLERGRUB_STEPSOUND_FREQUENCY_MAX 0.3

#define ROLLERGRUB_EXPLODE_EFFECT		"AntlionGib"
#define ROLLERGRUB_EXPLODE_SOUND		"NPC_Antlion_Grub.Explode"

ConVar sk_antlionrollergrub_health("sk_antlionrollergrub_health", "6");

// Animation Events
// Rollergrub
static int AE_ROLLERGRUB_STARTFLING;

// Activities
// Rollergrub
static Activity ACT_ROLLERGRUB_ENTERFLING;
static Activity ACT_ROLLERGRUB_FLING;
static Activity ACT_ROLLERGRUB_EXITFLING;

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_AntlionRollerGrub)

DEFINE_FIELD(m_bFlinging, FIELD_BOOLEAN),
DEFINE_FIELD(m_hMommaBirther, FIELD_EHANDLE)

END_DATADESC()

//---------------------------------------------------------
// Purpose: Precache needed resources
//---------------------------------------------------------
void CNPC_AntlionRollerGrub::Precache(void)
{
	PrecacheModel(ANTLIONROLLERGRUB_MODEL);

	PrecacheScriptSound("NPC_Antlion_Rollergrub.Footstep");
	PrecacheScriptSound("NPC_Antlion_Rollergrub.MeleeAttack");
	PrecacheScriptSound("NPC_Antlion_Rollergrub.Idle");
	PrecacheScriptSound("NPC_Antlion_Rollergrub.Pain");
	PrecacheScriptSound("NPC_Antlion_Rollergrub.Death");
	PrecacheScriptSound("NPC_Antlion_Rollergrub.IdleAngry");

	PrecacheScriptSound(ROLLERGRUB_EXPLODE_SOUND);
	PrecacheParticleSystem(ROLLERGRUB_EXPLODE_EFFECT);

	BaseClass::Precache();
}

//---------------------------------------------------------
// Purpose: Spawn entity into world
//---------------------------------------------------------
void CNPC_AntlionRollerGrub::Spawn(void)
{
	Precache();

	BaseClass::Spawn();

	SetModel( ANTLIONROLLERGRUB_MODEL );
	SetHullType( HULL_SMALL );
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);
	SetBloodColor(BLOOD_COLOR_GREEN); // TODO: Something else?

	m_iHealth = sk_antlionrollergrub_health.GetInt();
	m_flFieldOfView = -0.5;
	m_flNextFootstepSoundTime = 0;
	m_NPCState = NPC_STATE_NONE;

	m_bFlinging = false;

	CapabilitiesClear();
	CapabilitiesAdd(bits_CAP_MOVE_GROUND);
	//CapabilitiesAdd( bits_CAP_MOVE_JUMP );
	CapabilitiesAdd(bits_CAP_INNATE_RANGE_ATTACK1 /* | bits_CAP_INNATE_MELEE_ATTACK1 */);
	CapabilitiesAdd(bits_CAP_SQUAD);

	// make sure they dont collide with each other
	SetCollisionGroup(HL2COLLISION_GROUP_ANTLION_ROLLERGRUB);

	SetNavType(NAV_GROUND);
	SetMoveType(MOVETYPE_STEP);

	NPCInit();
}

void CNPC_AntlionRollerGrub::NPCThink(void)
{
	if (m_flGroundSpeed != 0.0f)
	{
		if (gpGlobals->curtime >= m_flNextFootstepSoundTime)
		{
			EmitSound("NPC_Antlion_Rollergrub.Footstep");

			float flStepInterval = random->RandomFloat(
				ANTLIONROLLERGRUB_STEPSOUND_FREQUENCY_MIN,
				ANTLIONROLLERGRUB_STEPSOUND_FREQUENCY_MAX
			);
			m_flNextFootstepSoundTime = gpGlobals->curtime + flStepInterval;
		}
	}
	BaseClass::NPCThink();
}
 
void CNPC_AntlionRollerGrub::IdleSound(void)
{
	EmitSound("NPC_Antlion_Rollergrub.Idle");
}

void CNPC_AntlionRollerGrub::AlertSound(void)
{
	EmitSound("NPC_Antlion_Rollergrub.IdleAngry");
}

void CNPC_AntlionRollerGrub::PainSound(const CTakeDamageInfo& info)
{
	if (IsOnFire() && random->RandomInt(0, ANTLIONROLLERGRUB_BURN_SOUND_FREQUENCY) > 0)
	{
		return;
	}

	EmitSound("NPC_Antlion_Rollergrub.Pain");
}

void CNPC_AntlionRollerGrub::DeathSound(const CTakeDamageInfo& info)
{
	EmitSound("NPC_Antlion_Rollergrub.Death");
}

void CNPC_AntlionRollerGrub::BiteSound(void)
{
	EmitSound("NPC_Antlion_Rollergrub.MeleeAttack");
}

// plays when the grub flings itself, *NOT* when it damages a target
void CNPC_AntlionRollerGrub::AttackSound(void)
{
	//TODO: use a more appropriate sound ?
	EmitSound("NPC_Antlion_Rollergrub.Pain");
}

//---------------------------------------------------------
// Purpose: Conditions for fling attack
//---------------------------------------------------------
int CNPC_AntlionRollerGrub::RangeAttack1Conditions(float flDot, float flDist)
{
	if ( flDist < 128.0f )
		return COND_TOO_CLOSE_TO_ATTACK;

	if ( flDist > 256.0f )
		return COND_TOO_FAR_TO_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}

void CNPC_AntlionRollerGrub::GatherConditions(void)
{
	if (random->RandomInt(0, 134) == 0 && m_NPCState == NPC_STATE_COMBAT)
		AlertSound();

	BaseClass::GatherConditions();
}

void CNPC_AntlionRollerGrub::PrescheduleThink()
{
	// drown
	if (GetWaterLevel() > 1 && m_lifeState == LIFE_ALIVE)
	{
		CTakeDamageInfo info;

		info.SetAttacker(this);
		info.SetInflictor(this);
		info.SetDamage(m_iHealth);
		info.SetDamageType(DMG_DROWN);
		info.SetDamageForce(Vector(0.1, 0.1, 0.1));

		TakeDamage(info);
	}
	BaseClass::PrescheduleThink();
}

//---------------------------------------------------------
// Purpose: Translate Schedule
//---------------------------------------------------------
int CNPC_AntlionRollerGrub::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_ROLLERGRUB_RANGE_ATTACK1;

	default:
		return BaseClass::TranslateSchedule(scheduleType);
	}
}

//---------------------------------------------------------
// Purpose: Schedule Selection
//---------------------------------------------------------
int CNPC_AntlionRollerGrub::SelectSchedule(void)
{
	if (m_bFlingedFromBirther)
	{
		m_bFlingedFromBirther = false;
		return SCHED_ROLLERGRUB_FLING_FROM_BIRTHER;
	}
	if (HasCondition(COND_NEW_ENEMY))
	{
		// squeek upon finding new enemy
		AlertSound();
	}

	switch (m_NPCState)
	{
		case NPC_STATE_ALERT:
		{
			if (HasCondition(COND_HEAR_COMBAT))
			{
				CSound* pSound = GetBestSound();

				if (pSound && pSound->IsSoundType(SOUND_COMBAT))
				{
					return SCHED_INVESTIGATE_SOUND;
				}
			}
			return SCHED_COMBAT_PATROL;
		}
		case NPC_STATE_IDLE:
			return SCHED_ROLLERGRUB_RELAXED_WANDER;

		case NPC_STATE_COMBAT:
		{
			if (HasCondition(COND_CAN_RANGE_ATTACK1))
				return SCHED_ROLLERGRUB_RANGE_ATTACK1;

			if (HasCondition(COND_TOO_CLOSE_TO_ATTACK))
				return SCHED_MOVE_AWAY_FROM_ENEMY;

			return SCHED_CHASE_ENEMY;
		}
	}

	return BaseClass::SelectSchedule();
}

//---------------------------------------------------------
// Purpose: Translate Activity
//---------------------------------------------------------
Activity CNPC_AntlionRollerGrub::TranslateActivity(Activity eNewActivity)
{
	if (eNewActivity == ACT_RANGE_ATTACK1)
		return ACT_ROLLERGRUB_ENTERFLING;

	return BaseClass::TranslateActivity(eNewActivity);
}

//---------------------------------------------------------
// Purpose: Translate Activity
//---------------------------------------------------------
void CNPC_AntlionRollerGrub::OnChangeActivity(Activity eNewActivity)
{
	if (eNewActivity == ACT_ROLLERGRUB_FLING)
	{
		// We need to set some state here.
		SetTouch(&CNPC_AntlionRollerGrub::TouchDamage);
		AttackSound();
		m_bFlinging = true;
	}
	if (eNewActivity == ACT_ROLLERGRUB_EXITFLING)
	{
		SetTouch(NULL);
		m_bFlinging = false;
	}

	BaseClass::OnChangeActivity(eNewActivity);
}

//---------------------------------------------------------
// Purpose: AnimEvent Handling
//---------------------------------------------------------
void CNPC_AntlionRollerGrub::HandleAnimEvent(animevent_t* pEvent)
{
	if (pEvent->event == AE_ROLLERGRUB_STARTFLING)
	{
		// dont fling unless on ground
		if (!(GetFlags() & FL_ONGROUND))
			return;

		Vector vecJumpVector = ConstructFlingVector(GetEnemy(), GetAbsOrigin());
		SetAbsVelocity(vecJumpVector);

		SetNextAttack(gpGlobals->curtime + 3.0f);
		return;
	}

	BaseClass::HandleAnimEvent(pEvent);
}

//---------------------------------------------------------
// Purpose: Task Handling
//---------------------------------------------------------
void CNPC_AntlionRollerGrub::StartTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
		case TASK_ROLLERGRUB_FLING:
			return;

		default:
			BaseClass::StartTask(pTask);
	}
}

//---------------------------------------------------------
// Purpose: Task Handling
//---------------------------------------------------------
void CNPC_AntlionRollerGrub::RunTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
		case TASK_ROLLERGRUB_FLING:
		{
			SetIdealActivity((Activity)ACT_ROLLERGRUB_FLING);

			// If we are on the ground, it is time to unroll!
			if ( (GetFlags() & FL_ONGROUND) )
				TaskComplete();
			break;
		}

		default:
			BaseClass::RunTask(pTask);
	}
}

//---------------------------------------------------------
// Purpose: Tell our birther that we're done
//---------------------------------------------------------
void CNPC_AntlionRollerGrub::Event_Killed(const CTakeDamageInfo& info)
{
	//DevMsg("Damage Type: %d\n", info.GetDamageType());
	//DevMsg("Damage Amount: %f\n", info.GetDamage());

	// used if the rollergrub should explode into goop instead of leaving a corpse behind
	bool bShouldExplode = false;

	// crowbar is more likely to cause explosion
	float flExplosionChanceThreshold = (info.GetDamageType() & DMG_CLUB) ? 10.0f : 25.0f;

	if (info.GetDamageType() & (DMG_BLAST | DMG_ALWAYSGIB | DMG_CRUSH))
		bShouldExplode = true;

	// if inflicted damage is high, there will be a greater chance of exploding
	else if ((RandomFloat(0., flExplosionChanceThreshold) < info.GetDamage()))
		bShouldExplode = true;

	// explode if hit with shotgun up close
	else if (info.GetDamageType() & DMG_BUCKSHOT)
		if ((info.GetAttacker()->GetAbsOrigin() - GetAbsOrigin()).Length() < 210.0f)
			bShouldExplode = true;

	if (bShouldExplode)
	{
		AddEffects(EF_NODRAW);
		AddSolidFlags(FSOLID_NOT_SOLID);
		EmitSound(ROLLERGRUB_EXPLODE_SOUND);
		DispatchParticleEffect(ROLLERGRUB_EXPLODE_EFFECT, GetAbsOrigin(), RandomAngle(0, 360));
	}

	if (m_hMommaBirther)
		m_hMommaBirther->Event_RollerGrubKilled(this);

	BaseClass::Event_Killed(info);
}

//---------------------------------------------------------
// Purpose: Set us up for instant flinging!
//---------------------------------------------------------
void CNPC_AntlionRollerGrub::FlingFromBirther(CNPC_AntlionBirther* pMommaBirther)
{
	m_bFlingedFromBirther = true;
	if (pMommaBirther && pMommaBirther->GetEnemy())
		SetEnemy(pMommaBirther->GetEnemy(), true);

	m_hMommaBirther = pMommaBirther;

	SetNextAttack(gpGlobals->curtime + 2.0f);

	// TODO: Some kind of bite sound here?
}

//---------------------------------------------------------
// Purpose: Release the reference to the dead birther.
//---------------------------------------------------------
void CNPC_AntlionRollerGrub::Event_BirtherKilled(void)
{
	if (m_hMommaBirther)
		m_hMommaBirther = NULL;
}

//---------------------------------------------------------
// Purpose: Deals the touch damage from the fling attack.
//---------------------------------------------------------
void CNPC_AntlionRollerGrub::TouchDamage(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer() && !pOther->IsNPC())
		return;

	CTakeDamageInfo info{ this, this, 5.0f, DMG_CLUB };
	CalculateMeleeDamageForce(&info, GetAbsVelocity(), GetAbsOrigin());
	pOther->TakeDamage(info);

	BiteSound();

	// Make sure we only execute this attack once per jump!
	SetTouch(NULL);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionRollerGrub::TraceAttack(const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr, CDmgAccumulator* pAccumulator)
{
	CTakeDamageInfo newInfo = info;
	
	// Ignore if we're in a dynamic scripted sequence
	if (info.GetDamageType() & DMG_PHYSGUN && !IsRunningDynamicInteraction())
	{
		Vector	puntDir = (info.GetDamageForce() * 1000.0f);

		newInfo.SetDamage(m_iMaxHealth / 2.0f);

		if (info.GetDamage() >= GetHealth())
		{
			// This blow will be fatal, so scale the damage force
			// (it's a unit vector) so that the ragdoll will be 
			// affected.
			newInfo.SetDamageForce(info.GetDamageForce() * 3000.0f);
		}

		PainSound(newInfo);
		SetGroundEntity(NULL);
		ApplyAbsVelocityImpulse(puntDir);
	}

	BaseClass::TraceAttack(newInfo, vecDir, ptr, pAccumulator);
}

//---------------------------------------------------------
// Purpose: Build a fling vector to our enemy. (Borrowed from Headcrab jump code)
//---------------------------------------------------------
Vector CNPC_AntlionRollerGrub::ConstructFlingVector(CBaseEntity* pEnemy, Vector vFrom)
{
	Vector vecDirection;
	// We should never get in here if we don't have an enemy
	// Just a sanity check
	if (!pEnemy)
	{
		Vector vecArbitraryDest;
		GetVectors(&vecDirection, NULL, NULL);
		VectorMA(vFrom, 10.0f, vecDirection, vecArbitraryDest);

		return vecArbitraryDest;
	}

	Vector jumpTarget = pEnemy->GetAbsOrigin() + (pEnemy->EyePosition() - pEnemy->GetAbsOrigin()) * 0.5f;

	vecDirection = vFrom - jumpTarget;
	DevMsg("vFrom: (%f, %f, %f)\n", vFrom.x, vFrom.y, vFrom.z);
	DevMsg("vecDirection: (%f, %f, %f)\n", vecDirection.x, vecDirection.y, vecDirection.z);

	Vector vecJumpVel;

	float gravity = GetCurrentGravity();

	float height = abs(jumpTarget.z - vFrom.z);

	float additionalHeight = 0.0f;
	height += additionalHeight;
	DevMsg("additionalHeight: (%f)\n", additionalHeight);

	float speed = sqrt(2 * gravity * height);
	DevMsg("height: (%f)\n", height);
	float flyTime = speed / gravity;

	DevMsg("speed: (%f)\n", speed);
	DevMsg("flytime: (%f)\n", flyTime);

	flyTime += sqrt((2 * additionalHeight) / gravity);

	VectorSubtract(jumpTarget, vFrom, vecJumpVel);

	DevMsg("flytime: (%f)\n", flyTime);
	vecJumpVel /= flyTime;

	vecJumpVel.z = speed;

	float flJumpSpeed = vecJumpVel.Length();
	float flMaxSpeed = 1000.0f;
	if (flJumpSpeed > flMaxSpeed)
	{
		vecJumpVel *= flMaxSpeed / flJumpSpeed;
	}

	DevMsg("Vector: (%f, %f, %f)\n", vecJumpVel.x, vecJumpVel.y, vecJumpVel.z);
	return vecJumpVel;
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC(npc_antlionrollergrub, CNPC_AntlionRollerGrub)

DECLARE_ANIMEVENT(AE_ROLLERGRUB_STARTFLING);

DECLARE_ACTIVITY(ACT_ROLLERGRUB_ENTERFLING);
DECLARE_ACTIVITY(ACT_ROLLERGRUB_FLING);
DECLARE_ACTIVITY(ACT_ROLLERGRUB_EXITFLING);

DECLARE_TASK(TASK_ROLLERGRUB_FLING);

DEFINE_SCHEDULE
(
	SCHED_ROLLERGRUB_RANGE_ATTACK1,
	"	Tasks"
	"		TASK_FACE_ENEMY					0"
	"		TASK_STOP_MOVING				0"
	"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_ROLLERGRUB_ENTERFLING"
	"		TASK_ROLLERGRUB_FLING			0"
	"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_ROLLERGRUB_EXITFLING"
	""
	"	Interrupts"
)

DEFINE_SCHEDULE
(
	SCHED_ROLLERGRUB_FLING_FROM_BIRTHER,
	"	Tasks"
	"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_ROLLERGRUB_FLING"
	"		TASK_ROLLERGRUB_FLING					0"
	"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_ROLLERGRUB_EXITFLING"
	""
	"	Interrupts"
)

DEFINE_SCHEDULE
(
	SCHED_ROLLERGRUB_RELAXED_WANDER,

	"	Tasks"
	//	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_SET_ROUTE_SEARCH_TIME		5"	// Spend 5 seconds trying to build a path if stuck
	"		TASK_GET_PATH_TO_RANDOM_NODE	200"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_WAIT_RANDOM				5"
	""
	"	Interrupts"
	"		COND_GIVE_WAY"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_DANGER"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_IDLE_INTERRUPT"
);

AI_END_CUSTOM_NPC()