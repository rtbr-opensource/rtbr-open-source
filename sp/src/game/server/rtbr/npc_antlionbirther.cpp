//========= Copyright (c) RTBR Team, 2024 ============//
//
// Purpose: Antlion Birther - Large Antlion that births rollergrubs
//
//====================================================//

#include "cbase.h"
#include "npc_antlionbirther.h"
#include "hl2_shareddefs.h"
#include "npcevent.h"
#include "saverestore_utlvector.h"
#include "movevars_shared.h"
#include "particle_parse.h"
#include "grenade_spit.h"

#include "npc_antlionrollergrub.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


LINK_ENTITY_TO_CLASS( npc_antlionbirther, CNPC_AntlionBirther );

extern ConVar sk_npc_head;

#define ANTLIONBIRTHER_MODEL "models/antlionbirther/antlionbirther.mdl"

#define ANTLIONBIRTHER_SACK_BODYGROUP 1

#define ANTLIONBIRTHER_GRUB_FLING_TOLERANCE 10 * 12

ConVar sk_antlionbirther_health( "sk_antlionbirther_health", "0" );
ConVar sk_antlionbirther_sack_health( "sk_antlionbirther_sack_health", "0" );
ConVar sk_antlionbirther_sackdamage_multiplier( "sk_antlionbirther_sackdamage_multiplier", "0" );
ConVar sk_antlionbirther_buckshot_resistance_multiplier( "sk_antlionbirther_buckshot_resistance_multiplier", "0" );
ConVar sk_antlionbirther_melee_dmg( "sk_antlionbirther_melee_dmg", "0" );
ConVar g_debug_antlionbirther( "g_debug_antlionbirther", "0", FCVAR_CHEAT );
ConVar antlionbirther_max_z_fling_mult( "antlionbirther_max_z_fling_mult", "1.50" );
ConVar antlionbirther_min_z_fling_mult( "antlionbirther_min_z_fling_mult", "0.5" );
ConVar antlionbirther_max_adjust_dist( "antlionbirther_max_adjust_dist", "600" );

// Birther
static int AE_BIRTHER_GRUB_THROW;
static int AE_BIRTHER_MELEE_ATTACK;

// Birther
static int ACT_BIRTHER_BURST;


// Storing off an index to the rollergrub's classname here so we don't have
// to do string comparisons later
static string_t s_iszRollerGrubClassname;


#define ANTLIONBIRTHER_MELEE_RANGE 171
#define ANTLIONBIRTHER_MELEE_CONE 0.8f

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_AntlionBirther )

	DEFINE_FIELD( m_iNumRollerGrubsActive, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_iRollerGrubCapacity, FIELD_INTEGER, "RollerGrubCapacity" ),
	DEFINE_FIELD( m_flTimeLastRollergrubThrown, FIELD_TIME ),
	DEFINE_UTLVECTOR( m_hRollerGrubs, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bSackKilled, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bSackDamageEnabled, FIELD_BOOLEAN, "SackDamageEnabled" ),
	DEFINE_FIELD( m_flNextIdleSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextPainSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextAlertSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_iSackHealth, FIELD_FLOAT ),
	DEFINE_FIELD( m_bSpittingAcid, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EnableSackDamage", Input_EnableSackDamage ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableSackDamage", Input_DisableSackDamage ),

END_DATADESC()

CNPC_AntlionBirther::CNPC_AntlionBirther()
	: m_bSackDamageEnabled{ true }
{
	m_iRollerGrubCapacity = 3;
}

//---------------------------------------------------------
// Purpose: Precache needed resources
//---------------------------------------------------------
void CNPC_AntlionBirther::Precache( void )
{
	PrecacheModel( ANTLIONBIRTHER_MODEL );

	UTIL_PrecacheOther( "npc_headcrab_fast" );
	UTIL_PrecacheOther( "npc_antlionrollergrub" );

	// Precache sounds
	PrecacheScriptSound( "NPC_Antlion_Birther.Idle" );
	PrecacheScriptSound( "NPC_Antlion_Birther.Alert" );
	PrecacheScriptSound( "NPC_Antlion_Birther.Death" );
	PrecacheScriptSound( "NPC_Antlion_Birther.Pain" );
	PrecacheScriptSound( "NPC_Antlion_Birther.Footstep" );
	PrecacheScriptSound( "NPC_Antlion_Birther.SpitSpit" );
	PrecacheScriptSound( "NPC_Antlion_Birther.SpitInhale" );
	PrecacheScriptSound( "NPC_Antlion_Birther.MeleeAttack" );
	PrecacheScriptSound( "NPC_Antlion_Birther.SpitGrub" );

	PrecacheParticleSystem( "blood_impact_antlion_01" );

	PrecacheParticleSystem( "birther_sack_burst" );
	PrecacheParticleSystem( "GrubSquashBlood" );

	BaseClass::Precache();
}

