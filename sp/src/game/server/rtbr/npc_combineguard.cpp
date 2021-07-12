//========= Copyright © LOLOLOL, All rights reserved. ============//
//
// Purpose: A big heavily-armored monstrosity who works for the combine and carries the combine 
// equivalent to the BFG.
//			TODO: 
//				Clean out all the obsolete/unused code
//				Berserk/Melee behavior during guard gun cooldown
//				Fix issue where he can't shoot up or down
//				Secondary Range Attack?
//=============================================================================//


#include "cbase.h"
#include "AI_Task.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Motor.h"
#include "AI_Memory.h"
#include "npc_combine.h"
#include "physics.h"
#include "bitstring.h"
#include "activitylist.h"
#include "game.h"
#include "NPCEvent.h"
#include "Player.h"
#include "EntityList.h"
#include "AI_Interactions.h"
#include "soundent.h"
#include "Gib.h"
#include "shake.h"
#include "ammodef.h"
#include "Sprite.h"
#include "explode.h"
#include "grenade_homer.h"
#include "AI_BaseNPC.h"
#include "soundenvelope.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "te_particlesystem.h"

#define CGUARD_GIB_COUNT			5   //Obsolete, but leaving it here anyway for no reason

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_combineguard_health( "sk_combineguard_health", "800" );

class CSprite;

extern void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );

#define	COMBINEGUARD_MODEL	"models/combine_guard.mdl"

#define CGUARD_MSG_SHOT	1
#define CGUARD_MSG_SHOT_START 2

#define		GUARD_BODY_GUNDRAWN		0
#define		GUARD_BODY_GUNGONE			1


class CNPC_CombineGuard : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_CombineGuard, CAI_BaseNPC );
public:


	CNPC_CombineGuard( void );

//	int	OnTakeDamage( const CTakeDamageInfo &info );
	int	OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	int	TranslateSchedule( int type );
	int	MeleeAttack1Conditions( float flDot, float flDist );
	int	RangeAttack1Conditions( float flDot, float flDist );
	void Gib(void);

	void Precache( void );
	void Spawn( void );
	void PrescheduleThink( void );
	void TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType );
	void HandleAnimEvent( animevent_t *pEvent );
	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	void			DoMuzzleFlash( void );

	bool AllArmorDestroyed( void );
	bool IsArmorPiece( int iArmorPiece );

	float MaxYawSpeed( void );

	Class_T Classify( void ) { return CLASS_COMBINE; }

	Activity NPC_TranslateActivity( Activity baseAct );

	virtual int	SelectSchedule( void );

	void BuildScheduleTestBits( void );

	DECLARE_DATADESC();

private:

	bool m_fOffBalance;
	float m_flNextClobberTime;

	int	GetReferencePointForBodyGroup( int bodyGroup );

	void DamageArmorPiece( int pieceID, float damage, const Vector &vecOrigin, const Vector &vecDir );
	void DestroyArmorPiece( int pieceID );
	void Shove( void );
	void FireRangeWeapon( void );


	bool AimGunAt( CBaseEntity *pEntity, float flInterval );


	int	m_nGibModel;

	float m_flGlowTime;
	float m_flLastRangeTime;

	float m_aimYaw;
	float m_aimPitch;

	int	m_YawControl;
	int	m_PitchControl;
	int	m_MuzzleAttachment;

	DEFINE_CUSTOM_AI;
};

#define	CGUARD_DEFAULT_ARMOR_HEALTH	50

#define	COMBINEGUARD_MELEE1_RANGE	98 //92 initially
#define	COMBINEGUARD_MELEE1_CONE	0.5f

#define	COMBINEGUARD_RANGE1_RANGE	1024
#define	COMBINEGUARD_RANGE1_CONE	0.0f

#define	CGUARD_GLOW_TIME			0.5f

