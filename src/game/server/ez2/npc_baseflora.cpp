//=============================================================================//
//
// Purpose:		Base class for plant NPCs, specifically Xen plants
//
// Author:		1upD
//
//=============================================================================//

#include "cbase.h"
#include "npc_baseflora.h"
#include "ai_senses.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CNPC_BaseFlora )
	DEFINE_FIELD( m_hLastAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flAnimSpeedOffset, FIELD_FLOAT ),
END_DATADESC()


#define XENFLORA_RETHINK_INTERVAL 1.0f

ConVar sk_xenflora_health( "sk_xenflora_health", "1000" );

void CNPC_BaseFlora::Spawn()
{
	// Add some variation because we're often in groups
	m_flAnimSpeedOffset = random->RandomFloat(0.8f, 1.2f);

	Precache();

	SetModel( STRING( GetModelName() ) );

	SetHullType( HULL_HUMAN );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_NONE );

#ifdef EZ
	if (m_tEzVariant == EZ_VARIANT_RAD)
	{
		SetBloodColor( BLOOD_COLOR_BLUE );
	}
	else
#endif
	{
		SetBloodColor( BLOOD_COLOR_GREEN );
	}

	SetRenderColor( 255, 255, 255, 255 );

	m_iMaxHealth		= sk_xenflora_health.GetFloat();
	m_iHealth			= m_iMaxHealth;
	m_flFieldOfView = GetFieldOfView();
	m_NPCState			= NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_SQUAD );

	SetViewOffset( Vector( 0, 0, 64.0f ) );

	NPCInit();

	// NOTE: This must occur *after* init, since init sets default dist look
	m_flDistTooFar = GetViewDistance();
	SetDistLook( m_flDistTooFar );

	m_flNextTriggerCheck = gpGlobals->curtime + XENFLORA_RETHINK_INTERVAL;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_BaseFlora::PostNPCInit()
{
	BaseClass::PostNPCInit();
	SetPlaybackRate(m_flAnimSpeedOffset);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_BaseFlora::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );

#ifdef EZ
	if (m_tEzVariant == EZ_VARIANT_RAD)
	{
		PrecacheParticleSystem( "blood_impact_blue_01" );
	}
	else
#endif
	{
		PrecacheParticleSystem( "blood_impact_yellow_01" );
	}

	BaseClass::Precache();
}

Disposition_t CNPC_BaseFlora::IRelationType( CBaseEntity * pTarget )
{
	CAI_BaseNPC * pOther = pTarget == NULL ? NULL : pTarget->MyNPCPointer();
	if ( pOther != NULL && this->m_pSquad != NULL && this->m_pSquad == pOther->GetSquad() )
	{
		return D_LI;
	}

	return BaseClass::IRelationType( pTarget );
}

//-----------------------------------------------------------------------------
// Purpose: Ensures that we know who attacked us
//-----------------------------------------------------------------------------
int CNPC_BaseFlora::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( !BaseClass::OnTakeDamage_Alive( info ) )
		return 0;

	m_hLastAttacker = info.GetAttacker();
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_BaseFlora::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	// Don't interrupt attack schedules
	if ( IsCurSchedule ( SCHED_MELEE_ATTACK1 ) )
	{
		ClearCustomInterruptCondition( COND_NEW_ENEMY );
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
		ClearCustomInterruptCondition( COND_NEW_ENEMY );
		ClearCustomInterruptCondition( COND_ENEMY_DEAD );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
		ClearCustomInterruptCondition( COND_ENEMY_OCCLUDED );
	}
}

//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards.
// Xen flora responds to combat and danger sounds
//=========================================================
int CNPC_BaseFlora::GetSoundInterests(void)
{
	BaseClass::GetSoundInterests();
	return	SOUND_WORLD |
		SOUND_COMBAT |
		SOUND_BULLET_IMPACT |
		SOUND_DANGER;
}

void CNPC_BaseFlora::GatherConditions(void)
{
	//clear custom condition
	ClearCondition(COND_XENFLORA_SHOULD_REACT);

	int bits_EnvironmentResponse = GetEnvironmentalResponse();
	if (bits_EnvironmentResponse & StimulusMask())
	{
		SetCondition(COND_XENFLORA_SHOULD_REACT);
	}

	BaseClass::GatherConditions();
}