//---------------------------------------------------------
// Purpose: Spawn entity into world
//---------------------------------------------------------
void CNPC_AntlionBirther::Spawn( void )
{
	Precache();

	// Call the base class first so we can override with our own properties
	BaseClass::Spawn();

	SetModel( ANTLIONBIRTHER_MODEL );
	SetHullType( HULL_LARGE );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_GREEN );

	m_iHealth		= sk_antlionbirther_health.GetInt();
	m_flFieldOfView = -0.5;
	m_NPCState		= NPC_STATE_NONE;

	m_flTimeLastRollergrubThrown = 0.0f;
	m_iSackHealth = sk_antlionbirther_sack_health.GetInt();
	m_bSackKilled = false;
	m_bSpittingAcid = false;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );
	//CapabilitiesAdd( bits_CAP_MOVE_JUMP );
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2);
	CapabilitiesAdd( bits_CAP_SQUAD );
	SetCollisionGroup( HL2COLLISION_GROUP_ANTLION );

	AddSpawnFlags( SF_NPC_LONG_RANGE );
	SetNavType( NAV_GROUND );
	SetMoveType( MOVETYPE_STEP );
	SetViewOffset( Vector(0, 0, 32) );

	// Expand our bounding box to account for our model's size.
	// This sets our hull slightly off of our BBox.
	UTIL_SetSize( this, Vector( GetHullMins().x * 1.5f, GetHullMins().y * 1.5f, GetHullMins().z ), Vector( GetHullMaxs().x * 1.5f, GetHullMaxs().y * 1.5f, GetHullMaxs().z ) );

	NPCInit();
}


//---------------------------------------------------------
// Purpose: Spawn entity into world
//---------------------------------------------------------
void CNPC_AntlionBirther::Activate( void )
{
	s_iszRollerGrubClassname = FindPooledString( "npc_antlionrollergrub" );

	m_iNumRollerGrubsActive = 0;
	m_hRollerGrubs.SetSize( m_iRollerGrubCapacity );

	// Setup sounds
	m_flNextIdleSoundTime = gpGlobals->curtime;
	m_flNextPainSoundTime = gpGlobals->curtime;
	m_flNextAlertSoundTime = gpGlobals->curtime;

	BaseClass::Activate();
}

//---------------------------------------------------------
// Purpose: Input handler for enabling sack damage.
//---------------------------------------------------------
void CNPC_AntlionBirther::Input_EnableSackDamage( inputdata_t &inputdata )
{
	if ( !m_bSackDamageEnabled )
		m_bSackDamageEnabled = true;
}

//---------------------------------------------------------
// Purpose: Input handler for disabling sack damage.
//---------------------------------------------------------
void CNPC_AntlionBirther::Input_DisableSackDamage( inputdata_t &inputdata )
{
	if ( m_bSackDamageEnabled )
		m_bSackDamageEnabled = false;
}

void CNPC_AntlionBirther::PrescheduleThink( void ) 
{
	// Check sounds
	if ( gpGlobals->curtime > m_flNextIdleSoundTime )
	{
		IdleSound();
	}

	if ( gpGlobals->curtime > m_flNextAlertSoundTime )
	{
		if ( HasCondition( COND_NEW_ENEMY ) || ( m_NPCState == NPC_STATE_ALERT && HasCondition( COND_HEAR_COMBAT ) ) )
		{
			AlertSound();
		}
	}

	BaseClass::PrescheduleThink();
}

//---------------------------------------------------------
// Purpose: A custom filter specifically for grabbing only roller-grubs.
//---------------------------------------------------------
class CTraceFilterRollerGrubs : public CTraceFilterOnlyNPCsAndPlayer
{
public:
	// It does have a base, but we'll never network anything below here...
	DECLARE_CLASS_NOBASE( CTraceFilterRollerGrubs );

	CTraceFilterRollerGrubs( IHandleEntity *passentity, int collisionGroup ) : CTraceFilterOnlyNPCsAndPlayer( passentity, collisionGroup ) { } 

	// We only want to hit rollergrubs.
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
			return false;

		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		CNPC_AntlionRollerGrub *pRollergrub = dynamic_cast< CNPC_AntlionRollerGrub *>( pEntity );