BEGIN_DATADESC( CNPC_CombineGuard )

	DEFINE_FIELD( m_fOffBalance, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextClobberTime, FIELD_TIME ),
	DEFINE_FIELD( m_nGibModel, FIELD_INTEGER ),
	DEFINE_FIELD( m_flGlowTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flLastRangeTime, FIELD_TIME ),
	DEFINE_FIELD( m_aimYaw,	FIELD_FLOAT ),
	DEFINE_FIELD( m_aimPitch, FIELD_FLOAT ),
	DEFINE_FIELD( m_YawControl,	FIELD_INTEGER ),
	DEFINE_FIELD( m_PitchControl, FIELD_INTEGER ),
	DEFINE_FIELD( m_MuzzleAttachment, FIELD_INTEGER ),

END_DATADESC()

enum CombineGuardSchedules 
{	
	SCHED_CGUARD_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE,

};

enum CombineGuardTasks 
{	
	TASK_CGUARD_RANGE_ATTACK1 = LAST_SHARED_TASK,
	TASK_COMBINEGUARD_SET_BALANCE,
};

enum CombineGuardConditions
{	
	COND_COMBINEGUARD_CLOBBERED = LAST_SHARED_CONDITION,
};

int	ACT_CGUARD_IDLE_TO_ANGRY;
int ACT_COMBINEGUARD_CLOBBERED;
int ACT_COMBINEGUARD_TOPPLE;
int ACT_COMBINEGUARD_GETUP;
int ACT_COMBINEGUARD_HELPLESS;

#define	CGUARD_AE_SHOVE	11
#define	CGUARD_AE_FIRE 12
#define	CGUARD_AE_FIRE_START 13
#define	CGUARD_AE_GLOW 14
#define CGUARD_AE_LEFTFOOT 20
#define CGUARD_AE_RIGHTFOOT	21
#define CGUARD_AE_SHAKEIMPACT 22

CNPC_CombineGuard::CNPC_CombineGuard( void )
{
}

LINK_ENTITY_TO_CLASS( npc_combineguard, CNPC_CombineGuard );

void CNPC_CombineGuard::Precache( void )
{
	PrecacheModel( COMBINEGUARD_MODEL );
	PrecacheModel("models/gibs/Antlion_gib_Large_2.mdl"); //Antlion gibs for now

	PrecacheModel( "sprites/blueflare1.vmt" ); //For some reason this appears between his feet.

	PrecacheScriptSound( "NPC_CombineGuard.FootstepLeft" );
	PrecacheScriptSound( "NPC_CombineGuard.FootstepRight" );
	PrecacheScriptSound( "NPC_CombineGuard.Fire" );
	PrecacheScriptSound( "Cguard.Evil" );
	PrecacheScriptSound( "NPC_CombineGuard.Charging" );

	BaseClass::Precache();
}



void CNPC_CombineGuard::Spawn( void )
{
	Precache();

	SetModel( COMBINEGUARD_MODEL );

	SetHullType( HULL_WIDE_HUMAN );
	
	SetNavType( NAV_GROUND );
	m_NPCState = NPC_STATE_NONE;
	SetBloodColor( BLOOD_COLOR_YELLOW );

	m_iHealth = m_iMaxHealth = sk_combineguard_health.GetFloat();
	m_flFieldOfView = 0.1f;

	
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	m_flGlowTime = gpGlobals -> curtime;
	m_flLastRangeTime = gpGlobals -> curtime;
	m_aimYaw = 0;
	m_aimPitch = 0;

	m_flNextClobberTime	= gpGlobals -> curtime;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_SQUAD );


	NPCInit();
	
	
	SetBodygroup( 1, 0 );

	m_YawControl = LookupPoseParameter( "aim_yaw" );
	m_PitchControl = LookupPoseParameter( "aim_pitch" );
	m_MuzzleAttachment = LookupAttachment( "muzzle" );

	m_fOffBalance = false;

	BaseClass::Spawn();
}

void CNPC_CombineGuard::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	if( HasCondition( COND_COMBINEGUARD_CLOBBERED ) )
	{
		Msg( "CLOBBERED!\n" );
	}

	if ( GetEnemy() != NULL )
	{
		AimGunAt( GetEnemy(), 0.1f );
	}

	Vector vecDamagePoint;
	QAngle vecDamageAngles;


}

