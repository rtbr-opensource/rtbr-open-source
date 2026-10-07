//========= Copyright (c) RTBR Team, 2024 ============//
//
// Purpose: Xen Light
//
//====================================================//

#include "cbase.h"
#include "ez2/npc_baseflora.h"
#include "Sprite.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	LIGHTSTALK_MODEL			"models/props_xen/xen_light.mdl"
#define LIGHTSTALK_GLOW_SPRITE		"sprites/grubflare1.vmt"

Activity ACT_IDLE2;
Activity ACT_RETRACT;

// Spawnflags
#define SF_XENLIGHT_NO_DYN_LIGHT	( 1 << 15 )

class CNPC_XenLight : public CNPC_BaseFlora
{
	DECLARE_CLASS( CNPC_XenLight, CNPC_BaseFlora );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

public:
	
	virtual void	Precache( void );
	virtual void	Spawn( void );

	int				SelectSchedule();
	void			StartTask(const Task_t* pTask);
	void			RunTask(const Task_t* pTask);

protected:

	virtual float	GetReactionDistance() { return 128.0f; }

	virtual int		StimulusMask() {
		return bits_REACT_XENFLORA_ENTITY_APPROACH |
			bits_REACT_XENFLORA_HURT |
			bits_REACT_XENFLORA_HEAR_DANGER |
			bits_REACT_XENFLORA_NEARBY_GUNSHOT;
	}

	DEFINE_CUSTOM_AI;

	enum
	{
		SCHED_XENLIGHT_RISE = BaseClass::NEXT_SCHEDULE,
		SCHED_XENLIGHT_FALL,

		NEXT_SCHEDULE,
	};

	enum
	{
		TASK_XENLIGHT_RISE = BaseClass::NEXT_TASK,
		TASK_XENLIGHT_FALL,

		NEXT_TASK,
	};

	COutputEvent m_OnRise;
	COutputEvent m_OnLower;

private:

	// continuously react until this time
	float		m_flHideUntil;

	CHandle<CSprite>	m_pGlowSprite;
	float				m_flLastTouch;					// Used to keep track of when our 'zone' was last touched, so we can delay our light raise.

	// Are we extended or retracted?
	CNetworkVar(bool, m_bIsExtended);
	CNetworkVar( bool, m_bHasDlight );			// Tells the client if we need to allocate a dlight.
};

LINK_ENTITY_TO_CLASS( npc_xenlight, CNPC_XenLight );

//---------------------------------------------------------
// Networking
//---------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CNPC_XenLight, DT_NPC_XenLight )
	SendPropBool( SENDINFO( m_bIsExtended ) ),
	SendPropBool( SENDINFO( m_bHasDlight ) ),
END_SEND_TABLE()

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_XenLight )

	DEFINE_FIELD( m_bIsExtended, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_pGlowSprite, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastTouch, FIELD_TIME ),
	DEFINE_FIELD( m_flNextTriggerCheck, FIELD_TIME ),
	DEFINE_FIELD( m_bHasDlight, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flHideUntil, FIELD_EHANDLE ),

	DEFINE_OUTPUT(m_OnRise, "OnRise"),
	DEFINE_OUTPUT(m_OnLower, "OnLower"),

END_DATADESC()


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_XenLight::Precache( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		SetModelName( AllocPooledString( LIGHTSTALK_MODEL ) );
	}

	PrecacheModel( LIGHTSTALK_GLOW_SPRITE );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_XenLight::Spawn( void )
{
	Precache();

	m_bHasDlight = HasSpawnFlags( SF_XENLIGHT_NO_DYN_LIGHT ) ? false : true;

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_TRIGGER );
	SetMoveType( MOVETYPE_NONE );
	SetHullType( HULL_SMALL_CENTERED );
	
	m_bIsExtended = true;
	BaseClass::Spawn();

	// Create our glow sprite
	m_pGlowSprite = CSprite::SpriteCreate( LIGHTSTALK_GLOW_SPRITE, GetLocalOrigin() + Vector( 0, 0, ( WorldAlignMins().z + WorldAlignMaxs().z ) * 0.5 ), false );
	m_pGlowSprite->TurnOn();
	m_pGlowSprite->SetParent( this );
	m_pGlowSprite->SetAttachment( this, 1 );
	m_pGlowSprite->SetTransparency( kRenderWorldGlow, 254, 245, 218, 155, kRenderFxGlowShell );	// These colors come from map prefabs - could be map keyvalues if needed
	m_pGlowSprite->SetScale( 0.3f );
	m_pGlowSprite->SetGlowProxySize( 8.0f );

	SetActivity( ACT_IDLE );
}

int CNPC_XenLight::SelectSchedule( void )
{
	if (m_bIsExtended)
	{
		if (HasCondition(COND_XENFLORA_SHOULD_REACT))
		{
			m_flHideUntil = gpGlobals->curtime + RandomFloat(3.0f, 9.0f);
			return SCHED_XENLIGHT_FALL;
		}
	}
	else {
		if ( (gpGlobals->curtime > m_flHideUntil) && !HasCondition(COND_XENFLORA_SHOULD_REACT) )
			return SCHED_XENLIGHT_RISE;
	}
	
	return SCHED_XENFLORA_IDLE;
}

void CNPC_XenLight::StartTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_XENLIGHT_RISE:
	{
		SetActivity(ACT_DEPLOY);
		m_pGlowSprite->TurnOn();
		m_bIsExtended = true;

		m_OnRise.FireOutput(GetEnemy(), this);

		break;
	}
	case TASK_XENLIGHT_FALL:
	{
		m_bIsExtended = false;
		m_pGlowSprite->TurnOff();

		SetActivity(ACT_RETRACT);
		SetSkin(1);

		m_OnLower.FireOutput(GetEnemy(), this);

		break;
	}
	default:
		BaseClass::StartTask(pTask);
	}
}

void CNPC_XenLight::RunTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_XENLIGHT_RISE:
	{
		// switch activity when finished
		if (IsActivityFinished())
		{
			SetActivity(ACT_IDLE);
			TaskComplete();
		}

		break;
	}
	case TASK_XENLIGHT_FALL:
	{
		// switch activity when finished
		if (IsActivityFinished())
		{
			SetActivity(ACT_IDLE2);
			TaskComplete();
		}

		break;
	}
	default:
		BaseClass::RunTask(pTask);
	}
}

AI_BEGIN_CUSTOM_NPC(npc_xenlight, CNPC_XenLight)

DECLARE_ACTIVITY(ACT_IDLE2)
DECLARE_ACTIVITY(ACT_RETRACT)

DECLARE_TASK(TASK_XENLIGHT_RISE)
DECLARE_TASK(TASK_XENLIGHT_FALL)

DEFINE_SCHEDULE
(
	SCHED_XENLIGHT_RISE,

	"	Tasks"
	"		 TASK_XENLIGHT_RISE				0"
	""
	"	Interrupts"
	"		COND_XENFLORA_SHOULD_REACT"
)

DEFINE_SCHEDULE
(
	SCHED_XENLIGHT_FALL,

	"	Tasks"
	"		 TASK_XENLIGHT_FALL				0"
	""
	"	Interrupts"
)
AI_END_CUSTOM_NPC()