		if ( pRollergrub )
		{
			//NDebugOverlay::EntityBounds( pRollergrub, 255, 0, 0, 55, 10.0f );
			return true;
		}

		return false;
	}
};

//---------------------------------------------------------
// Purpose: Schedule Selection
//---------------------------------------------------------
int CNPC_AntlionBirther::SelectSchedule( void )
{
	switch ( m_NPCState )
	{
		case NPC_STATE_COMBAT:
		{
			// Melee if the enemy gets too close
			if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) && gpGlobals->curtime > m_flNextAttack )
				return SCHED_MELEE_ATTACK1;

			// If our sack has been popped, we can no longer throw grubs - rush the enemy for a final melee attack
			/*if (HasCondition(COND_ANTLIONBIRTHER_SACK_BURST))
				return SCHED_ANTLIONBIRTHER_FINAL_RUSH_ENEMY;*/

			// If we cannot see our enemy, we must establish LOS
			if ( !HasCondition( COND_SEE_ENEMY ) )
			{
				return SCHED_ESTABLISH_LINE_OF_FIRE;
			}

			// If we can see our enemy, and we are able to attack -- Launch a roller grub
			if ( HasCondition( COND_SEE_ENEMY ) && HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( m_iNumRollerGrubsActive < m_iRollerGrubCapacity && !HasCondition( COND_ANTLIONBIRTHER_SACK_BURST ) )
				{
					// If there is already a rollergrub in the way, then we NEED to move so we don't just smack into it.
					//trace_t tr;
					//CTraceFilterRollerGrubs rollergrubFilter( this, COLLISION_GROUP_NONE );
					//UTIL_TraceHull( GetAbsOrigin(), GetEnemy()->WorldSpaceCenter(), NAI_Hull::Mins( HULL_SMALL_CENTERED ), NAI_Hull::Maxs( HULL_SMALL_CENTERED ), MASK_ALL, &rollergrubFilter, &tr );
					//if ( tr.fraction <= 1.0f && tr.m_pEnt )
					//{
					//	//DevMsg( "ROLLER GRUB IN THE WAY!\n" );
					//	return SCHED_TAKE_COVER_FROM_ENEMY;
					//}

					m_bSpittingAcid = false;

					return SCHED_ANTLIONBIRTHER_SHOOT_GRUBS;
				}
				else
				{
					m_bSpittingAcid = true;
					return SCHED_ANTLIONBIRTHER_SPIT_ATTACK;
				}
			}

			// If we meet the same conditions as above, have not maxed out our rollergrub output, but cannot attack yet, Stand Our Ground!
			if ( HasCondition( COND_SEE_ENEMY ) && ( m_iNumRollerGrubsActive < m_iRollerGrubCapacity ) && ( gpGlobals->curtime < m_flNextAttack ) )
			{
				// Only select this schedule if we aren't already running this -- otherwise we will constantly select it over and over again, ruining the pathing.
				if ( !IsCurSchedule( SCHED_ANTLIONBIRTHER_MOVE_RANDOM_PATH ) )
				{
					return SCHED_ANTLIONBIRTHER_MOVE_RANDOM_PATH;
				}
				// Just stand in place for now, better than aimlessly wandering around
				//return SCHED_COMBAT_FACE;
			}
			
			// If all else fails, take cover!
			return SCHED_TAKE_COVER_FROM_ENEMY;
		}
	}
	return BaseClass::SelectSchedule();
}


//---------------------------------------------------------
// Purpose: Melee Attack Conditions
//---------------------------------------------------------
int CNPC_AntlionBirther::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > ANTLIONBIRTHER_MELEE_RANGE || fabsf( GetEnemy()->GetAbsOrigin().z - GetAbsOrigin().z > 64 ) ) // Also check for vertical height differences.
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}


//---------------------------------------------------------
// Purpose: The Birther should always be facing their enemy
//			otherwise they look like they're retreating when
//			moving ever so slightly.
//---------------------------------------------------------
bool CNPC_AntlionBirther::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	/*
	// If we are in a position to attack our enemy, we MUST be facing them.
	if ( GetEnemy() && HasCondition( COND_SEE_ENEMY ) && HasCondition( COND_CAN_RANGE_ATTACK1 ) )
	{
		AddFacingTarget( GetEnemy(), GetEnemy()->WorldSpaceCenter(), 1.0f, 0.2f );
	}*/

	// If we are around to fight our enemy, we should be facing them, rather than the path.
	if ( ( IsCurSchedule( SCHED_ANTLIONBIRTHER_MOVE_RANDOM_PATH ) || IsCurSchedule( SCHED_MOVE_AWAY_FROM_ENEMY ) ) && GetEnemy() )
	{
		// Only face our enemy if they are close enough, otherwise, face the path
		Vector enemyPos = GetEnemyLKP();
		if ( UTIL_DistApprox( enemyPos, GetAbsOrigin()) < 4096 )
		{
			AddFacingTarget( GetEnemy(), GetEnemy()->WorldSpaceCenter(), 1.0f, 1.0f);
			return BaseClass::OverrideMoveFacing( move, flInterval );
		}
	}

	return BaseClass::OverrideMoveFacing( move, flInterval );
}