void CNPC_CombineGuard::Event_Killed( const CTakeDamageInfo &info )
{

	if ( m_nBody < GUARD_BODY_GUNGONE )
	{
		// drop the gun!
		Vector vecGunPos;
		QAngle angGunAngles;

		SetBodygroup( 1, GUARD_BODY_GUNGONE);

		GetAttachment( "0", vecGunPos, angGunAngles );
		
		angGunAngles.y += 180;

	}

    BaseClass::Event_Killed( info );

}

int CNPC_CombineGuard::SelectSchedule ( void )
{
	{

	switch (m_NPCState)
	{

	case NPC_STATE_ALERT:
	{
		if (HasCondition(COND_HEAR_DANGER) || HasCondition(COND_HEAR_COMBAT))
				{
						return SCHED_INVESTIGATE_SOUND;
				}
	}
		break;


	case NPC_STATE_COMBAT:
	{
							 // dead enemy
		if (HasCondition(COND_ENEMY_DEAD))
		{

		int min = 1;
		int max = 4;
		double scaled = (double)rand()/RAND_MAX;
		int r = (max - min +1)*scaled + min;
		if (r == 1) //1 in 4 chance to taunt enemies
		{
			EmitSound( "CGuard.Evil" );
		}

		if (r > 1)
		{
			EmitSound( "Common.Null" );
		}

		}
		return BaseClass::SelectSchedule(); // call base class, all code to handle dead enemies is centralized there.


			if (!HasCondition(COND_SEE_ENEMY))
				{
								 // we can't see the enemy
					if (!HasCondition(COND_ENEMY_OCCLUDED))
					 {
						return SCHED_COMBAT_FACE;
						}
					 else
						{
						return SCHED_CHASE_ENEMY;
						}
					}

					 if (HasCondition(COND_SEE_ENEMY))
					{
						if (HasCondition(COND_TOO_FAR_TO_ATTACK))
						 {
							 return SCHED_CHASE_ENEMY;
						 }
					 }
		}
		break;

	case NPC_STATE_IDLE:
		if (HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return SCHED_SMALL_FLINCH;
		}

		if (GetEnemy() == NULL) //&& GetFollowTarget() )
		{
			{

				return SCHED_TARGET_FACE;
			}
		}

		if (HasCondition(COND_HEAR_DANGER) || HasCondition(COND_HEAR_COMBAT))
		{
						return SCHED_INVESTIGATE_SOUND;
		}

		// try to say something about smells
		break;
	}

	return BaseClass::SelectSchedule();
	}
}

void CNPC_CombineGuard::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent -> event )
	{
	case CGUARD_AE_SHAKEIMPACT:
		Shove();
		UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );
		break;

	case CGUARD_AE_LEFTFOOT:
		{
			EmitSound( "NPC_CombineGuard.FootstepLeft" );
		}
		break;

	case CGUARD_AE_RIGHTFOOT:
		{
			EmitSound( "NPC_CombineGuard.FootstepRight" );
		}
		break;
	
	case CGUARD_AE_SHOVE:
		Shove();
		break;

	case CGUARD_AE_FIRE:
		{
			m_flLastRangeTime = gpGlobals->curtime + 6.0f;
			FireRangeWeapon();
			
			EmitSound( "NPC_CombineGuard.Fire" );

			EntityMessageBegin( this, true );
				WRITE_BYTE( CGUARD_MSG_SHOT );
			MessageEnd();
		}
		break;

	case CGUARD_AE_FIRE_START:
		{
			EmitSound( "NPC_CombineGuard.Charging" );

			EntityMessageBegin( this, true );
				WRITE_BYTE( CGUARD_MSG_SHOT_START );
			MessageEnd();
		}
		break;

	case CGUARD_AE_GLOW:
		m_flGlowTime = gpGlobals->curtime + CGUARD_GLOW_TIME;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		return;
	}
}