int CNPC_BaseFlora::GetEnvironmentalResponse()
{
	int result = bits_REACT_XENFLORA_NO_REPONSE;

	// Check if a player or other NPC has entered our space
	// We don't check this every think as this could become too intensive for something that is essentially a world detail.
	if (gpGlobals->curtime > m_flNextTriggerCheck)
	{
		m_bRecentlyDetectedHostile = false;
		CBaseEntity* list[1024];
		int nCount = UTIL_EntitiesInSphere(list, 1024, GetAbsOrigin(), GetReactionDistance(), FL_NPC | FL_CLIENT);

		if (nCount > 0)
		{
			for (int i = 0; i < nCount; i++)
			{
				CBaseEntity* pEntity = list[i];

				if (pEntity->GetFlags() & FL_NOTARGET)
					continue;

				if (IRelationType(pEntity) == D_HT)
				{
					m_bRecentlyDetectedHostile = true;
					SetEnemy(pEntity);
					UpdateEnemyMemory(GetEnemy(), GetEnemy()->GetAbsOrigin(), this);
					break;
				}
			}
		}

		m_flNextTriggerCheck = gpGlobals->curtime + XENFLORA_RETHINK_INTERVAL;
	}

	if (m_bRecentlyDetectedHostile)
	{
		result |= bits_REACT_XENFLORA_ENTITY_APPROACH;
	}

	if (HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE))
		result |= bits_REACT_XENFLORA_HURT;

	if (HasCondition(COND_HEAR_DANGER))
	{
		if (!GetEnemy())
		{
			// Assign m_hLastAttacker to the source of the danger
			CSound* pSound = GetBestSound(SOUND_DANGER);
			if (pSound)
				m_hLastAttacker = pSound->m_hOwner;
		}

		result |= bits_REACT_XENFLORA_HEAR_DANGER;
	}

	if (HasCondition(COND_HEAR_BULLET_IMPACT))
	{
		if (!GetEnemy())
		{
			// Assign m_hLastAttacker to the source of the impact
			CSound* pSound = GetBestSound(SOUND_BULLET_IMPACT);
			if (pSound)
				m_hLastAttacker = pSound->m_hOwner;
		}

		result |= bits_REACT_XENFLORA_NEARBY_GUNSHOT;
	}

	if (GetEnemy())
	{
		Vector vecLOS = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin());
		float  flDist = vecLOS.Length();
		vecLOS.z = 0;
		VectorNormalize(vecLOS);

		Vector vBodyDir = BodyDirection2D();
		float  flDot = DotProduct(vecLOS, vBodyDir);

		if (flDist <= GetViewDistance() && flDot > GetFieldOfView())
		{
			result |= bits_REACT_XENFLORA_ENEMY_IN_HIT_RANGE;
		}
	}

	return result;
}

//=========================================================
// Purpose: Returns the entity which caused us to retract.
//=========================================================
CBaseEntity *CNPC_BaseFlora::GetRetractActivator()
{
	CBaseEntity *pActivator = GetEnemy();
	if (!pActivator)
		pActivator = m_hLastAttacker;

	// As a failsafe, assign ourselves
	if (!pActivator)
		pActivator = this;

	return pActivator;
}

void CNPC_BaseFlora::StartTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_XENFLORA_IDLE_LOOP:
	{
		m_flNextEnvironmentCheck = gpGlobals->curtime + 0.5;
		break;
	}
	default:
	{
		BaseClass::StartTask(pTask);
	}
	}
}

int CNPC_BaseFlora::SelectSchedule(void)
{
	if (HasCondition(COND_XENFLORA_SHOULD_REACT))
	{
		return SCHED_MELEE_ATTACK1;
	}

	return SCHED_XENFLORA_IDLE;
}

void CNPC_BaseFlora::RunTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_XENFLORA_IDLE_LOOP:
	{
		if (gpGlobals->curtime >= m_flNextEnvironmentCheck)
		{
			TaskComplete();
		}
		break;
	}
	default:
	{
		BaseClass::RunTask(pTask);
	}
	}
}

void CNPC_BaseFlora::OnChangeActivity(Activity eNewActivity)
{
	BaseClass::OnChangeActivity(eNewActivity);

	if (eNewActivity == ACT_IDLE)
	{
		SetPlaybackRate(m_flAnimSpeedOffset);
	}
}

AI_BEGIN_CUSTOM_NPC(npc_baseflora, CNPC_BaseFlora)

DECLARE_CONDITION(COND_XENFLORA_SHOULD_REACT);
DECLARE_TASK(TASK_XENFLORA_IDLE_LOOP);

//DEFINE_SCHEDULE
//(
//	SCHED_XENFLORA_IDLE,
//
//	"	Tasks"
//	"		TASK_WAIT_INDEFINITE	0"
//	""
//	"	Interrupts"
//	"		COND_XENFLORA_SHOULD_REACT"
//);
DEFINE_SCHEDULE
(
	SCHED_XENFLORA_IDLE,

	"	Tasks"
	"		TASK_XENFLORA_IDLE_LOOP	0"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
	""
	"	Interrupts"
	"		COND_XENFLORA_SHOULD_REACT"
);

AI_END_CUSTOM_NPC()