//---------------------------------------------------------
// Purpose: Yaw Turning Speed
//---------------------------------------------------------
float CNPC_AntlionBirther::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	// If we are about to attack, we cannot be turning (or else the model's feet will slide).
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
		return 0.0f;
		break;

	case ACT_RANGE_ATTACK1:
		return 10.0f;
		break;

	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
	case ACT_IDLE:
	case ACT_RUN:
	default:
		return 40.0f;
		break;
	}

	//return BaseClass::MaxYawSpeed();
}

void CNPC_AntlionBirther::GatherConditions()
{
	BaseClass::GatherConditions();

	if (m_bSackKilled)
		SetCondition( COND_ANTLIONBIRTHER_SACK_BURST );
}

void CNPC_AntlionBirther::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	// don't let the enemy being close stop us from moving; we can't attack anyway
	if (IsCurSchedule( SCHED_ANTLIONBIRTHER_MOVE_RANDOM_PATH ))
		ClearCustomInterruptCondition( COND_CAN_MELEE_ATTACK1 );
}

//---------------------------------------------------------
// Purpose: We take excessive damage when damaged in our sack
//---------------------------------------------------------
int CNPC_AntlionBirther::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo infoCopy = info;

	if ( LastHitGroup() == HITGROUP_HEAD )
	{
		// hack: do *not* take headshot damage, as that makes the birther squishy
		infoCopy.SetDamage( infoCopy.GetDamage() * sk_antlionbirther_sackdamage_multiplier.GetFloat() / sk_npc_head.GetFloat() );

		// Take the original amount of damage ONLY in the sack, this makes it easier to pop the sack without just killing us.
		if (m_bSackDamageEnabled)
			m_iSackHealth -= info.GetDamage();

		if (m_iSackHealth <= 0 && !m_bSackKilled)
		{
			m_bSackKilled = true;
			//DevMsg( "Sack burst!\n" );

			SetActivity( (Activity)ACT_BIRTHER_BURST );

			// Switch on our 'burst sack' bodygroup.
			CBaseAnimating *pModel = GetBaseAnimating();
			if (pModel)
			{
				pModel->SetBodygroup( ANTLIONBIRTHER_SACK_BODYGROUP, m_bSackKilled );
			}

			DispatchParticleEffect( "birther_sack_burst", PATTACH_POINT_FOLLOW, this, "sackbleed" );
		}
	}
	else
	{
		if ( infoCopy.GetDamageType() == DMG_BUCKSHOT )
		{
			// We have a resistance to buckshot
			infoCopy.SetDamage( infoCopy.GetDamage() * sk_antlionbirther_buckshot_resistance_multiplier.GetFloat() );
		}
	}

	if ( gpGlobals->curtime > m_flNextPainSoundTime )
	{
		PainSound();
	}

	return BaseClass::OnTakeDamage_Alive( infoCopy );
}


//---------------------------------------------------------
// Purpose: Death Event Handing
// 
// We need to override this to handle any grubs still hanging around
//---------------------------------------------------------
void CNPC_AntlionBirther::Event_Killed( const CTakeDamageInfo &info )
{
	CNPC_AntlionRollerGrub *pRollerGrub;
	// If we have any active rollergrubs still active,
	// we need to remove their references to us now.
	if ( !m_hRollerGrubs.IsEmpty() )
	{
		for ( int i = 0; i < m_hRollerGrubs.Count(); i++ )
		{
			pRollerGrub = m_hRollerGrubs[i];
			if ( pRollerGrub )
				pRollerGrub->Event_BirtherKilled();
		}
	}

	DeathSound();

	StopParticleEffects(this);

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: For innate range attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_AntlionBirther::RangeAttack1Conditions( float flDot, float flDist )
{
	if (m_flNextAttack > gpGlobals->curtime)
	{
		return COND_NONE;
	}

	if (flDist < 64)
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
}

//---------------------------------------------------------
// Purpose: AnimEvent Handling
//---------------------------------------------------------
void CNPC_AntlionBirther::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_BIRTHER_GRUB_THROW )
	{
		if ( !m_bSpittingAcid )
		{
			// Attempt to fling a rollergrub.
			FlingRollerGrub();
		}
		else
			SpitAttack();

		return;
	}
	else if ( pEvent->event == AE_BIRTHER_MELEE_ATTACK )
	{
		CBaseEntity *pHurt = MeleeAttack();
		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer *>( pHurt );
		if ( pHurt != NULL && pPlayer != NULL )
			return;
		return;
	}

	return BaseClass::HandleAnimEvent( pEvent );
}