void CNPC_CombineGuard::Shove( void ) // Doesn't work for some reason // It sure works now! -Dan
{
	if ( GetEnemy() == NULL )
		return;

	CBaseEntity *pHurt = NULL;

	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );

	float flDist = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() ).Length();
	Vector2D v2LOS = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() ).AsVector2D();
	Vector2DNormalize( v2LOS );
	float flDot	= DotProduct2D ( v2LOS , forward.AsVector2D() );

	flDist -= GetEnemy() -> WorldAlignSize().x * 0.5f;

	if ( flDist < COMBINEGUARD_MELEE1_RANGE && flDot >= COMBINEGUARD_MELEE1_CONE )
	{
		Vector vStart = GetAbsOrigin();
		vStart.z += WorldAlignSize().z * 0.5;

		Vector vEnd = GetEnemy() -> GetAbsOrigin();
		vEnd.z += GetEnemy() -> WorldAlignSize().z * 0.5;

		pHurt = CheckTraceHullAttack( vStart, vEnd, Vector( -16, -16, 0 ), Vector( 16, 16, 24 ), 25, DMG_CLUB );
	}

	if ( pHurt )
	{
		pHurt -> ViewPunch( QAngle( -20, 0, 20 ) );

		UTIL_ScreenShake( pHurt -> GetAbsOrigin(), 100.0, 1.5, 1.0, 2, SHAKE_START );
		
		color32 white = { 255, 255, 255, 64 };
		UTIL_ScreenFade( pHurt, white, 0.5f, 0.1f, FFADE_IN );

		if ( pHurt->IsPlayer() )
		{
			Vector forward, up;
			AngleVectors( GetLocalAngles(), &forward, NULL, &up );
			pHurt->ApplyAbsVelocityImpulse( forward * 300 + up * 250 );
		}	
	}
}

void CNPC_CombineGuard::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	
		TaskComplete();

		break;

	case TASK_CGUARD_RANGE_ATTACK1:
		{
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTarget( flEnemyLKP );
		}
		SetActivity( ACT_RANGE_ATTACK1 );
		return;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

void CNPC_CombineGuard::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CGUARD_RANGE_ATTACK1:
		{
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTargetAndUpdate( flEnemyLKP );

			if ( IsActivityFinished() )
			{
				TaskComplete();
				return;
			}
		}
		return;
	}

	BaseClass::RunTask( pTask );
}


#define TRANSLATE_SCHEDULE( type, in, out ) { if ( type == in ) return out; }

Activity CNPC_CombineGuard::NPC_TranslateActivity( Activity baseAct )
{
	if( baseAct == ACT_RUN )
	{
		return ( Activity )ACT_WALK;
	}
	
	if ( baseAct == ACT_IDLE && m_NPCState != NPC_STATE_IDLE )
	{
		return ( Activity ) ACT_IDLE_ANGRY;
	}

	return baseAct;
}

int CNPC_CombineGuard::TranslateSchedule( int type ) 
{
	switch( type )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_CGUARD_RANGE_ATTACK1;
		break;
	}

	return BaseClass::TranslateSchedule( type );
}


int CNPC_CombineGuard::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newInfo = info;
	if( newInfo.GetDamageType() & DMG_CRUSH)
	{
		newInfo.ScaleDamage( 5.0 );
		DevMsg( "physics collision: 5X DAMAGE DONE TO CGUARD!\n" );
	}	

	int nDamageTaken = BaseClass::OnTakeDamage_Alive( newInfo );


	return nDamageTaken;
}


int CNPC_CombineGuard::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > COMBINEGUARD_MELEE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < COMBINEGUARD_MELEE1_CONE )
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_MELEE_ATTACK1;
}

int CNPC_CombineGuard::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > COMBINEGUARD_RANGE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < COMBINEGUARD_RANGE1_CONE )
		return COND_NOT_FACING_ATTACK;

	if ( m_flLastRangeTime > gpGlobals->curtime )
		return COND_TOO_FAR_TO_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}

void CNPC_CombineGuard::DoMuzzleFlash( void )
{
	BaseClass::DoMuzzleFlash();
	
	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment( "Muzzle" );
	data.m_nEntIndex = entindex();
	DispatchEffect( "StriderMuzzleFlash", data );
}


