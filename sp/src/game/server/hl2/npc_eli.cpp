//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Dr. Eli Vance, earths last great hope, single-handedly fighting
//			off both an evil alien invasion, as well as trying to stop 
//			that idiot lab assistant from putting the moves on his daughter.
//=============================================================================//


//-----------------------------------------------------------------------------
// Generic NPC - purely for scripted sequence work.
//-----------------------------------------------------------------------------
#include	"cbase.h"
#include	"npcevent.h"
#include	"ai_basenpc.h"
#include	"ai_hull.h"
#include "ai_baseactor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const int ELI_FOLLOW_DISTANCE_THRESHOLD = ( 164 * 164 );

//-----------------------------------------------------------------------------
// NPC's Anim Events Go Here
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_Eli : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CNPC_Eli, CAI_BaseActor );
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	void	Spawn( void );
	void	Precache( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests( void );
	void	SetupWithoutParent( void );

	void	PrescheduleThink( void );
	int		SelectSchedule( void );

	virtual void RunTask( const Task_t *pTask );

	// Input handlers
	void	InputActivateFollowBehvaior( inputdata_t &inputdata );
	void	InputDeactivateFollowBehvaior( inputdata_t &inputdata );

#ifdef MAPBASE
	// Use Eli's default subtitle color (255,208,172)
	bool	GetGameTextSpeechParams( hudtextparms_t &params ) { params.r1 = 255; params.g1 = 208; params.b1 = 172; return BaseClass::GetGameTextSpeechParams( params ); }
#endif

private:
	bool	m_bFollowBehvaiorActive;
	EHANDLE m_hFollowTarget;

private:
	enum
	{
		SCHED_ELI_FOLLOW_TARGET = BaseClass::NEXT_SCHEDULE
	};
};

LINK_ENTITY_TO_CLASS( npc_eli, CNPC_Eli );

BEGIN_DATADESC( CNPC_Eli )

	DEFINE_FIELD( m_bFollowBehvaiorActive,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hFollowTarget, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_STRING,		"ActivateFollowBehavior",	InputActivateFollowBehvaior ),
	DEFINE_INPUTFUNC( FIELD_STRING,		"DeactivateFollowBehavior",	InputDeactivateFollowBehvaior )

END_DATADESC()

//-----------------------------------------------------------------------------
// Classify - indicates this NPC's place in the 
// relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Eli::Classify ( void )
{
	return	CLASS_PLAYER_ALLY_VITAL;
}



//-----------------------------------------------------------------------------
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//-----------------------------------------------------------------------------
void CNPC_Eli::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//-----------------------------------------------------------------------------
// GetSoundInterests - generic NPC can't hear.
//-----------------------------------------------------------------------------
int CNPC_Eli::GetSoundInterests ( void )
{
	return	NULL;
}

//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CNPC_Eli::Spawn()
{
	// Eli is allowed to use multiple models, because he appears in the pod.
	// He defaults to his normal model.
	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		szModel = "models/eli.mdl";
		SetModelName( AllocPooledString(szModel) );
	}

	Precache();
	SetModel( szModel );

	BaseClass::Spawn();

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	// If Eli has a parent, he's currently inside a pod. Prevent him from moving.
	if ( GetMoveParent() )
	{
		SetSolid( SOLID_BBOX );
		AddSolidFlags( FSOLID_NOT_STANDABLE );
		SetMoveType( MOVETYPE_NONE );

		CapabilitiesAdd( bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );
		CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );
	}
	else
	{
		SetupWithoutParent();
	}

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= 8;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;

	NPCInit();
}

//-----------------------------------------------------------------------------
// Precache - precaches all resources this NPC needs
//-----------------------------------------------------------------------------
void CNPC_Eli::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Eli::SetupWithoutParent( void )
{
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );
}

// TODO: These should be refactored at some point.
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Eli::InputActivateFollowBehvaior( inputdata_t &inputdata )
{
	CBaseEntity *followTarget = gEntList.FindEntityByName( 0, inputdata.value.String(), this, this, this, 0 );

	if ( followTarget )
	{
		m_bFollowBehvaiorActive = true;
		m_hFollowTarget = followTarget;

		// Think immediately after this is fired - so that we begin moving right away
		SetNextThink( gpGlobals->curtime + 0.1f );

		return;
	}

	m_bFollowBehvaiorActive = false;
	DevMsg( "npc_eli->ActivateFollowBehavior requires a valid target to be passed in as a parameter\n" );
	return;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Eli::InputDeactivateFollowBehvaior( inputdata_t &inputdata )
{
	if ( !m_bFollowBehvaiorActive )
	{
		return;
	}

	if ( GetTarget() )
	{
		m_hFollowTarget = NULL;
		SetTarget( NULL );
		m_bFollowBehvaiorActive = false;

		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Eli::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	// Figure out if Eli has just been removed from his parent
	if ( GetMoveType() == MOVETYPE_NONE && !GetMoveParent() )
	{
		SetupWithoutParent();
		SetupVPhysicsHull();
	}
}

// TODO: These should be refactored at some point.
//-----------------------------------------------------------------------------
// Purpose: Schedule selection, overridden so Eli can have a custom follow schedule.
//-----------------------------------------------------------------------------
int CNPC_Eli::SelectSchedule( void )
{
	// If we're running a scene, don't let us follow a target.
	int nBaseSched = BaseClass::SelectSchedule();

	if ( nBaseSched == SCHED_SCENE_GENERIC )
	{
		DevWarning( "Eli is running a scene! Don't follow!\n" );
	}

#ifdef RTBR_DLL
	if ( m_bFollowBehvaiorActive && m_hFollowTarget && !IsCurSchedule( SCHED_ELI_FOLLOW_TARGET ) && nBaseSched != SCHED_SCENE_GENERIC )
	{
		if ( m_hFollowTarget->GetAbsOrigin().DistToSqr( GetAbsOrigin() ) > ELI_FOLLOW_DISTANCE_THRESHOLD )	// Don't follow if we're close enough.
		{
			SetTarget( m_hFollowTarget );

			// We need to think more often when we have a follow target... or else we'll lag behind
			SetNextThink( gpGlobals->curtime + 0.2f );

			return SCHED_ELI_FOLLOW_TARGET;
		}
	}
#endif

	return nBaseSched;
}

//-----------------------------------------------------------------------------
// Purpose: Task handling, overridden so Eli can have a custom follow schedule.
//-----------------------------------------------------------------------------
void CNPC_Eli::RunTask( const Task_t *pTask )
{
	if ( IsCurSchedule( SCHED_ELI_FOLLOW_TARGET ) && (pTask->iTask == TASK_RUN_PATH || pTask->iTask == TASK_WAIT_FOR_MOVEMENT) )
	{
		if ( m_hFollowTarget && m_hFollowTarget->GetAbsOrigin().DistToSqr( GetAbsOrigin() ) < ELI_FOLLOW_DISTANCE_THRESHOLD )
		{
			TaskComplete();
			return;
		}
	}

	BaseClass::RunTask( pTask );
}

//-----------------------------------------------------------------------------
// AI Schedules Specific to this NPC
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_eli, CNPC_Eli )

	DEFINE_SCHEDULE
			(
				// This is just SCHED_FOLLOW
				SCHED_ELI_FOLLOW_TARGET,
				"	Tasks"
				"		TASK_GET_PATH_TO_TARGET			0"
				"		TASK_RUN_PATH					0"
				"		TASK_WAIT_FOR_MOVEMENT			0"
				"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_TARGET_FACE "
				""
				"	Interrupts"
			)

AI_END_CUSTOM_NPC()