//---------------------------------------------------------
// Purpose: Performs a melee attack (mostly borrowed from zombie code)
//---------------------------------------------------------
CBaseEntity *CNPC_AntlionBirther::MeleeAttack( void )
{
	if ( GetEnemy() )
	{
		trace_t tr;
		AI_TraceHull( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), -Vector(32, 32, 32), Vector(32, 32, 32), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if ( !tr.DidHitWorld() && tr.fraction < 1.0f )
			return NULL;

		//
		// Trace out a cubic section of our bbox and see what we hit.
		// We use the bbox instead of our hull because we use an edited bbox that doesn't align completely with our hull.
		//
		//Vector vecMins = WorldAlignMins() * Vector( 1.2f, 1.2f, 1.0f );
		//Vector vecMaxs = WorldAlignMaxs() * Vector( 1.2f, 1.2f, 1.0f );
		Vector vecMins = WorldAlignMins();
		Vector vecMaxs = WorldAlignMaxs();

		if ( g_debug_antlionbirther.GetBool() )
		{
			NDebugOverlay::Box( GetAbsOrigin(), vecMins, vecMaxs, 255, 0, 0, 96, 2.0f );
		}

		CBaseEntity *pHurt = NULL;

		CTakeDamageInfo dmgInfo( this, this, sk_antlionbirther_melee_dmg.GetInt(), DMG_SLASH);

		// Fire off the sound.
		EmitSound( "NPC_Antlion_Birther.MeleeAttack" );

		if ( GetEnemy() )
		{
			// If the target's still inside the shove cone, ensure we hit him	
			Vector vecForward, vecEnd;
			AngleVectors( GetAbsAngles(), &vecForward );
			float flDistSqr = (GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter()).LengthSqr();
			Vector2D v2LOS = (GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter()).AsVector2D();
			Vector2DNormalize( v2LOS );
			float flDot = DotProduct2D( v2LOS, vecForward.AsVector2D() );
			if (flDistSqr < (ANTLIONBIRTHER_MELEE_RANGE * ANTLIONBIRTHER_MELEE_RANGE) && flDot >= ANTLIONBIRTHER_MELEE_CONE)
			{
				vecEnd = GetEnemy()->WorldSpaceCenter();
			}
			else
			{
				vecEnd = WorldSpaceCenter() + (BodyDirection3D() * ANTLIONBIRTHER_MELEE_RANGE);
			}

			CTraceFilterMelee meleeTraceFilter( this, COLLISION_GROUP_NONE, &dmgInfo, sk_antlionbirther_melee_dmg.GetInt(), true );
			Ray_t ray;
			ray.Init( WorldSpaceCenter(), vecEnd, Vector(-16, -16, -16), Vector(16, 16, 16) );
			enginetrace->TraceRay( ray, MASK_SHOT_HULL, &meleeTraceFilter, &tr );
			pHurt = tr.m_pEnt;
		}

		if ( pHurt != NULL )
		{
			// If the player, throw him around
			if (pHurt->IsPlayer())
			{
				Vector forward, up;
				AngleVectors( GetLocalAngles(), &forward, NULL, &up );
				pHurt->ApplyAbsVelocityImpulse( forward * 200 + up * 125 );
			}

			return pHurt;
		}
		else
			return NULL;
	}
	else
		return NULL;
}