void CNPC_CombineGuard::FireRangeWeapon( void )
{
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->Classify() == CLASS_BULLSEYE ) )
	{
		GetEnemy()->TakeDamage( CTakeDamageInfo( this, this, 1.0f, DMG_GENERIC ) );
	}

	Vector vecSrc, vecAiming;
	Vector vecShootOrigin;

	GetVectors( &vecAiming, NULL, NULL );
	vecSrc = WorldSpaceCenter() + vecAiming * 64;
	
	Vector	impactPoint	= vecSrc + ( vecAiming * MAX_TRACE_LENGTH );

	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );
	QAngle angDir;
	
	QAngle	angShootDir;
	DoMuzzleFlash();
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );

	trace_t	tr;
	AI_TraceLine( vecSrc, impactPoint, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	// Just using the gunship tracers for a placeholder unless a better effect can be found.
	UTIL_Tracer( tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat( 0, 2000 ), true, "GunshipTracer" );
	UTIL_Tracer( tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat( 0, 3000 ), true, "GunshipTracer" );
	UTIL_Tracer( tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat( 0, 4000 ), true, "GunshipTracer" );

	CreateConcussiveBlast( tr.endpos, tr.plane.normal, this, 1.0 );
}

#define	DEBUG_AIMING 0

bool CNPC_CombineGuard::AimGunAt( CBaseEntity *pEntity, float flInterval )
{
	if ( !pEntity )
		return true;

	matrix3x4_t gunMatrix;
	GetAttachment( m_MuzzleAttachment, gunMatrix );

	Vector localEnemyPosition;
	VectorITransform( pEntity->GetAbsOrigin(), gunMatrix, localEnemyPosition );
	
	QAngle localEnemyAngles;
	VectorAngles( localEnemyPosition, localEnemyAngles );
	
	localEnemyAngles.x = UTIL_AngleDiff( localEnemyAngles.x, 0 );	
	localEnemyAngles.y = UTIL_AngleDiff( localEnemyAngles.y, 0 );

	float targetYaw = m_aimYaw + localEnemyAngles.y;
	float targetPitch = m_aimPitch + localEnemyAngles.x;
	
	QAngle unitAngles = localEnemyAngles;
	float angleDiff = sqrt( localEnemyAngles.y * localEnemyAngles.y + localEnemyAngles.x * localEnemyAngles.x );
	const float aimSpeed = 1;

	float yawSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.y);
	float pitchSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.x);

	yawSpeed = max(yawSpeed,15);
	pitchSpeed = max(pitchSpeed,15);

	m_aimYaw = UTIL_Approach( targetYaw, m_aimYaw, yawSpeed );
	m_aimPitch = UTIL_Approach( targetPitch, m_aimPitch, pitchSpeed );

	SetPoseParameter( m_YawControl, m_aimYaw );
	SetPoseParameter( m_PitchControl, m_aimPitch );

	m_aimPitch = GetPoseParameter( m_PitchControl );
	m_aimYaw = GetPoseParameter( m_YawControl );

	if ( angleDiff < 1 )
		return true;

	return false;
}



float CNPC_CombineGuard::MaxYawSpeed( void ) 
{ 	
	if ( GetActivity() == ACT_RANGE_ATTACK1 )
	{
		return 1.0f;
	}

	return 60.0f;
}

void CNPC_CombineGuard::BuildScheduleTestBits( void )
{
	SetCustomInterruptCondition( COND_COMBINEGUARD_CLOBBERED );
}

AI_BEGIN_CUSTOM_NPC( npc_combineguard, CNPC_CombineGuard )

	DECLARE_TASK( TASK_CGUARD_RANGE_ATTACK1 )
	DECLARE_TASK( TASK_COMBINEGUARD_SET_BALANCE )
	
	DECLARE_CONDITION( COND_COMBINEGUARD_CLOBBERED )

	DECLARE_ACTIVITY( ACT_CGUARD_IDLE_TO_ANGRY )
	DECLARE_ACTIVITY( ACT_COMBINEGUARD_CLOBBERED )
	DECLARE_ACTIVITY( ACT_COMBINEGUARD_TOPPLE )
	DECLARE_ACTIVITY( ACT_COMBINEGUARD_GETUP )
	DECLARE_ACTIVITY( ACT_COMBINEGUARD_HELPLESS )

	DEFINE_SCHEDULE
	(
		SCHED_CGUARD_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_ANNOUNCE_ATTACK		1"
		"		TASK_CGUARD_RANGE_ATTACK1	0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_NO_PRIMARY_AMMO"
	)

AI_END_CUSTOM_NPC()



