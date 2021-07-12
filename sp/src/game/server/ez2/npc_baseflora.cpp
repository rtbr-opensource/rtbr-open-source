//=============================================================================//
//
// Purpose:		Base class for plant NPCs, specifically Xen plants
//
// Author:		1upD
//
//=============================================================================//

#include "cbase.h"
#include "npc_baseflora.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CNPC_BaseFlora )
	DEFINE_KEYFIELD( m_bInvincible, FIELD_BOOLEAN, "invincible" ),
	DEFINE_KEYFIELD( m_bIsRetracted, FIELD_BOOLEAN, "retracted" ),

	DEFINE_OUTPUT( m_OnRise, "OnRise" ),
	DEFINE_OUTPUT( m_OnLower, "OnLower" ),
END_DATADESC()

ConVar sk_xenflora_health( "sk_xenflora_health", "1000" );

void CNPC_BaseFlora::Spawn()
{
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

	if (IsCurSchedule ( SCHED_IDLE_STAND ) )
	{
		SetCustomInterruptCondition( COND_FLORA_EXTEND );
		SetCustomInterruptCondition( COND_FLORA_RETRACT );
	}
}

//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards.
// Xen flora responds to combat and danger sounds
//=========================================================
int CNPC_BaseFlora::GetSoundInterests ( void )
{
	BaseClass::GetSoundInterests();
	return	SOUND_WORLD	|
		SOUND_COMBAT	|
		SOUND_BULLET_IMPACT |
		SOUND_DANGER;
}

//=========================================================
// Gather custom conditions
//=========================================================
void CNPC_BaseFlora::GatherConditions( void )
{
	// Clear custom conditions
	ClearCondition( COND_FLORA_EXTEND );
	ClearCondition( COND_FLORA_RETRACT );

	// Base condition gathering
	BaseClass::GatherConditions();

	// Did a sound or event startle the plant?
	bool hasStartleCondition = HasStartleCondition();

	if (!IsRetracted() && hasStartleCondition)
	{
		SetCondition( COND_FLORA_RETRACT );
	}

	switch (m_NPCState)
	{
	case NPC_STATE_COMBAT:
		if ( !IsRetracted() && HasCondition( COND_SEE_ENEMY ) )
		{
			SetCondition( COND_FLORA_RETRACT );
		}
		if ( IsRetracted() && !hasStartleCondition && HasCondition( COND_ENEMY_TOO_FAR ) )
		{
			SetCondition( COND_FLORA_EXTEND );
		}
		break;
	default:
		if ( IsRetracted() && !hasStartleCondition )
		{
			SetCondition( COND_FLORA_EXTEND );
		}
	}
}

//=========================================================
// GetSchedule 
//=========================================================
int CNPC_BaseFlora::SelectSchedule( void )
{
	if ( HasCondition( COND_FLORA_RETRACT ) )
	{
		return SelectRetractSchedule();
	}

	if (HasCondition( COND_FLORA_EXTEND ) )
	{
		return SelectExtendSchedule();
	}

	return SCHED_IDLE_STAND;
}

//=========================================================
// Purpose: Returns true if some condition caused the flora
//	to react, even without the presence of an enemy.
//=========================================================
bool CNPC_BaseFlora::HasStartleCondition()
{
	return HasCondition( COND_HEAR_DANGER ) || HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE );
}

AI_BEGIN_CUSTOM_NPC( npc_baseflora, CNPC_BaseFlora )

DECLARE_CONDITION( COND_FLORA_EXTEND )
DECLARE_CONDITION( COND_FLORA_RETRACT )

AI_END_CUSTOM_NPC()