//---------------------------------------------------------
// Purpose: Flings Roller-Grub egg at enemy
//---------------------------------------------------------
void CNPC_AntlionBirther::FlingRollerGrub( void )
{
	if ( m_iNumRollerGrubsActive < m_iRollerGrubCapacity && m_flNextAttack < gpGlobals->curtime && m_flTimeLastRollergrubThrown < gpGlobals->curtime + 3.0f )
	{
		Vector vecMouthPos, vecTarget;
		GetAttachment( "mouth", vecMouthPos );

		vecTarget = GetEnemy()->BodyTarget( vecMouthPos, true );

		// Make sure there isn't an existing grub in our way before we attempt the fling.
		//trace_t tr;
		//CTraceFilterRollerGrubs rollergrubTraceFilter( this, COLLISION_GROUP_NPC );
		//UTIL_TraceHull( vecMouthPos, vecTarget, NAI_Hull::Mins( HULL_SMALL_CENTERED ), NAI_Hull::Maxs( HULL_SMALL_CENTERED ), MASK_ALL, &rollergrubTraceFilter, &tr );

		//if ( tr.fraction < 1.0f && tr.m_pEnt )
		//{
		//	// There is an existing rollergrub in our way. Boot out and let the caller decide what happens next.
		//	return;
		//}

		if ( GetEnemy() )
		{
			CNPC_AntlionRollerGrub *pRollerGrub = (CNPC_AntlionRollerGrub *)CreateEntityByName( s_iszRollerGrubClassname.ToCStr() );

			//Vector throwVector;
			//GetThrowVector( vecMouthPos, vecTarget, 800, &throwVector );

			Vector rollerSpawnPos = vecMouthPos + Vector(0, 0, -8);
			Vector throwVector = pRollerGrub->ConstructFlingVector(GetEnemy(), rollerSpawnPos);

			pRollerGrub->SetAbsOrigin( rollerSpawnPos );
			pRollerGrub->SetAbsAngles( GetAbsAngles() );
			pRollerGrub->SetOwnerEntity( this );
			pRollerGrub->SetAbsVelocity( throwVector );

			DispatchSpawn( pRollerGrub );
			pRollerGrub->FlingFromBirther( this );

			// Fire off a sound.
			EmitSound( "NPC_Antlion_Birther.SpitGrub" );
			Vector birthFXSpawnLocation = rollerSpawnPos + Vector(0, 0, -32);
			Vector birthFXSpawnDirection = throwVector.Normalized();

			// blood spray when birthing
			DispatchParticleEffect("blood_impact_antlion_01", birthFXSpawnLocation, GetAbsAngles(), this);
			UTIL_BloodSpray(birthFXSpawnLocation, birthFXSpawnDirection, BLOOD_COLOR_YELLOW, 4, FX_BLOODSPRAY_ALL);
			//UTIL_BloodImpact(birthFXSpawnLocation, birthFXSpawnDirection, BLOOD_COLOR_ANTLION, 1);

			// leave decals on the ground after birthing
			trace_t ptr;
			Vector bloodDir = Vector(0, 0, -128);
			UTIL_TraceLine(birthFXSpawnLocation, birthFXSpawnLocation + bloodDir, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &ptr);
			MakeDamageBloodDecal(8, 50, &ptr, bloodDir);

			// Save off a reference to our grub
			m_hRollerGrubs[m_iNumRollerGrubsActive] = pRollerGrub;

			m_iNumRollerGrubsActive++;

			SetNextAttack( gpGlobals->curtime + RandomFloat( 2.0f, 5.0f ) );
			m_flTimeLastRollergrubThrown = gpGlobals->curtime;
			return;
		}
	}
	else
	{
		//DevMsg("Too Many Rollergrubs Active, no more spawning!\n");
		return;
	}
}


//---------------------------------------------------------
// Purpose: Keep track of killed Roller Grubs
//---------------------------------------------------------
void CNPC_AntlionBirther::Event_RollerGrubKilled( CNPC_AntlionRollerGrub *pRollerGrub )
{
	// Remove any reference to this rollergrub.
	CNPC_AntlionRollerGrub* theGrub = dynamic_cast<CNPC_AntlionRollerGrub*>( pRollerGrub );
	m_hRollerGrubs.FindAndRemove( theGrub );
	m_iNumRollerGrubsActive--;
}

//---------------------------------------------------------
// Purpose: Spit attack
//---------------------------------------------------------
void CNPC_AntlionBirther::SpitAttack( void )
{
	if ( GetEnemy() )
	{
		Vector vecSpitPos;
		GetAttachment( "mouth", vecSpitPos );

		Vector vecTarget;

		// If our enemy is looking at us and far enough away, lead him
		if ( HasCondition( COND_ENEMY_FACING_ME ) && UTIL_DistApprox( GetAbsOrigin(), GetEnemy()->GetAbsOrigin() ) > ( 40*12 ) )
		{
			UTIL_PredictedPosition( GetEnemy(), 0.5f, &vecTarget );
			vecTarget.z = GetEnemy()->GetAbsOrigin().z;
		}
		else
		{
			// Otherwise he can't see us and he won't be able to dodge
			vecTarget = GetEnemy()->BodyTarget( vecTarget, true );
		}

		//vecTarget.z += random->RandomFloat( 0.0f, 32.0f );

		// Try and spit at our target
		Vector vecToss;
		GetThrowVector( vecSpitPos, vecTarget, 800, &vecToss, true );

		Vector vecToTarget = ( vecTarget - vecSpitPos );
		float flVelocity = VectorNormalize( vecToss );
		float flCosTheta = DotProduct( vecToTarget, vecToss );
		float flTime = ( vecSpitPos - vecTarget ).Length2D() / ( flVelocity * flCosTheta );

		// Emit a sound where this is going to hit so that targets get a chance to act correctly.
		CSoundEnt::InsertSound( SOUND_DANGER, vecTarget, ( 15 * 12 ), flTime, this );

		// Play the actual sound.
		EmitSound( "NPC_Antlion_Birther.SpitSpit" );

		// Don't fire again until this volley would have hit the ground (with some lag behind it)
		SetNextAttack( gpGlobals->curtime + flTime + random->RandomFloat( 0.5f, 2.0f ) );

		CGrenadeSpit* pGrenade = (CGrenadeSpit*)CreateEntityByName("grenade_spit");
		pGrenade->SetAbsOrigin(vecSpitPos);
		pGrenade->SetAbsAngles(vec3_angle);
		pGrenade->SetBullsquidSpit(true);
		DispatchSpawn(pGrenade);
		pGrenade->SetThrower(this);
		pGrenade->SetOwnerEntity(this);

		pGrenade->SetSpitSize(SPIT_LARGE);
		pGrenade->SetAbsVelocity(vecToss * flVelocity);

		// Tumble through the air
		pGrenade->SetLocalAngularVelocity(
			QAngle(random->RandomFloat(-250, -500),
				random->RandomFloat(-250, -500),
				random->RandomFloat(-250, -500)));
	}
}

//---------------------------------------------------------
// Purpose: Idle Sound
//---------------------------------------------------------
void CNPC_AntlionBirther::IdleSound( void )
{
	//DevMsg( "IDLE SOUND!\n" );
	EmitSound( "NPC_Antlion_Birther.Idle" );
	m_flNextIdleSoundTime = gpGlobals->curtime + random->RandomFloat( 5.0f, 15.0f );
}

//---------------------------------------------------------
// Purpose: Alert Sound
//---------------------------------------------------------
void CNPC_AntlionBirther::AlertSound( void )
{
	//DevMsg( "ALERT SOUND!\n" );
	EmitSound( "NPC_Antlion_Birther.Alert" );
	m_flNextAlertSoundTime = gpGlobals->curtime + random->RandomFloat( 5.0f, 15.0f );
}

//---------------------------------------------------------
// Purpose: Death Sound
//---------------------------------------------------------
void CNPC_AntlionBirther::DeathSound( void )
{
	//DevMsg( "DEATH SOUND!\n" );
	EmitSound( "NPC_Antlion_Birther.Death" );
}

//---------------------------------------------------------
// Purpose: Pain Sound
//---------------------------------------------------------
void CNPC_AntlionBirther::PainSound( void )
{
	//DevMsg( "PAIN SOUND!\n" );
	EmitSound( "NPC_Antlion_Birther.Pain" );
	m_flNextPainSoundTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 1.0f );
}

//---------------------------------------------------------
// Purpose: Calculates a throw vector for the rollergrub fling
// 
// Code copied from Antlion worker spitball fling
//---------------------------------------------------------
void CNPC_AntlionBirther::GetThrowVector( const Vector& vecStartPos, const Vector& vecTarget, float flSpeed, Vector* vecOut, bool bSpit /* = false */)
{
	flSpeed = MAX( 1.0f, flSpeed );

	float flGravity = GetCurrentGravity();

	Vector vecFlingVelocity = ( vecTarget - vecStartPos );

	// throw at a constant time
	float flingTime = vecFlingVelocity.Length() / flSpeed;
	vecFlingVelocity = vecFlingVelocity * ( 1.0 / flingTime );

	// adjust upward toss to compensate for gravity loss
	float zMult;
	if (bSpit)
	{
		// antlion-style spit
		zMult = 0.5;
	}
	else
	{
		// birther fling - adjust npc z-fling velocity based on closeness to the birther
		float zAdjustFactor = antlionbirther_max_z_fling_mult.GetFloat() - antlionbirther_min_z_fling_mult.GetFloat();
		float maxDistSqr = antlionbirther_max_adjust_dist.GetFloat() * antlionbirther_max_adjust_dist.GetFloat();
		zMult = antlionbirther_min_z_fling_mult.GetFloat() + (zAdjustFactor * MAX( 0, (1 - (vecStartPos - vecTarget).Length2DSqr() / (maxDistSqr)) ));
	}

	vecFlingVelocity.z += flGravity * flingTime * zMult;
	Vector vecApex = vecStartPos + ( vecTarget - vecStartPos ) * 0.5;
	vecApex.z += 0.5 * flGravity * ( flingTime * 0.5 ) * ( flingTime * 0.5 );

	trace_t tr;
	UTIL_TraceLine( vecStartPos, vecApex, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 )
	{
		// fail!
		if ( g_debug_antlionbirther.GetBool() )
		{
			NDebugOverlay::Line( vecStartPos, vecApex, 255, 0, 0, true, 5.0 );
		}

		*vecOut = vec3_origin;
	}

	if ( g_debug_antlionbirther.GetBool() )
	{
		NDebugOverlay::Line( vecStartPos, vecApex, 0, 255, 0, true, 5.0 );
	}

	UTIL_TraceLine( vecApex, vecTarget, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 )
	{
		bool bFail = true;

		// Didn't make it all the way there, but check if we're within our tolerance range
		{
			float flNearness = ( tr.endpos - vecTarget ).LengthSqr();
			if ( flNearness < Square( ANTLIONBIRTHER_GRUB_FLING_TOLERANCE ) )
			{
				if ( g_debug_antlionbirther.GetBool() )
				{
					NDebugOverlay::Sphere( tr.endpos, vec3_angle, ANTLIONBIRTHER_GRUB_FLING_TOLERANCE, 0, 255, 0, 0, true, 5.0 );
				}
				bFail = false;
			}
		}

		if ( bFail )
		{
			if ( g_debug_antlionbirther.GetBool() )
			{
				NDebugOverlay::Line( vecApex, vecTarget, 255, 0, 0, true, 5.0 );
				NDebugOverlay::Sphere( tr.endpos, vec3_angle, ANTLIONBIRTHER_GRUB_FLING_TOLERANCE, 255, 0, 0, 0, true, 5.0 );
			}
			*vecOut = vec3_origin;
		}
	}

	if ( g_debug_antlionbirther.GetBool() )
	{
		NDebugOverlay::Line( vecApex, vecTarget, 0, 255, 0, true, 5.0 );
	}

	*vecOut = vecFlingVelocity;
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_antlionbirther, CNPC_AntlionBirther )

	DECLARE_CONDITION( COND_ANTLIONBIRTHER_HAS_ACTIVE_GRUBS );
	DECLARE_CONDITION( COND_ANTLIONBIRTHER_SACK_BURST );

	DECLARE_TASK( TASK_ANTLIONBIRTHER_SHOOT_GRUBS );

	DECLARE_ANIMEVENT( AE_BIRTHER_GRUB_THROW );
	DECLARE_ANIMEVENT( AE_BIRTHER_MELEE_ATTACK );

	DECLARE_ACTIVITY( ACT_BIRTHER_BURST );

	//Schedules

	//==================================================
	// Fling Rollergrubs
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONBIRTHER_SHOOT_GRUBS,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_FACE_ENEMY					0"
		"		TASK_SET_TOLERANCE_DISTANCE		512"
		"		TASK_RANGE_ATTACK1				0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ANTLIONBIRTHER_SACK_BURST"
	)

		DEFINE_SCHEDULE
		(
			SCHED_ANTLIONBIRTHER_SPIT_ATTACK,

			"	Tasks"
			"		TASK_STOP_MOVING				0"
			"		TASK_FACE_ENEMY					0"
			"		TASK_SET_TOLERANCE_DISTANCE		512"
			"		TASK_RANGE_ATTACK1				0"
			""
			"	Interrupts"
			"		COND_TASK_FAILED"
		)

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONBIRTHER_MOVE_RANDOM_PATH,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE							SCHEDULE:SCHED_COMBAT_FACE"
		"		TASK_SET_TOLERANCE_DISTANCE						8"
		"		TASK_SET_ROUTE_SEARCH_TIME						1"
		"		TASK_GET_PATH_TO_RANDOM_NODE					2096"
		"		TASK_RUN_PATH									0"
		"		TASK_WAIT_FOR_MOVEMENT							0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_ANTLIONBIRTHER_SACK_BURST"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONBIRTHER_FINAL_RUSH_ENEMY,

		"	Tasks"
		"	TASK_GET_PATH_TO_ENEMY		0"
		"	TASK_RUN_PATH				0"
	);

AI_END_CUSTOM_NPC()

