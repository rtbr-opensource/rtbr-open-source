//========= Copyright (c) RTBR Team, 2024 ============//
//
// Purpose: Antlion King
//
//====================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "npcevent.h"
#include "activitylist.h"
#include "te_effect_dispatch.h"
#include "triggers.h"
#include "saverestore_utlvector.h"
#include "movevars_shared.h"
#include "basegrenade_shared.h"
#include "IEffects.h"
#include "mapbase/GlobalStrings.h"
#include "props.h"
#include "hl2_shareddefs.h"
#include "entityoutput.h"
#include "explode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Forward Declarations
class CAntKingSpitBomb;
class CAntKingPukeZone;
class CNPC_AntlionKing;

static const char *kAntlionKing_Model = "models/antlionking/antlionking.mdl";

static const char *kAntlionKing_SpitballModel_Small	= "models/props_hive/spit/antking_spit_small.mdl";
static const char *kAntlionKing_SpitballModel_Medium	= "models/props_hive/spit/antking_spit_medium.mdl";
static const char *kAntlionKing_SpitballModel_Large	= "models/props_hive/spit/antking_spit_large_barrel.mdl";
static const char *kPropPhysics_Classname = "prop_physics";

static const int kAntKing_PukeZone_Length = 450;
static const int kAntKing_PukeZone_Width = 450;
static const int kAntKing_PukeZone_Height = 75;

ConVar sk_antlionking_spitbomb_damage( "sk_antlionking_spitbomb_damage", "0" );
ConVar sk_antlionking_spitbomb_radius( "sk_antlionking_spitbomb_radius", "0" );

ConVar sk_antlionking_health( "sk_antlionking_health", "0" );
ConVar sk_antlionking_weakpoint_damage_threshold( "sk_antlionking_weakpoint_damage_threshold", "100" );
ConVar sk_antlionking_pukezone_lifetime( "sk_antlionking_pukezone_lifetime", "0" );
ConVar g_debug_antlionking( "g_debug_antlionking", "0", FCVAR_CHEAT );

ConVar sk_antlionking_shockwave_advance_rate( "sk_antlionking_shockwave_advance_rate", "10" );
ConVar sk_antlionking_shockwave_damage( "sk_antlionking_shockwave_damage", "25" );
ConVar sk_antlionking_melee_attack_dmg( "sk_antlionking_melee_attack_dmg", "15" );

const float kKingAttackInterval_0 = 1.0f;
const float kKingAttackInterval_1 = 0.75f;
const float kKingAttackInterval_2 = 0.5f;

const float kKingBodyTurnRate = 300.0f;

// Animation Events
static int AE_KING_SPIT_ATTACK;
static int AE_KING_PUKE_ATTACK;
static int AE_KING_SHOCKWAVE_ATTACK;
static int AE_KING_SWIPE_ATTACK;
static int AE_KING_STEP_FRONTRIGHT;
static int AE_KING_STEP_BACKLEFT;
static int AE_KING_STEP_FRONTLEFT;
static int AE_KING_STEP_BACKRIGHT;

// Activities
static Activity ACT_SPIT;
static Activity ACT_PUKE;
static Activity ACT_PAIN;
static Activity ACT_STOMP;
static Activity ACT_SWIPE;
static Activity ACT_WEAKPOINT_START;
static Activity ACT_WEAKPOINT_LOOP;
static Activity ACT_WEAKPOINT_END;

// Storing off indices to these strings so we can access them later w/o string comparisons.
static string_t s_SpitballModelSmall;
static string_t s_SpitballModelMedium;
static string_t s_SpitballModelLarge;
static string_t s_PropPhysicsClassname;

// The bone names for all of our bone followers.
// We're statically-creating a list here because not all
// of the King's bones appear to be solid (i.e. att.blood1)
// so we can't simply iterate through the bone list.
static const char *pKingFollowerBoneNames[] =
{
	"pelvis",
	"thorax",
	"abdomen1",
	"abdomen2",

	// Back Left Leg
	"legback1.l",
	"legback2.l",
	"legback3.l",
	"legback4.l",
	"legback5.l",
	"legback6.l",

	"pedipalp1.l",
	"pedipalp2.l",
	"pedipalp3.l",
	"pedipalp4.l",
	"pedipalp5.l",
	"pedipalp6.l",
	"pedipalp7.l",

	// Front Right Leg
	"legfront1.r",
	"legfront2.r",
	"legfront3.r",
	"legfront4.r",
	"legfront5.r",
	"legfront6.r",
	"legfront7.r",

	// Back Right Leg
	"legback1.r",
	"legback2.r",
	"legback3.r",
	"legback4.r",
	"legback5.r",
	"legback6.r",

	"pedipalp1.r",
	"pedipalp2.r",
	"pedipalp3.r",
	"pedipalp4.r",
	"pedipalp5.r",
	"pedipalp6.r",
	"pedipalp7.r",

	// Front Left Leg
	"legfront1.l",
	"legfront2.l",
	"legfront3.l",
	"legfront4.l",
	"legfront5.l",
	"legfront6.l",
	"legfront7.l",

	// Chest
	"chest",

	"neck1",
	"neck2",
	"neck3",
	"neck4",
	"neck5",

	"head",

	"fangtop1.l",
	"fangtop2.l",
	"fangbottom1.l",
	"fangbottom2.l",

	"tonguetop1.l",
	"tonguetop2.l",
	"tonguetop3.l",
	"tonguetop4.l",

	"tonguebottom1.l",
	"tonguebottom2.l",
	"tonguebottom3.l",
	"tonguebottom4.l",

	"fangtop1.r",
	"fangtop2.r",
	"fangbottom1.r",
	"fangbottom2.r",

	"tonguetop1.r",
	"tonguetop2.r",
	"tonguetop3.r",
	"tonguetop4.r",

	"tonguebottom1.r",
	"tonguebottom2.r",
	"tonguebottom3.r",
	"tonguebottom4.r",

	"throatscrotetop",
	"throatscrotebottom",
};

//==================================================
// CAntKingSpitBomb - An hazardous spit bomb
//==================================================
class CAntKingSpitBomb : public CBreakableProp
{
	DECLARE_CLASS( CAntKingSpitBomb, CBreakableProp );
	DECLARE_DATADESC();

public:
	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual bool	OverridePropdata( void ) { return true; }
	void			CreateParticles( void );

	void			ExplodeTouch( CBaseEntity *pOther );

public:
	int				m_iSize{ 0 };
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CAntKingSpitBomb )

	DEFINE_ENTITYFUNC( ExplodeTouch ),

	DEFINE_FIELD( m_iSize, FIELD_INTEGER )

END_DATADESC()

//---------------------------------------------------------
// Purpose: Precache needed resources
//---------------------------------------------------------
void CAntKingSpitBomb::Precache( void )
{
	s_SpitballModelSmall = AllocPooledString( kAntlionKing_SpitballModel_Small );
	s_SpitballModelMedium = AllocPooledString( kAntlionKing_SpitballModel_Medium );
	s_SpitballModelLarge = AllocPooledString( kAntlionKing_SpitballModel_Large );
	s_PropPhysicsClassname = AllocPooledString( kPropPhysics_Classname );

	PropBreakablePrecacheAll( s_SpitballModelSmall );
	PropBreakablePrecacheAll( s_SpitballModelMedium );
	PropBreakablePrecacheAll( s_SpitballModelLarge );

	// Spit particles
	PrecacheParticleSystem( "antking_spit_small_trail" );
	PrecacheParticleSystem( "antking_spit_medium_trail" );
	PrecacheParticleSystem( "antking_spit_large_trail" );
	PrecacheParticleSystem( "antking_spit_small_explosion" );
	PrecacheParticleSystem( "antking_spit_medium_explosion" );
	PrecacheParticleSystem( "antking_spit_large_explosion" );

	BaseClass::Precache();
}

//---------------------------------------------------------
// Purpose: Spawn entity into world
//---------------------------------------------------------
void CAntKingSpitBomb::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetSolidFlags( FSOLID_NOT_STANDABLE );

	SetTouch( &CAntKingSpitBomb::ExplodeTouch );

	SetCollisionGroup( HL2COLLISION_GROUP_SPIT );
	m_impactEnergyScale = 10.0f;
}

//---------------------------------------------------------
// Purpose: Create the appropriate spit particles.
//---------------------------------------------------------
void CAntKingSpitBomb::CreateParticles( void )
{
	Assert( m_iSize >= 0 && m_iSize <= 2 );

	switch ( m_iSize )
	{
	case 0:
		DispatchParticleEffect( "antking_spit_small_trail", PATTACH_ABSORIGIN_FOLLOW, this );
		break;
	case 1:
	default:
		DispatchParticleEffect( "antking_spit_medium_trail", PATTACH_ABSORIGIN_FOLLOW, this );
		break;
	case 2:
		DispatchParticleEffect( "antking_spit_large_trail", PATTACH_ABSORIGIN_FOLLOW, this );
		break;
	};
}

//---------------------------------------------------------
// Purpose: Spit Bomb Explode Logic
//---------------------------------------------------------
void CAntKingSpitBomb::ExplodeTouch( CBaseEntity *pOther )
{
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();

	//Explode( pNewTrace, DMG_ACID );

	CTakeDamageInfo dmgInfo;
	dmgInfo.SetDamage( sk_antlionking_spitbomb_damage.GetFloat() );
	dmgInfo.SetDamageType( DMG_ACID );
	dmgInfo.SetDamagePosition( pTrace->endpos );
	dmgInfo.SetAttacker( this );
	dmgInfo.SetInflictor( this );

	RadiusDamage( dmgInfo, pTrace->endpos, sk_antlionking_spitbomb_radius.GetFloat(), CLASS_NONE, NULL, 0 );

	// Explode our model.
	Vector vecOrigin = GetAbsOrigin();
	QAngle vecAngles = GetAbsAngles();
	Vector vecVelocity;
	AngularImpulse vecAngVel;
	CPASFilter filter( WorldSpaceCenter() );
	GetVelocity( &vecVelocity, &vecAngVel );

	// Particles!
	switch ( m_iSize )
	{
	case 0:
		DispatchParticleEffect( "antking_spit_small_explosion", GetAbsOrigin(), GetAbsAngles() );
		break;
	case 1:
	default:
		DispatchParticleEffect( "antking_spit_medium_explosion", GetAbsOrigin(), GetAbsAngles() );
		break;
	case 2:
		DispatchParticleEffect( "antking_spit_large_explosion", GetAbsOrigin(), GetAbsAngles() );
		break;
	}

	float flDamageRadiusTable[3] = { 150, 200, 256 };
	
	EmitSound( "EggClutch.Explode" );
	/*ExplosionCreate(WorldSpaceCenter(), vecAngles, this, 15, flDamageRadiusTable[m_iSize],
				SF_ENVEXPLOSION_NODAMAGE | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NOPARTICLES | SF_ENVEXPLOSION_NOFIREBALL | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE | SF_ENVEXPLOSION_SURFACEONLY | SF_ENVEXPLOSION_NOSOUND,
				0.0f, this ); */

	breakablepropparams_t propParams( vecOrigin, vecAngles, vecVelocity, vecAngVel );
	propParams.impactEnergyScale = 10.0f;
	propParams.defCollisionGroup = GetCollisionGroup();
	
	// NOTE: This may need to be tweaked for multiplayer? See props.cpp
	PropBreakableCreateAll( GetModelIndex(), VPhysicsGetObject(), propParams, this, -1, true, false );
	Break( pOther, dmgInfo );
}


//==================================================
// CAntKingPukeZone - 
// A trigger volume that follows the king's puke attacks
// Deals damage to the player as they stand in it
//==================================================
class CAntKingPukeZone : public CBaseTrigger
{
	DECLARE_CLASS( CAntKingPukeZone, CBaseTrigger );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

public:

	virtual void	Spawn( void );
	virtual void	Activate( void );
	virtual void	StartTouch( CBaseEntity *pOther );
	virtual void	EndTouch( CBaseEntity *pOther );

	virtual void	HurtThink( void );

	virtual int		UpdateTransmitState( void ) { return SetTransmitState( FL_EDICT_PVSCHECK ); } // NOTE: we transmit to the client for particles

private:
	float			m_LifeTime;
	float			m_NextDamageTime;

	CHandle<CBasePlayer> m_hTouchingPlayer;
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CAntKingPukeZone )

	DEFINE_FIELD( m_hTouchingPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( m_LifeTime, FIELD_TIME ),
	DEFINE_FIELD( m_NextDamageTime, FIELD_TIME ),

	DEFINE_ENTITYFUNC( HurtThink )

END_DATADESC()

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CAntKingPukeZone, DT_AntKingPukeZone )
END_SEND_TABLE()


//---------------------------------------------------------
// Purpose: Spawn entity into world
//---------------------------------------------------------
void CAntKingPukeZone::Spawn( void )
{
	if ( !GetGroundEntity() )
	{
		// The player isn't on the ground, so pull us down to sit on the floor.
		trace_t floorTrace;
		Vector vecUp;
		GetVectors( NULL, NULL, &vecUp );
		Vector vecDown = GetAbsOrigin() + ( -vecUp * MAX_TRACE_LENGTH );

		UTIL_TraceLine( GetAbsOrigin(), vecDown, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &floorTrace );

		Vector newOrigin = GetAbsOrigin();
		newOrigin.z = floorTrace.endpos.z;
		SetAbsOrigin( newOrigin );
	}

	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
	SetSize( -Vector( kAntKing_PukeZone_Length, kAntKing_PukeZone_Width, kAntKing_PukeZone_Height ), Vector( kAntKing_PukeZone_Length, kAntKing_PukeZone_Width, kAntKing_PukeZone_Height ) );

	// Fit the zone to the geometry we're placed on (this way we don't deal damage in the air over ledges).
	// NOTE: These aren't used right now.
	{
		enum ePukeZoneCorners { FRONT_LEFT = 0, FRONT_RIGHT = 1, BACK_LEFT = 2, BACK_RIGHT = 3, MAX_CORNERS = 4 };
		Vector vecForward, vecRight;
		Vector vecCorners[MAX_CORNERS];
		GetVectors( &vecForward, &vecRight, NULL );

		//vecForwardLeftCorner = ( vecForward * kAntKing_PukeZone_Length );
		vecCorners[FRONT_LEFT] = GetAbsOrigin() + ( vecForward * kAntKing_PukeZone_Length ) - ( vecRight * kAntKing_PukeZone_Width );
		vecCorners[FRONT_RIGHT] = GetAbsOrigin() + ( vecForward * kAntKing_PukeZone_Length ) + ( vecRight * kAntKing_PukeZone_Width );
		vecCorners[BACK_LEFT] = GetAbsOrigin() + ( -vecForward * kAntKing_PukeZone_Length ) - ( vecRight * kAntKing_PukeZone_Width );
		vecCorners[BACK_RIGHT] = GetAbsOrigin() + ( -vecForward * kAntKing_PukeZone_Length ) + ( vecRight * kAntKing_PukeZone_Width );

		// Make sure that all of our corners are on the ground.
		bool bCornerOnGround[4];
		bool bResizeNecessary = false;
		trace_t tr;

		for ( int corner = 0; corner < MAX_CORNERS; corner++ )
		{
			// Clear out any leftover data.
			UTIL_ClearTrace( tr );
			bCornerOnGround[corner] = false;
			//DevMsg( "Checking Corner: %d\n", corner );

			// Perform the check!
			UTIL_TraceLine( vecCorners[corner], Vector( vecCorners[corner].x, vecCorners[corner].y, vecCorners[corner].z - 50 ), MASK_ALL, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction < 0.99f && tr.DidHitWorld() )
			{
				// We did collide with the the ground, we're good!
				bCornerOnGround[corner] = true;
				bResizeNecessary = true;
			}
			//NDebugOverlay::Cross3D( tr.endpos, 255, 0, 0, 255, true, 30.0f );
			//DevMsg( "Corner %d on ground? %s\n", corner, bCornerOnGround[corner] ? "true" : "false" );
		}

		const int kPukeZoneShinkFactor = 30.0f;	// We will check for valid surface placement in shrinking increments of 30 units per check.

		// We need to pull the corners closer to the origin until the volume is snug on the surface.
		{
			// Front left corner
			if ( !bCornerOnGround[FRONT_LEFT] )
			{
				int numShrinkChecks = kAntKing_PukeZone_Length / kPukeZoneShinkFactor;
				int currCheck = 0;
				trace_t curTR;

				//NDebugOverlay::Cross3D( vecCorners[FRONT_LEFT], 100, 255, 0, 0, true, 15.0f );
				while ( numShrinkChecks > 0 && !curTR.DidHitWorld() )
				{
					Vector vecNewEndPos = GetAbsOrigin() + ( vecForward * ( kAntKing_PukeZone_Length - kPukeZoneShinkFactor * currCheck ) - ( vecRight * ( kAntKing_PukeZone_Width - kPukeZoneShinkFactor * currCheck ) ) );
					UTIL_TraceLine( vecNewEndPos, Vector( vecNewEndPos.x, vecNewEndPos.y, vecNewEndPos.z - 50 ), MASK_ALL, this, COLLISION_GROUP_NONE, &curTR );
					
					//NDebugOverlay::Cross3D( vecNewEndPos, 100, 0, 255, 0, true, 15.0f );
					if ( curTR.DidHitWorld() )
					{
						//NDebugOverlay::Cross3D( vecNewEndPos, 100, 0, 0, 255, true, 15.0f );
						vecCorners[FRONT_LEFT] = vecNewEndPos;
					}

					currCheck++;
					numShrinkChecks--;
				}
			}

			// Front right corner
			if ( !bCornerOnGround[FRONT_RIGHT] )
			{
				int numShrinkChecks = kAntKing_PukeZone_Length / kPukeZoneShinkFactor;
				int currCheck = 0;
				trace_t curTR;

				//NDebugOverlay::Cross3D( vecCorners[FRONT_RIGHT], 100, 255, 0, 0, true, 15.0f );
				while ( numShrinkChecks > 0 && !curTR.DidHitWorld() )
				{
					Vector vecNewEndPos = GetAbsOrigin() + ( vecForward * ( kAntKing_PukeZone_Length - kPukeZoneShinkFactor * currCheck ) + ( vecRight * ( kAntKing_PukeZone_Width - kPukeZoneShinkFactor * currCheck ) ) );
					UTIL_TraceLine( vecNewEndPos, Vector( vecNewEndPos.x, vecNewEndPos.y, vecNewEndPos.z - 50 ), MASK_ALL, this, COLLISION_GROUP_NONE, &curTR );
					
					//NDebugOverlay::Cross3D( vecNewEndPos, 100, 0, 255, 0, true, 15.0f );
					if ( curTR.DidHitWorld() )
					{
						//NDebugOverlay::Cross3D( vecNewEndPos, 100, 0, 0, 255, true, 15.0f );
						vecCorners[FRONT_RIGHT] = vecNewEndPos;
					}

					currCheck++;
					numShrinkChecks--;
				}
			}

			// Back left corner
			if ( !bCornerOnGround[BACK_LEFT] )
			{
				int numShrinkChecks = kAntKing_PukeZone_Length / kPukeZoneShinkFactor;
				int currCheck = 0;
				trace_t curTR;

				//NDebugOverlay::Cross3D( vecCorners[BACK_LEFT], 100, 255, 0, 0, true, 15.0f );
				while ( numShrinkChecks > 0 && !curTR.DidHitWorld() )
				{
					Vector vecNewEndPos = GetAbsOrigin() + ( -vecForward * ( kAntKing_PukeZone_Length - kPukeZoneShinkFactor * currCheck ) - ( vecRight * ( kAntKing_PukeZone_Width - kPukeZoneShinkFactor * currCheck ) ) );
					UTIL_TraceLine( vecNewEndPos, Vector( vecNewEndPos.x, vecNewEndPos.y, vecNewEndPos.z - 50 ), MASK_ALL, this, COLLISION_GROUP_NONE, &curTR );
					
					//NDebugOverlay::Cross3D( vecNewEndPos, 100, 0, 255, 0, true, 15.0f );
					if ( curTR.DidHitWorld() )
					{
						//NDebugOverlay::Cross3D( vecNewEndPos, 100, 0, 0, 255, true, 15.0f );
						vecCorners[BACK_LEFT] = vecNewEndPos;
					}

					currCheck++;
					numShrinkChecks--;
				}
			}

			// Back right corner
			if ( !bCornerOnGround[BACK_RIGHT] )
			{
				int numShrinkChecks = kAntKing_PukeZone_Length / kPukeZoneShinkFactor;
				int currCheck = 0;
				trace_t curTR;

				//NDebugOverlay::Cross3D( vecCorners[BACK_RIGHT], 100, 255, 0, 0, true, 15.0f );
				while ( numShrinkChecks > 0 && !curTR.DidHitWorld() )
				{
					Vector vecNewEndPos = GetAbsOrigin() + ( -vecForward * ( kAntKing_PukeZone_Length - kPukeZoneShinkFactor * currCheck ) + ( vecRight * ( kAntKing_PukeZone_Width - kPukeZoneShinkFactor * currCheck ) ) );
					UTIL_TraceLine( vecNewEndPos, Vector( vecNewEndPos.x, vecNewEndPos.y, vecNewEndPos.z - 50 ), MASK_ALL, this, COLLISION_GROUP_NONE, &curTR );
					
					//NDebugOverlay::Cross3D( vecNewEndPos, 100, 0, 255, 0, true, 15.0f );
					if ( curTR.DidHitWorld() )
					{
						//NDebugOverlay::Cross3D( vecNewEndPos, 100, 0, 0, 255, true, 15.0f );
						vecCorners[BACK_RIGHT] = vecNewEndPos;
					}

					currCheck++;
					numShrinkChecks--;
				}
			}
		}
	}

	//NDebugOverlay::EntityBounds( this, 255, 0, 0, 128, 15.0f );

	SetThink( &CAntKingPukeZone::HurtThink );
	SetNextThink( gpGlobals->curtime + 0.2f );
	Enable();

	m_LifeTime = gpGlobals->curtime + sk_antlionking_pukezone_lifetime.GetFloat();

	Activate();

	if ( g_debug_antlionking.GetInt() == 2 )
		NDebugOverlay::Box( GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs(), 255, 0, 0, 255, sk_antlionking_pukezone_lifetime.GetFloat() );
}

//---------------------------------------------------------
// Purpose: Spawn entity into world
//---------------------------------------------------------
void CAntKingPukeZone::Activate( void )
{
	m_hTouchingPlayer = NULL;

	SetThink( &CAntKingPukeZone::HurtThink );
	SetNextThink( gpGlobals->curtime + 0.2f );

	BaseClass::Activate();
}

//---------------------------------------------------------
// Purpose: A player has just entered/touched the bounds
//			of our trigger volume.
//---------------------------------------------------------
void CAntKingPukeZone::StartTouch( CBaseEntity *pOther )
{
	if ( pOther && pOther->IsPlayer() )
	{
		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>( pOther );
		m_hTouchingPlayer = pPlayer;
	}
}

//---------------------------------------------------------
// Purpose: A player has just exited the bounds
//			of our trigger volume.
//---------------------------------------------------------
void CAntKingPukeZone::EndTouch( CBaseEntity *pOther )
{
	if ( pOther && pOther->IsPlayer() )
	{
		m_hTouchingPlayer = NULL;
	}
}

//---------------------------------------------------------
// Purpose: Our periodic think function.
// If the player is within our bounds, deal damage to them
// Also keep track of when we should be removed from the world.
//---------------------------------------------------------
void CAntKingPukeZone::HurtThink( void )
{
	if ( m_LifeTime < gpGlobals->curtime )
	{
		// Our time is up, dissolve this zone.
		
		if ( g_debug_antlionking.GetInt() == 2 )
		{
			DevMsg( "Destroying Puke Zone\n" );
		}

		StopSound( entindex(), "NPC_AntlionKing.Acidloop" );

		SetThink( NULL );
		UTIL_Remove( this );
	}

	SetNextThink( gpGlobals->curtime + 0.2f );

	if ( !m_hTouchingPlayer || m_NextDamageTime > gpGlobals->curtime )
		return;

	CTakeDamageInfo dmgInfo;
	dmgInfo.SetDamage( 5.0f );
	dmgInfo.SetDamageType( DMG_RADIATION );

	if ( m_hTouchingPlayer )
	{
		m_hTouchingPlayer->TakeDamage( dmgInfo );

		// Do a quick screen fade.
		color32 fadeColor = { 0,96, 15, 255 };
		UTIL_ScreenFade( m_hTouchingPlayer, fadeColor, 0.4f, 0.0f, FFADE_IN );
	}
	
	m_NextDamageTime = gpGlobals->curtime + 1.5f;
}

//-----------------------------------------------------------------------------
// Purpose: A special trace filter for the King's shock attack.
//			Only concerned with the player and physics objects.
//-----------------------------------------------------------------------------
class CTraceFilterPhysicsShock : public CTraceFilterEntitiesOnly
{
public:
	// It does have a bass, but we'll never network anything below here.
	DECLARE_CLASS_NOBASE( CTraceFilterPhysicsShock );

	CTraceFilterPhysicsShock( IHandleEntity *passEntity, int iCollisionGroup, CTakeDamageInfo *dmgInfo, float flForceScale, bool bDamageAnyNPC, CBaseEntity *pIgnoreEnt, Vector vecForceOrigin, Vector vecShoveDir )
		: m_pPassEnt( passEntity ), m_iCollisionGroup( iCollisionGroup ), m_pDmgInfo( *dmgInfo ), m_flForceScale( flForceScale ), m_bDamageAnyNPC( bDamageAnyNPC ), m_pIgnoreEnt( pIgnoreEnt ), m_vecForceOrigin( vecForceOrigin ), m_vecShoveDir( vecShoveDir )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int iContentsMask );

private:
	const IHandleEntity *m_pPassEnt;
	int					m_iCollisionGroup;
	CTakeDamageInfo		m_pDmgInfo;
	CBaseEntity			*m_pHit;
	CBaseEntity			*m_pIgnoreEnt;
	Vector				m_vecForceOrigin;
	Vector				m_vecShoveDir;
	float				m_flForceScale;
	bool				m_bDamageAnyNPC;
	static float		m_flTimeNextHitPlayer;
};

float CTraceFilterPhysicsShock::m_flTimeNextHitPlayer = 0.0f;

//-----------------------------------------------------------------------------
// Purpose: Checks if this shockwave filter should push the given entity.
//			If so, it applies a force to the entity.
//-----------------------------------------------------------------------------
bool CTraceFilterPhysicsShock::ShouldHitEntity( IHandleEntity *pHandleEntity, int iContentsMask )
{
	if ( !StandardFilterRules( pHandleEntity, iContentsMask ) )
		return false;

	if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
		return false;

	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );

	if ( pEntity )
	{
		// Don't test if the game code tells us we should ignore it...
		if ( !pEntity->ShouldCollide( m_iCollisionGroup, iContentsMask ) )
			return false;

		if ( !g_pGameRules->ShouldCollide( m_iCollisionGroup, pEntity->GetCollisionGroup() ) )
			return false;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			return false;

		if ( pEntity == m_pIgnoreEnt )
			return false;

		// Game checks passed...
		// Out of all the remaining entities, we ONLY care about the player and physics props.
		// NOTE: We're doing a 'string' check here because (for whatever reason) some prop_dynamics are being counted as physics entities.
		if ( !pEntity->IsPlayer() && ( pEntity->VPhysicsGetObject() == NULL || !EntIsClass( pEntity, s_PropPhysicsClassname ) ) )
			return false;

		if ( pEntity->IsPlayer() && gpGlobals->curtime < m_flTimeNextHitPlayer )
			return false;

		// We know we can be affected now...
		// Send it flying!
		//Warning( "Hit entity '%s'. Has Physics: '%s'\n", pEntity->GetDebugName(), pEntity->VPhysicsGetObject() ? "y" : "n" );
		//NDebugOverlay::EntityBounds( pEntity, 0, 255, 0, 50, 10.0f );

		CTakeDamageInfo dmgInfo = m_pDmgInfo;
		Vector vecShoveVel = m_vecShoveDir * 600.0f;

		// Add some vertical momentum to throw them around.
		vecShoveVel += Vector( 0, 0, 1 ) * 600.0f;

		vecShoveVel.NormalizeInPlace();
		CalculateMeleeDamageForce( &dmgInfo, vecShoveVel, m_vecForceOrigin, m_flForceScale );

		// Handle setting the player's velocity separately.
		if ( pEntity->IsPlayer() )
		{
			pEntity->SetAbsVelocity( vecShoveVel );
			m_flTimeNextHitPlayer = gpGlobals->curtime + 3.0f;
			//Warning( "PLAYER HIT!\n" );
		}
		else
		{
			// We ONLY want to hit players, not physics objects.
			return false;
		}

		pEntity->TakeDamage( dmgInfo );

		m_pHit = pEntity;
		return true;
	}

	return false;
}

//==================================================
// CAntKingShockwaveVolume - A trigger volume that
// moves away from the King, wreaking havoc to 
// all in it's path.
//==================================================
class CAntKingShockwaveVolume : public CBaseTrigger
{
public:
	DECLARE_CLASS( CAntKingShockwaveVolume, CBaseTrigger );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	void	ShockwaveInit( Vector vecDir );
	void	ShockwaveThink( void );

	virtual int		UpdateTransmitState( void ) { return SetTransmitState( FL_EDICT_PVSCHECK ); } // NOTE: we transmit to the client for particles

private:
	Vector	m_vecDir;
	Vector	m_vecLastCheckPos;
	float	m_flLastMoveThink;
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CAntKingShockwaveVolume )

	DEFINE_FIELD( m_vecDir, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecLastCheckPos, FIELD_VECTOR ),
	DEFINE_FIELD( m_flLastMoveThink, FIELD_TIME ),

	// Think context
	DEFINE_ENTITYFUNC( CAntKingShockwaveVolume::ShockwaveThink )

END_DATADESC()

//---------------------------------------------------------
// Networking
//---------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CAntKingShockwaveVolume, DT_AntKingShockwaveVolume )
END_SEND_TABLE()

//---------------------------------------------------------
// Purpose: Sets up specific state for the shockwave.
//---------------------------------------------------------
void CAntKingShockwaveVolume::ShockwaveInit( Vector vecDir )
{
	//m_pPhysicsObjectsAffected.SetCollisionGroup( COLLISION_GROUP_INTERACTIVE | COLLISION_GROUP_PLAYER );

	//DevMsg( "Initializing Shockwave!\n" );
	//NDebugOverlay::EntityBounds( this, 255, 0, 0, 96, 30.0f );
	//NDebugOverlay::BoxAngles( GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs(), GetAbsAngles(), 255, 0, 0, 96, 30.0f );
	//NDebugOverlay::HorzArrow( GetAbsOrigin(), GetAbsOrigin() + ( vecDir * 10.0f ), 30.0f, 0.0f, 0.0f, 255.0f, 255.0f, true, 30.0f );

	// Save off our direction vector.
	m_vecDir = vecDir;
	m_flLastMoveThink = gpGlobals->curtime;
	m_vecLastCheckPos = GetAbsOrigin();

	// We need to call this very often...
	SetContextThink( &CAntKingShockwaveVolume::ShockwaveThink, gpGlobals->curtime + 0.01f, "ShockwaveThink" );

	if ( g_debug_antlionking.GetInt() == 3 )
	{
		NDebugOverlay::Box( GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs(), 0, 255, 0, 96, 1.0f );
	}
}

//---------------------------------------------------------
// Purpose: Periodically slide us along our direction vector.
// Wreaking havok all along the way! heh... havok.
//---------------------------------------------------------
void CAntKingShockwaveVolume::ShockwaveThink( void )
{
	Vector vecNewPos;
	VectorMA( GetAbsOrigin(), sk_antlionking_shockwave_advance_rate.GetFloat(), m_vecDir, vecNewPos );

	// Perform a tracehull to see what we need to send flying.
	trace_t traceHull{};
	CTakeDamageInfo dmgInfo( this, this, sk_antlionking_shockwave_damage.GetFloat(), DMG_ENERGYBEAM);
	CTraceFilterPhysicsShock filter( this, COLLISION_GROUP_PLAYER, &dmgInfo, 1.0f, true, this, GetAbsOrigin(), m_vecDir );
	AI_TraceHull( GetAbsOrigin(), vecNewPos, WorldAlignMins(), WorldAlignMaxs(), MASK_ALL, &filter, &traceHull );

	SetAbsOrigin( vecNewPos );

	// If we've been pushed out of the world, time to remove ourselves!
	trace_t worldTR;
	CTraceFilterWorldOnly worldFilter;
	UTIL_TraceLine( vecNewPos, Vector{ vecNewPos.x, vecNewPos.y, vecNewPos.z - 50000.0f }, MASK_ALL, &worldFilter, &worldTR );

	if ( !worldTR.DidHitWorld() )
	{
		//DevMsg( "Shockwave volume fell out of world! Removing!\n" );
		
		SetThink( &CAntKingShockwaveVolume::SUB_Remove );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	m_flLastMoveThink = gpGlobals->curtime;
	SetNextThink( gpGlobals->curtime + 0.01f, "ShockwaveThink" );

	if ( g_debug_antlionking.GetInt() == 3 )
	{
		NDebugOverlay::Box( GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs(), 0, 255, 0, 96, 1.0f );
	}
}

//==================================================
// CNPC_AntlionKing - 
// Big Climactic Antlion Boss
//==================================================
class CNPC_AntlionKing : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_AntlionKing, CAI_BaseNPC );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

public:
	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void Activate( void );
	virtual Class_T Classify( void ) { return CLASS_ANTLION; }
			void InitBoneFollowers( void );
			void	CachePoseParameters( void );

	virtual void	OnRestore( void );
	virtual bool	CreateVPhysics( void );
	virtual void	HandleAnimEvent( animevent_t *pEvent );
	virtual void	PrescheduleThink( void );
	virtual int		SelectSchedule( void );
	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );
	virtual float	MaxYawSpeed( void );
	virtual int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual Activity NPC_TranslateActivity( Activity baseAct );
	virtual int		MeleeAttack1Conditions( float flDot, float flDist );

	void			UpdateHeading( void );

	void			SpitAttack( void );
	void			PukeAttack( void );
	void			ShockwaveAttack( void );

	// Sounds
	void			IdleSound( void );

	// We are not stunned by the steambow.
	virtual bool	CanBeStunnedBySteambow( void ) const { return false; }

	// Input Handlers
	void			Input_SetAttackState( inputdata_t &data );
	void			Input_ResetDamageStats( inputdata_t &data );
	void			Input_DisableStunState( inputdata_t &data ) { m_bCanBeStunned = false; }
	void			Input_EnableStunState( inputdata_t &data ) { m_bCanBeStunned = true; }
	void			Input_EnableTurning( inputdata_t &data ) { m_bIsSteeringActive = true; }
	void			Input_DisableTurning( inputdata_t &data ) { m_bIsSteeringActive = false; }
	void			Input_EnableHeadTurning( inputdata_t &data ) { m_bIsHeadTurningActive = true; }
	void			Input_DisableHeadTurning( inputdata_t &data );
	void			Input_EnableShield( inputdata_t &data );
	void			Input_DisableShield( inputdata_t &data );
	void			Input_LongLiveTheKing( inputdata_t &data );

	// Output Events
	COutputEvent	m_OnStunnedOutput;
	COutputEvent	m_OnStunFinishedOutput;
	COutputEvent	m_OnBombedOutput;
	COutputEvent	m_OnAttackEvent;
	COutputEvent	m_OnPukeAttackEvent;

private:
	int				m_AntKingAttackState;
	int				m_NumTimesHitByBomb;
	int				m_NumTimesStunned;
	float			m_flAttackRate;
	float			m_flLastPukeTime;

	bool			m_bCanBeStunned;
	bool			m_IsWeakpointExposed;
	float			m_flWeakpointDamageSustained;
	bool			m_bDying;

	float			m_flHeadPitch;
	float			m_flHeadYaw;
	float			m_flHeadRoll;

	int				m_iHeadPitchParam;
	int				m_iHeadYawParam;
	int				m_iHeadRollParam;
	int				m_iSpitAttachIndex;

	Vector			m_vecPukePos;

	bool			m_bIsSteeringActive;
	bool			m_bIsHeadTurningActive;
	// If true, the code will attempt to steer his head back to the front overtime.
	bool			m_bSteerHeadBackToFront;
	float			m_flTimeHeadTurnDisabled;

	// We need this for collision against our model (BBoxes won't suffice).
	CBoneFollowerManager m_BoneFollowerManager;

	// Sound timers
	float			m_flNextIdleSoundTime;

	// Shield stuff
	CNetworkVar( bool, m_bShieldActive );

private:

	//==================================================
	// AntlionKing Schedules
	//==================================================
	enum
	{
		SCHED_ANTLIONKING_SPIT = LAST_SHARED_SCHEDULE,
		SCHED_ANTLIONKING_PUKE,
		SCHED_ANTLIONKING_SHOCKWAVE,
		SCHED_ANTLIONKING_EXPOSE_WEAKPOINT,
		SCHED_ANTLIONKING_WEAKPOINT_LOOP,
		SCHED_ANTLIONKING_WEAKPOINT_END,
	};


	//==================================================
	// AntlionKing Tasks
	//==================================================
	enum
	{
		TASK_ANTLIONKING_PUKE_ATTACK = LAST_SHARED_TASK,
		TASK_ANTLIONKING_SHOCKWAVE_ATTACK,
		TASK_ANTLIONKING_EXPOSE_WEAKPOINT,
		TASK_ANTLIONKING_WEAKPOINT_LOOP,
		TASK_ANTLIONKING_END_WEAKPOINT_LOOP,

		TASK_ANTLIONKING_DRAW_PUKE_DEBUG_LINES,
	};
	
	//==================================================
	// AntlionKing Conditions
	//==================================================
	enum
	{
		COND_ANTLIONKING_HIT_BY_BOMB = LAST_SHARED_CONDITION,
	};
};

enum AntKingAttackState
{
	ANTKING_ATTACK_NONE = 0,
	ANTKING_ATTACK_SPIT,
	ANTKING_ATTACK_PUKE,
	ANTKING_ATTACK_SHOCKWAVE,
	ANTKING_ATTACK_PUKE_MAPTRIGGERS,

	ANTKING_ATTACK_MAX_STATES
};

LINK_ENTITY_TO_CLASS( antking_spitbomb, CAntKingSpitBomb );
LINK_ENTITY_TO_CLASS( trigger_antking_pukezone, CAntKingPukeZone );
LINK_ENTITY_TO_CLASS( trigger_antking_shockwave_volume, CAntKingShockwaveVolume );
LINK_ENTITY_TO_CLASS( npc_antlionking, CNPC_AntlionKing );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_AntlionKing )

	DEFINE_FIELD( m_AntKingAttackState, FIELD_INTEGER ),
	DEFINE_FIELD( m_NumTimesHitByBomb, FIELD_INTEGER ),
	DEFINE_FIELD( m_NumTimesStunned, FIELD_INTEGER ),
	DEFINE_FIELD( m_flAttackRate, FIELD_FLOAT ),
	DEFINE_FIELD( m_flLastPukeTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_bCanBeStunned, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_IsWeakpointExposed, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDying, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flWeakpointDamageSustained, FIELD_FLOAT ),

	DEFINE_FIELD( m_flHeadPitch, FIELD_FLOAT ),
	DEFINE_FIELD( m_flHeadYaw, FIELD_FLOAT ),
	DEFINE_FIELD( m_flHeadRoll, FIELD_FLOAT ),

	DEFINE_FIELD( m_vecPukePos, FIELD_VECTOR ),

	DEFINE_FIELD( m_bIsSteeringActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsHeadTurningActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bSteerHeadBackToFront, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flTimeHeadTurnDisabled, FIELD_TIME ),

	// We don't need to save/restore these, because they get populated on game load anyways.
	//DEFINE_FIELD( m_iHeadPitchParam, FIELD_INTEGER ),
	//DEFINE_FIELD( m_iHeadYawParam, FIELD_INTEGER ),
	//DEFINE_FIELD( m_iHeadRollParam, FIELD_INTEGER ),

	DEFINE_EMBEDDED( m_BoneFollowerManager ),

	DEFINE_FIELD( m_flNextIdleSoundTime, FIELD_TIME ),

	DEFINE_FIELD( m_bShieldActive, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetAttackState", Input_SetAttackState ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ResetDamageStats", Input_ResetDamageStats ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableStunState", Input_DisableStunState ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableStunState", Input_EnableStunState ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableTurning", Input_EnableTurning ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableTurning", Input_DisableTurning ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableHeadTurning", Input_EnableHeadTurning ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "DisableHeadTurning", Input_DisableHeadTurning ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableShield", Input_EnableShield ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableShield", Input_DisableShield ),
	DEFINE_INPUTFUNC( FIELD_VOID, "LongLiveTheKing", Input_LongLiveTheKing ),

	DEFINE_OUTPUT( m_OnStunnedOutput, "OnKingStunned" ),
	DEFINE_OUTPUT( m_OnStunFinishedOutput, "OnKingStunFinished" ),
	DEFINE_OUTPUT( m_OnBombedOutput, "OnBombed" ),
	DEFINE_OUTPUT( m_OnAttackEvent, "OnKingAttack" ),
	DEFINE_OUTPUT( m_OnPukeAttackEvent, "OnKingPukeAttack" ),

END_DATADESC()

//---------------------------------------------------------
// Networking
//---------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CNPC_AntlionKing, DT_NPC_AntlionKing )
	SendPropBool( SENDINFO( m_bShieldActive ) ),
END_SEND_TABLE()

//---------------------------------------------------------
// Purpose: Precache needed resources
//---------------------------------------------------------
void CNPC_AntlionKing::Precache( void )
{
	PrecacheModel( kAntlionKing_Model );

	UTIL_PrecacheOther( "antking_spitbomb" );
	UTIL_PrecacheOther( "trigger_antking_pukezone" );

	// Precache sounds
	PrecacheScriptSound( "NPC_AntlionKing.Idle" );
	PrecacheScriptSound( "NPC_AntlionKing.Spit" );
	PrecacheScriptSound( "NPC_AntlionKing.Puke" );
	PrecacheScriptSound( "NPC_AntlionKing.Slam" );
	PrecacheScriptSound( "NPC_AntlionKing.Damaged" );
	PrecacheScriptSound( "NPC_AntlionKing.Swipe" );
	PrecacheScriptSound( "NPC_AntlionKing.Steps" );
	PrecacheScriptSound( "NPC_AntlionKing.Shield1" );
	PrecacheScriptSound( "NPC_AntlionKing.Shield2" );
	PrecacheScriptSound( "NPC_AntlionKing.StompShockwave" );
	PrecacheScriptSound( "NPC_AntlionKing.PukeStart" );
	PrecacheScriptSound( "NPC_AntlionKing.Acidloop" );

	PrecacheParticleSystem( "blood_impact_antlion_01" );
	PrecacheParticleSystem( "antking_pukespray" );
	PrecacheParticleSystem( "antking_shockwave" );

	BaseClass::Precache();
}

//---------------------------------------------------------
// Purpose: Spawn entity into world
//---------------------------------------------------------
void CNPC_AntlionKing::Spawn( void )
{
	Precache();
	
	BaseClass::Spawn();

	SetModel( kAntlionKing_Model );
	SetHullType( HULL_LARGE_CENTERED );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE  );
	SetMoveType( MOVETYPE_NONE );
	SetBloodColor( BLOOD_COLOR_ANTLION );
	m_iHealth = sk_antlionking_health.GetInt();
	m_flFieldOfView = 0.5; // indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState = NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK2 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_MOVE_GROUND );

	m_AntKingAttackState = ANTKING_ATTACK_NONE; // NOTE: Sensible default
	m_NumTimesHitByBomb = 0;
	m_NumTimesStunned = 0;
	m_flAttackRate = kKingAttackInterval_0;

	m_bCanBeStunned = true;
	m_IsWeakpointExposed = false;
	m_flWeakpointDamageSustained = 0.0f;
	m_bDying = false;
	m_flLastPukeTime = 0.0f;

	m_vecPukePos = vec3_invalid;

	m_bIsSteeringActive = false;
	m_bIsHeadTurningActive = true;
	m_bSteerHeadBackToFront = false;
	m_flTimeHeadTurnDisabled = 0.0f;

	// Store off our pose parameters.
	CachePoseParameters();

	/*
	// Cache off the head pose parameters.
	m_flHeadPitch = GetPoseParameter( m_iHeadPitchParam );
	m_flHeadYaw = GetPoseParameter( m_iHeadYawParam );
	m_flHeadRoll = GetPoseParameter( m_iHeadRollParam );*/

	// Zero out the head pose parameters.
	m_flHeadPitch = 0.0f;
	m_flHeadYaw = 0.0f;
	m_flHeadRoll = 0.0f;
	SetPoseParameter( m_iHeadPitchParam, m_flHeadPitch );
	SetPoseParameter( m_iHeadYawParam, m_flHeadYaw );
	SetPoseParameter( m_iHeadRollParam, m_flHeadRoll );

	// We need to collide with pretty much everything.
	// No, we're not a strider, but have similar collision requirements so this should work fine...
	SetCollisionGroup( COLLISION_GROUP_NPC );

	// WE NEED TO BE USING OUR HITBOXES!!
	// Figuring this out took way too long... :(
	CollisionProp()->SetSurroundingBoundsType( USE_HITBOXES );

	// Setup shield stuff.
	m_bShieldActive = false;

	NPCInit();
}

//---------------------------------------------------------
// Purpose: Spawn entity into world
//---------------------------------------------------------
void CNPC_AntlionKing::Activate( void )
{
	BaseClass::Activate();

	// Massively extend our attack range.
	// This gets overwritten by NPCInit(), so this needs to happen later.
	SetDistLook( 10000 );
	m_flDistTooFar = 10000;

	// Set sound timers.
	m_flNextIdleSoundTime = gpGlobals->curtime;
}

//---------------------------------------------------------
// Purpose: Sets up bone followers needed for more complex collision.
//---------------------------------------------------------
void CNPC_AntlionKing::InitBoneFollowers( void )
{
	// Only do this if we haven't already loaded them.
	if ( m_BoneFollowerManager.GetNumBoneFollowers() != 0 )
		return;

	m_BoneFollowerManager.InitBoneFollowers( this, ARRAYSIZE( pKingFollowerBoneNames ), pKingFollowerBoneNames );
}

//---------------------------------------------------------
// Purpose: Input handler for setting the King's attack state
// 0 is No Attack
// 1 is Spit Attack
// 2 is Puke Attack
// 3 is Shockwave Attack
// 4 is Puke Attack with map triggers (set via "Area Denial" triggers keyvalue)	(This is nonfunctional!)
//---------------------------------------------------------
void CNPC_AntlionKing::Input_SetAttackState( inputdata_t &inputdata )
{
	if ( inputdata.value.Int() < ANTKING_ATTACK_NONE || inputdata.value.Int() > ANTKING_ATTACK_MAX_STATES )
	{
		DevMsg( "Antlion King -> SetAttackState, parameter is out of range\n" );
		return;
	}

	m_AntKingAttackState = inputdata.value.Int();
}

//---------------------------------------------------------
// Purpose: Input handler for resetting the damage stats from map code.
// 
// Basically, if the King has taken enough damage to be flinging bombs 'really fast',
// then this input can be used to set him back to his 'initial' flinging speed.
//---------------------------------------------------------
void CNPC_AntlionKing::Input_ResetDamageStats( inputdata_t &inputdata )
{
	m_flAttackRate = kKingAttackInterval_0;
	m_flWeakpointDamageSustained = 0.0f;
	m_IsWeakpointExposed = false;
}

//---------------------------------------------------------
// Purpose: Input handler for disabling head turning.
//---------------------------------------------------------
void CNPC_AntlionKing::Input_DisableHeadTurning( inputdata_t &data )
{
	m_bIsHeadTurningActive = false;
	m_flTimeHeadTurnDisabled = gpGlobals->curtime;

	if ( data.value.Int() == 1 )
		m_bSteerHeadBackToFront = true;
	else
		m_bSteerHeadBackToFront = false;
}

//---------------------------------------------------------
// Purpose: Input handler for enabling the shield.
//---------------------------------------------------------
void CNPC_AntlionKing::Input_EnableShield( inputdata_t &data )
{
	EmitSound( "NPC_AntlionKing.Shield1" );
	EmitSound( "NPC_AntlionKing.Shield2" );
	m_bShieldActive = true; 
}

//---------------------------------------------------------
// Purpose: Input handler for disabling the shield.
//---------------------------------------------------------
void CNPC_AntlionKing::Input_DisableShield( inputdata_t &data )
{
	StopSound( "NPC_AntlionKing.Shield1" );
	StopSound( "NPC_AntlionKing.Shield2" );
	m_bShieldActive = false;
}

//---------------------------------------------------------
// Purpose: Special death handler for the king.
//---------------------------------------------------------
void CNPC_AntlionKing::Input_LongLiveTheKing( inputdata_t &inputdata )
{
	/*m_bDying = true; // Make sure we actually take the damage.
	CTakeDamageInfo killInfo( this, this, GetHealth() + 20, DMG_PHYSGUN );
	TakeDamage( killInfo );*/

	m_AntKingAttackState = 0;
	SetSchedule( SCHED_DIE_RAGDOLL );
}

//---------------------------------------------------------
// Purpose: The King does not take damage via conventional methods.
//---------------------------------------------------------
int CNPC_AntlionKing::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// If we were hit by a bomb, then we need to store that off.
	if ( FClassnameIs( info.GetInflictor(), "prop_physics" ) && ( info.GetDamageType() & DMG_BLAST ) )
	{
		ResetIdealActivity( ACT_PAIN );
		//EmitSound( "NPC_AntlionKing.Damaged" );

		// Make sure we push out our next attack, so that the attack CANNOT interrupt the flinch animation.
		m_flNextAttack += 3.0f;

		// We only care about this if we can actually be stunned. Otherwise, we're not keeping count.
		if ( m_bCanBeStunned )
			m_NumTimesHitByBomb++;

		// Even though we are handling damage differently than most NPCs, we still want to notify the i/o system when we're bombed.
		m_OnBombedOutput.FireOutput( this, this );
	}

	// NOTE: This is not really a good place to do this... but it's the fastest way to 
	// get the king to expose it's weakpoint.
	if ( m_NumTimesHitByBomb >= 3 && !m_IsWeakpointExposed )
	{
		// Damaging the King while he's exposing his weakspot will cause him to call into here multiple times, make sure to catch these to avoid it.
		if ( IsCurSchedule( SCHED_ANTLIONKING_EXPOSE_WEAKPOINT ) )
		{
			// I'm already stunned! Don't do any of the processing below.
			return 0;
		}

		SetSchedule( SCHED_ANTLIONKING_EXPOSE_WEAKPOINT );
		SetNextThink( gpGlobals->curtime );
		m_NumTimesStunned++;

		if ( g_debug_antlionking.GetInt() > 0 )
		{
			DevMsg( "KING CHANGING ATTACK INTERVAL: %d\n", m_NumTimesStunned );
		}

		// We attack faster the more we've been damaged.
		switch ( m_NumTimesStunned )
		{
		case 0:
			m_flAttackRate = kKingAttackInterval_0;
			break;
		case 1:
			m_flAttackRate = kKingAttackInterval_1;
			break;
		case 2:
			m_flAttackRate = kKingAttackInterval_2;
			break;
		}
	}

	if ( m_IsWeakpointExposed )
	{
		m_flWeakpointDamageSustained += info.GetDamage();

		if ( m_flWeakpointDamageSustained >= sk_antlionking_weakpoint_damage_threshold.GetFloat() )
		{
			// We've been damaged enough! Time to return to our regular battle state.
			m_NumTimesHitByBomb = 0;
			m_IsWeakpointExposed = false;
			m_flWeakpointDamageSustained = 0.0f;

			// Immediately switch up schedules.
			SetSchedule( SCHED_ANTLIONKING_WEAKPOINT_END );
			m_OnStunFinishedOutput.FireOutput( this, this );
		}
	}

	// The King does not take any damage.
	// (Unless he's dying).
	if ( m_bDying )
		return GetHealth() + 20;

	return 0;
}

//---------------------------------------------------------
// Purpose: Trace attack handler.
//---------------------------------------------------------
void CNPC_AntlionKing::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// If our shield is active, completely ignore damage.
	// This includes NOT calling the base class to not create blood spurts.
	if ( m_bShieldActive )
		return;

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//---------------------------------------------------------
// Purpose: Translate Activity.
//---------------------------------------------------------
Activity CNPC_AntlionKing::NPC_TranslateActivity( Activity baseAct )
{
	if ( baseAct == ACT_RANGE_ATTACK1 )
		return ACT_SPIT;

	if ( baseAct == ACT_MELEE_ATTACK1 )
		return ACT_SWIPE;

	return BaseClass::NPC_TranslateActivity( baseAct );
}

//---------------------------------------------------------
// Purpose: Melee attack conditions.
//---------------------------------------------------------
int CNPC_AntlionKing::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( m_flNextAttack > gpGlobals->curtime )
	{
		return COND_NOT_FACING_ATTACK;
	}

	if (flDist < (150 * 3))
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}

	if ( flDist > (150 * 6) )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}

//---------------------------------------------------------
// Purpose: Update facing direction. (Borrowed from Strider code).
//---------------------------------------------------------
void CNPC_AntlionKing::UpdateHeading( void )
{
	if ( !GetEnemy() )
		return;

	// If we were knocked down, then clear out our look parameters.
	if ( m_IsWeakpointExposed )
	{
		m_flHeadYaw = 0.0f;
		m_flHeadPitch = 0.0f;
		m_flHeadRoll = 0.0f;

		SetPoseParameter( m_iHeadYawParam, 0.0f );
		SetPoseParameter( m_iHeadPitchParam, 0.0f );
		SetPoseParameter( m_iHeadRollParam, 0.0f );

		return;
	}

	// If the head is already facing the front, don't do anymore work.
	if ( m_bSteerHeadBackToFront )
	{
		if ( ( m_flHeadPitch < 0.001f && m_flHeadPitch > -0.001f ) && 
			 ( m_flHeadYaw < 0.001f && m_flHeadYaw > -0.001f ) &&
			 ( m_flHeadRoll < 0.001f && m_flHeadRoll > -0.001f ) )
		{
			return;
		}
	}

	float curYaw = GetPoseParameter( m_iHeadYawParam );
	float curPitch = GetPoseParameter( m_iHeadPitchParam );
	float curRoll = GetPoseParameter( m_iHeadRollParam );

	if ( !m_bIsHeadTurningActive && m_bSteerHeadBackToFront )
	{
		// In this particular case, we are not looking at a target, but attempting to steer back to the front over time.
		float flCurLerpPercent = ( gpGlobals->curtime - m_flTimeHeadTurnDisabled ) / 5.0f; // We want to turn over the course of 5 seconds.
		float flNewPitch = Lerp( flCurLerpPercent, curPitch, 0.0f );
		float flNewYaw = Lerp( flCurLerpPercent, curYaw, 0.0f );
		float flNewRoll = Lerp( flCurLerpPercent, curRoll, 0.0f );

		// 5 seconds is up! Time to stop!
		if ( flCurLerpPercent > 1.0f )
		{
			m_bSteerHeadBackToFront = false;
		}
	
		SetPoseParameter( m_iHeadPitchParam, flNewPitch );
		SetPoseParameter( m_iHeadYawParam, flNewYaw );
		SetPoseParameter( m_iHeadRollParam, flNewRoll );

		return;
	}

	// This code is mostly duplicated from CAI_BaseNPC::SetAim()
	// Thanks Valve for doing all the fun math!
	Vector vecEnemyPos = GetEnemy()->GetAbsOrigin();
	Vector vecEnemyDir = vecEnemyPos - GetAbsOrigin();

	// If we are currently raring back to spit, then we need to focus on our puke zone, not the enemy.
	if ( GetActivity() == ACT_PUKE )
	{
		vecEnemyPos = m_vecPukePos;
		vecEnemyDir = vecEnemyPos - GetAbsOrigin();
	}
	
	QAngle angDir;
	VectorAngles( vecEnemyDir, angDir );

	float newPitch = curPitch + 0.8 * UTIL_AngleDiff( UTIL_ApproachAngle( angDir.x, curPitch, 20 ), curPitch );

	float flRelativeYaw = UTIL_AngleDiff( angDir.y, GetAbsAngles().y );
	float newYaw = curYaw + UTIL_AngleDiff( flRelativeYaw, curYaw );

	newPitch = AngleNormalize( newPitch );
	newPitch = -newPitch; // NOTE: We need to invert pitch here
	newYaw = AngleNormalize( newYaw );

	// Roll is simply the yaw divided by 2.
	// Since the roll has half the range of the yaw, this looks good for our use case.
	float newRoll = newYaw / 2.0f;
	
	SetPoseParameter( m_iHeadPitchParam, newPitch );
	SetPoseParameter( m_iHeadYawParam, newYaw );
	SetPoseParameter( m_iHeadRollParam, newRoll );

	// Cache off our pose parameters.
	m_flHeadYaw = newYaw;
	m_flHeadPitch = newPitch;
	m_flHeadRoll = newRoll;
}

//---------------------------------------------------------
// Purpose: Store off our pose parameters so we don't
// do lookups each update.
//---------------------------------------------------------
void CNPC_AntlionKing::CachePoseParameters( void )
{
	m_iSpitAttachIndex = LookupAttachment( "SPITORIGIN" );
	m_iHeadPitchParam = LookupPoseParameter( "head_pitch" );
	m_iHeadYawParam = LookupPoseParameter( "head_yaw" );
	m_iHeadRollParam = LookupPoseParameter( "head_roll" );
}

//---------------------------------------------------------
// Purpose: Restore callback.
// Overridden so we can make sure we have our pose parameters.
//---------------------------------------------------------
void CNPC_AntlionKing::OnRestore( void )
{
	CachePoseParameters();
	BaseClass::OnRestore();
	CreateVPhysics();
}

//---------------------------------------------------------
// Purpose: Creates our VPhysics representation.
// Overridden because we use bone followers for collision
// (similar to the Strider), thus we do not need bbox collision.
//---------------------------------------------------------
bool CNPC_AntlionKing::CreateVPhysics()
{
	// Not calling the base class as we do not need a solid bbox.

	InitBoneFollowers();

	return true;
}

//---------------------------------------------------------
// Purpose: Animation Event Handling
//---------------------------------------------------------
void CNPC_AntlionKing::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_KING_SPIT_ATTACK )
	{
		SpitAttack();
		m_OnAttackEvent.FireOutput( this, this );
		return;
	}
	else if ( pEvent->event == AE_KING_PUKE_ATTACK )
	{
		PukeAttack();
		m_OnPukeAttackEvent.FireOutput( this, this );
		return;
	}
	else if ( pEvent->event == AE_KING_SHOCKWAVE_ATTACK )
	{
		ShockwaveAttack();
		m_OnAttackEvent.FireOutput( this, this );
		return;
	}
	else if ( pEvent->event == AE_KING_SWIPE_ATTACK )
	{
		Vector leftOrigin;
		Vector rightOrigin;

		GetAttachment( "LegImpactL", leftOrigin );
		GetAttachment( "LegImpactR", rightOrigin );

		if ( g_debug_antlionking.GetInt() == 3 )
		{
			//NDebugOverlay::Box( leftOrigin, Vector( -40, -40, -225 ), Vector( 40, 40, 225 ), 255, 0, 0, 128, 20.0f );
			NDebugOverlay::Box( rightOrigin, Vector( -40, -40, -225 ), Vector( 40, 40, 225 ), 255, 0, 0, 128, 20.0f );
		}

		// Deal the damage.
		trace_t meleeTr;
		Ray_t meleeRay;
		CTakeDamageInfo dmgInfo( this, this, sk_antlionking_melee_attack_dmg.GetFloat(), DMG_SLASH);
		CTraceFilterMelee meleeFilter( this, COLLISION_GROUP_NONE, &dmgInfo, 1.0, true );
		meleeRay.Init( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), -Vector( 40, 40, 255 ), Vector( 40, 40, 255 ) );
		enginetrace->TraceRay( meleeRay, MASK_SHOT_HULL, &meleeFilter, &meleeTr );

		if ( meleeTr.m_pEnt )
		{
			Vector vecTraceDir = ( meleeTr.endpos - meleeTr.startpos );
			VectorNormalize( vecTraceDir );

			// Push the thing we hurt away!
			Vector vecForce = vecTraceDir * ImpulseScale( 75, 600 );
			CTakeDamageInfo info( this, this, vecForce, meleeTr.endpos, 15, DMG_CLUB );
			meleeTr.m_pEnt->TakeDamage( info );
		}

		UTIL_ScreenShake( rightOrigin, 10.0f, 1.0f, 2.0f, 2000.0f, SHAKE_START, false );

		// Foot/Ground FX
		trace_t rightGroundTrace;
		float yaw = random->RandomInt( 0, 128 );
		UTIL_TraceLine( rightOrigin, rightOrigin * Vector( 1, 1, 1000.0f ), MASK_SHOT, this, COLLISION_GROUP_NONE, &rightGroundTrace);

		// Right foot FX
		if ( UTIL_PointContents( rightGroundTrace.endpos + Vector( 0, 0, 1 ) ) & MASK_WATER )
		{
			float flWaterZ = UTIL_FindWaterSurface( rightGroundTrace.endpos, rightGroundTrace.endpos.z, rightGroundTrace.endpos.z + 100.0f );

			CEffectData	data;
			data.m_fFlags = 0;
			data.m_vOrigin = rightGroundTrace.endpos;
			data.m_vOrigin.z = flWaterZ;
			data.m_vNormal = Vector( 0, 0, 1 );
			data.m_flScale = random->RandomFloat( 10.0, 14.0 );

			DispatchEffect( "watersplash", data );
		}
		else
		{
			for ( int i = 0; i < 3; i++ )
			{
				Vector dir = UTIL_YawToVector( yaw + i*120 ) * 10;
				VectorNormalize( dir );
				dir.z = 0.25;
				VectorNormalize( dir );
				g_pEffects->Dust( rightGroundTrace.endpos, dir, 12, 50 );
			}
		}
		// Swipe sound!
		//EmitSound( "NPC_AntlionKing.Swipe" );
		m_OnAttackEvent.FireOutput( this, this );
		
		return;
	}
	else if ( pEvent->event == AE_KING_STEP_FRONTRIGHT || pEvent->event == AE_KING_STEP_BACKRIGHT )
	{
		Vector vecRightLeg;
		GetAttachment( "LegImpactR", vecRightLeg );
		CPASAttenuationFilter filter( this, "NPC_AntlionKing.Steps" );
		EmitSound( filter, entindex(), "NPC_AntlionKing.Steps" );

		return;
	}
	else if ( pEvent->event == AE_KING_STEP_BACKLEFT || pEvent->event == AE_KING_STEP_FRONTLEFT )
	{
		Vector vecLeftLeg;
		GetAttachment( "LegImpactL", vecLeftLeg );
		CPASAttenuationFilter filter( this, "NPC_AntlionKing.Steps" );
		EmitSound( filter, entindex(), "NPC_AntlionKing.Steps" );

		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//---------------------------------------------------------
// Purpose: Preschedule Think
// 
// NOTE: Since the king cannot turn, the player can get into a position where
// the king's AI will try to 'turn' to attack them.
// 
// Since the king cannot attack, the king will be stuck trying to execute a turning schedule.
// So we just boot out here to get around that...
//---------------------------------------------------------
void CNPC_AntlionKing::PrescheduleThink( void )
{
	// Check sounds
	// NOTE: We now handle most of our sounds through animevents.
	/*if (gpGlobals->curtime > m_flNextIdleSound)
	{
		IdleSound();
	}*/

	/*if ((gpGlobals->curtime - GetTimeScheduleStarted() > 2.5f) && !m_IsWeakpointExposed) // NOTE: This should not be happening when the king is stunned btw!
	{
		// It has been too long since schedule selection, we are probably stuck on a facing schedule, just skip
		SetSchedule( SCHED_FAIL );
		DevMsg( "MOVING ON!!!\n" );
	}*/

	if ( m_bIsHeadTurningActive || m_bSteerHeadBackToFront )
		UpdateHeading();

	BaseClass::PrescheduleThink();

	m_BoneFollowerManager.UpdateBoneFollowers( this );
}

//---------------------------------------------------------
// Purpose: Schedule Selection
//---------------------------------------------------------
int CNPC_AntlionKing::SelectSchedule( void )
{
	int baseSched = BaseClass::SelectSchedule();

	if ( !GetEnemy() )
		return baseSched;

	switch( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		{
			if ( m_NumTimesHitByBomb == 3 )
			{
				if ( !m_IsWeakpointExposed )
				{
					return SCHED_ANTLIONKING_EXPOSE_WEAKPOINT;
				}
				else
				{
					return SCHED_ANTLIONKING_WEAKPOINT_LOOP;
				}
			}
			else if ( m_IsWeakpointExposed )
			{
				return SCHED_ANTLIONKING_WEAKPOINT_END;
			}
			
			// Melee attack if at all possible (the player gets in our personal space).
			float flEnemyDistance = EnemyDistance( GetEnemy() );
			float flEnemyDot = GetAbsOrigin().Dot( GetEnemy()->GetAbsOrigin() );

			// NOTE: Experiementing with conditions here.
			if ( MeleeAttack1Conditions( flEnemyDot, flEnemyDistance ) == COND_CAN_MELEE_ATTACK1 )
			{
				baseSched = SCHED_MELEE_ATTACK1;
				SetNextAttack( gpGlobals->curtime + 3.0f );
				return baseSched;
			}

			if ( GetEnemy() && m_flNextAttack < gpGlobals->curtime )
			{
				switch ( m_AntKingAttackState )
				{
					case ANTKING_ATTACK_SPIT:
					{	
						baseSched = SCHED_ANTLIONKING_SPIT;
						break;
					}
					case ANTKING_ATTACK_PUKE:
					case ANTKING_ATTACK_PUKE_MAPTRIGGERS:
					{
						return SCHED_ANTLIONKING_PUKE;
					}
					case ANTKING_ATTACK_SHOCKWAVE:
					{
						return SCHED_ANTLIONKING_SHOCKWAVE;
					}
					case ANTKING_ATTACK_NONE:
					default:
					{
						baseSched = SCHED_IDLE_STAND;
						break;
					}
				}
			}
			/*
			// If we cannot attack but we still have an enemy, we MUST idle
			// We cannot do SCHED_COMBAT_FACE as we are not allowed to turn without input from the map.
			else if ( GetEnemy() )
			{
				baseSched = SCHED_IDLE_STAND;
			}*/
		}
		break;

	default:
		baseSched = BaseClass::SelectSchedule();
		break;
	}

	// We can get stuck here, so make sure we un-stick ourselves.
	if ( baseSched == SCHED_PRE_FAIL_ESTABLISH_LINE_OF_FIRE || baseSched == SCHED_ESTABLISH_LINE_OF_FIRE )
	{
		//DevWarning( "WE'RE STUCK! TIME TO UNSTUCK!\n" );
		if ( m_AntKingAttackState >= ANTKING_ATTACK_SPIT )
			return SCHED_ANTLIONKING_SPIT;
		else
			return SCHED_IDLE_STAND;
	}
	
	if ( GetCurSchedule() && g_debug_antlionking.GetBool() )
	{
		DevMsg( "AntKing Schedule: %s\n", GetCurSchedule()->GetName() );
	}
	return baseSched;
}

//---------------------------------------------------------
// Purpose: Task Handling
//---------------------------------------------------------
void CNPC_AntlionKing::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANTLIONKING_PUKE_ATTACK:
		ResetIdealActivity( (Activity)ACT_PUKE );
		break;

	case TASK_ANTLIONKING_SHOCKWAVE_ATTACK:
		ResetIdealActivity( (Activity)ACT_STOMP );
		break;

	case TASK_ANTLIONKING_EXPOSE_WEAKPOINT:
		ResetIdealActivity( (Activity)ACT_WEAKPOINT_START ); 
		break;

	case TASK_ANTLIONKING_WEAKPOINT_LOOP:
		ResetIdealActivity( (Activity)ACT_WEAKPOINT_LOOP );
		break;

	case TASK_ANTLIONKING_END_WEAKPOINT_LOOP:
		ResetIdealActivity( (Activity)ACT_WEAKPOINT_END );
		break;

	case TASK_FACE_ENEMY:
		if ( !m_bIsSteeringActive )
		{
			TaskComplete();
			return;
		}
		else
			BaseClass::StartTask( pTask );

	case TASK_ANTLIONKING_DRAW_PUKE_DEBUG_LINES:
		if ( GetEnemy() )
		{
			Vector forwardVector;
			Vector vecMouthPos;
			GetVectors( &forwardVector, NULL, NULL );

			vecMouthPos = GetAbsOrigin() + ( forwardVector * 250 );
			vecMouthPos += Vector( 0, 0, 800 );

			// Try to lead the enemy's position.
			Vector vecPukePos;
			UTIL_PredictedPosition( GetEnemy(), 0.5f, &vecPukePos );
			vecPukePos.z = GetEnemy()->GetAbsOrigin().z;

			// Cache off the puke position.
			m_vecPukePos = vecPukePos;

			//NDebugOverlay::VertArrow( vecMouthPos, vecPukePos, 20.0f, 0, 255, 0, 255, false, sk_antlionking_pukezone_lifetime.GetFloat() );
		}
		
		TaskComplete();
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}

//---------------------------------------------------------
// Purpose: Task Handling
//---------------------------------------------------------
void CNPC_AntlionKing::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANTLIONKING_PUKE_ATTACK:
		// Nothing to do here
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		return;

	case TASK_ANTLIONKING_SHOCKWAVE_ATTACK:
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		return;

	case TASK_ANTLIONKING_EXPOSE_WEAKPOINT:
		if ( IsActivityFinished() ) 
		{
			m_IsWeakpointExposed = true;

			// Fire off the output for map events.
			m_OnStunnedOutput.FireOutput( this, this );

			TaskComplete();
		}
		return;

	case TASK_ANTLIONKING_WEAKPOINT_LOOP:
		if ( IsActivityFinished() )
			TaskComplete();
		break;

	case TASK_ANTLIONKING_END_WEAKPOINT_LOOP:
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		return;

	default:
		BaseClass::RunTask( pTask );
	}
}

//---------------------------------------------------------
// Purpose: Idle Sound
//---------------------------------------------------------
void CNPC_AntlionKing::IdleSound( void )
{
	EmitSound( "NPC_AntlionKing.Idle" );
	m_flNextIdleSoundTime = gpGlobals->curtime + random->RandomFloat( 2.0f, 5.0f );
}

//---------------------------------------------------------
// Purpose: The King can turn if enabled by entity i/o.
//---------------------------------------------------------
float CNPC_AntlionKing::MaxYawSpeed( void )
{
	Activity eActivity = GetActivity();

	if ( !m_bIsSteeringActive )
		return 0.0f;
	else
	{
		if ( eActivity == ACT_TURN_LEFT || eActivity == ACT_TURN_RIGHT )
			return 40.0f;

		return 0.0f;
	}
}

//---------------------------------------------------------
// Purpose: Implements the spit attack.
//---------------------------------------------------------
void CNPC_AntlionKing::SpitAttack( void )
{
	if ( !GetEnemy() )
		return;

	Vector spitVector;
	Vector vecSpitAttachment, vecEnemyPosition;
	
	//GetAttachment( "SPITORIGIN", vecSpitAttachment, NULL );
	{
		Vector forwardVector;
		GetVectors( &forwardVector, NULL, NULL );

		vecSpitAttachment = GetAbsOrigin() + ( forwardVector * 600 );
		vecSpitAttachment += Vector( 0, 0, 1000 );
	}
	vecEnemyPosition = GetEnemy()->BodyTarget( vecSpitAttachment, true );

	// Add some slight noise to our target vector.
	vecEnemyPosition.x = random->RandomFloat( vecEnemyPosition.x - 10, vecEnemyPosition.x + 10 );
	vecEnemyPosition.z = random->RandomFloat( vecEnemyPosition.z - 10, vecEnemyPosition.z + 10 );

	Vector vecFlingVelocity;

	// Construct spit vector (borrowed from worker spit fling code)
	{
		float flSpeed	= 800.0f; // Yes, this is a magic number, and yes, it's worked this long, don't break it!
		float flGravity = GetCurrentGravity();

		vecFlingVelocity = ( vecEnemyPosition - vecSpitAttachment );

		// throw at a constant time
		float throwTime = vecFlingVelocity.Length() / flSpeed;
		vecFlingVelocity = vecFlingVelocity * ( 1.0 / throwTime );


		// adjust upward toss to compensate for gravity loss
		vecFlingVelocity.z += flGravity * throwTime * 0.5;

		Vector vecApex = vecSpitAttachment + ( vecEnemyPosition - vecSpitAttachment ) * 0.5;
		vecApex.z += 0.5 * flGravity * ( throwTime * 0.5 ) * ( throwTime * 0.5 );

		if ( g_debug_antlionking.GetInt() == 2 )
		{
			NDebugOverlay::Line( vecSpitAttachment, vecApex, 255, 0, 0, true, 5.0f );
			NDebugOverlay::Line( vecApex, vecEnemyPosition, 0, 255, 0, true, 5.0f );
		}
	}

	int maxSpitballs = 6 - m_NumTimesStunned;	// NOTE: We should throw less as we attack faster after taking more damage.
	int numSpitballs = random->RandomInt( 3, maxSpitballs );

	for ( int i = 0; i < numSpitballs; i++ )
	{
		CAntKingSpitBomb *pSpitBomb = (CAntKingSpitBomb *)CreateEntityByName( "antking_spitbomb" );
		if ( pSpitBomb )
		{
			// Spread each spitball out.
			Vector vecSpitOrigin = vecSpitAttachment;
			vecSpitOrigin.x += ( random->RandomFloat( 10.0f, 20.0f ) * i );
			vecSpitOrigin.z += ( random->RandomFloat( 10.0f, 20.0f ) * i );

			if ( i > 3 )
			{
				vecSpitOrigin.y += ( random->RandomFloat( 10.0f, 20.0f ) * i );
			}
			else
			{
				vecSpitOrigin.y -= ( random->RandomFloat( 10.0f, 20.0f ) * i );
			}

			pSpitBomb->SetAbsOrigin( vecSpitOrigin );
			pSpitBomb->SetModel( kAntlionKing_SpitballModel_Medium );
			pSpitBomb->m_iSize = 1;
			DispatchSpawn( pSpitBomb );
			pSpitBomb->SetOwnerEntity( this ); // Make sure our spit doesn't collide with us.
			pSpitBomb->SetAbsVelocity( vecFlingVelocity );

			if ( i == 0 && numSpitballs > 3 )
			{
				pSpitBomb->SetModel( kAntlionKing_SpitballModel_Large );
				pSpitBomb->m_iSize = 2;
			}
			else if ( i % 2 == 0 )
			{
				pSpitBomb->SetModel( kAntlionKing_SpitballModel_Small );
				pSpitBomb->m_iSize = 0;
			}

			pSpitBomb->CreateParticles();

			// Tumble through the air.
			pSpitBomb->SetLocalAngularVelocity(
					QAngle(random->RandomFloat(-250, -500),
					random->RandomFloat(-250, -500) * (2 * i),
					random->RandomFloat(-250, -500)));
		}
	}

	// Spit sound!
	//EmitSound( "NPC_AntlionKing.Spit" );

	m_flNextAttack = gpGlobals->curtime + m_flAttackRate; // We attack faster when we've been stunned more.
}

//---------------------------------------------------------
// Purpose: Implements the puke attack
//---------------------------------------------------------
void CNPC_AntlionKing::PukeAttack( void )
{
	Vector vecEnemyDir, vecRight;

	GetVectors( NULL, &vecRight, NULL );
	vecEnemyDir = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
	VectorNormalize( vecEnemyDir );

	vecEnemyDir.z = 0;
	vecRight.z = 0;

	CAntKingPukeZone *pPukeZone = (CAntKingPukeZone *)CreateEntityByName( "trigger_antking_pukezone" );
	pPukeZone->SetAbsOrigin( m_vecPukePos );
	
	// Pull the zone out of the ground a little so the particles are not stuck under the world.
	pPukeZone->SetAbsOrigin( Vector( pPukeZone->GetAbsOrigin().x, pPukeZone->GetAbsOrigin().y, pPukeZone->GetAbsOrigin().z + 64 ) );
	
	pPukeZone->SetOwnerEntity( this );
	DispatchSpawn( pPukeZone );

	UTIL_ScreenShake( m_vecPukePos, 4, 100, 2.5, 100000, SHAKE_START, true );

	// Puke sound!
	//EmitSound( "NPC_AntlionKing.Puke" );

	// Clear out stored puke pos.
	m_vecPrevOrigin = vec3_invalid;

	// Dispatch puke particles.
	Vector vecMouthPos, forwardVector;
	GetVectors( &forwardVector, NULL, NULL );
	vecMouthPos = GetAbsOrigin() + ( forwardVector * 600 );
	vecMouthPos += Vector( 0, 0, 1000 );

	DispatchParticleEffect( "antking_pukespray", PATTACH_POINT_FOLLOW, this, "att.mouth" );

	CPASAttenuationFilter filter( this, "NPC_AntlionKing.Acidloop" );
	EmitSound( filter, pPukeZone->entindex(), "NPC_AntlionKing.Acidloop", &pPukeZone->GetAbsOrigin() );
	EmitSound( filter, pPukeZone->entindex(), "NPC_AntlionKing.PukeStart", &pPukeZone->GetAbsOrigin() );

	m_flLastPukeTime = gpGlobals->curtime;
}

//---------------------------------------------------------
// Purpose: Implements the shockwave attack
//---------------------------------------------------------
void CNPC_AntlionKing::ShockwaveAttack( void )
{
	if ( !GetEnemy() )
	{
		// We need an enemy to attack for this to work.
		return;
	}

	// Create our new shockwave volume and spawn it into the world.
	// Create 5 and rotate them into a semi-circle.
	for ( int iShockwave = 0; iShockwave < 5; iShockwave++ )
	{
		CAntKingShockwaveVolume* pShockwaveVolume = (CAntKingShockwaveVolume*)CreateEntityByName("trigger_antking_shockwave_volume");
		if ( !pShockwaveVolume )
		{
			// This should never happen, but if this fails, we need to not crash.
			DevWarning( "King failed to create shockwave attack volume, we may need to reduce edicts.\n" );
			return;
		}

		Vector vecForward, vecRight, vecUp;
		GetVectors( &vecForward, &vecRight, &vecUp );
		VectorNormalize( vecForward );
		VectorNormalize( vecRight );
		VectorNormalize( vecUp );

		Vector vecShockwave = GetAbsOrigin();
		QAngle angShockwave = GetAbsAngles();

		vecShockwave.z -= 72.0f;

		// Push us infront of the King's origin.
		if ( iShockwave == 0 )
			VectorMA( vecShockwave, 370.0f, vecForward, vecShockwave );

		if ( iShockwave > 0 && iShockwave < 3 )
		{
			VectorMA( vecShockwave, 175.0f, vecForward, vecShockwave );
			VectorMA( vecShockwave, 460.0f, ( iShockwave == 1 ) ? vecRight : -vecRight, vecShockwave );

			if ( iShockwave == 1 )
			{
				angShockwave.y -= 45.0f;
			}
			else
			{
				angShockwave.y += 45.0f;
			}
		}
		else if ( iShockwave > 2 && iShockwave < 5 )
		{
			VectorMA( vecShockwave, -210.0f, vecForward, vecShockwave );
			VectorMA( vecShockwave, 680.0f, ( iShockwave == 3 ) ? vecRight : -vecRight, vecShockwave );

			if ( iShockwave == 3 )
			{
				angShockwave.y -= 75.0f;
			}
			else
			{
				angShockwave.y += 75.0f;
			}
		}

		// BUGBUG: This won't work unless the King is rotated either (0,0,0) or (0,180,0)
		// LEFTOFF: Figure this out.
		static Vector vecShockBounds[5] =
		{
			{34, 280, 28},	// Center mins
			{34, 280, 28},	// Center maxs
			{256, 239, 23},	// Left 1 mins
			{256, 239, 23},	// Left 1 maxs
			{34, 280, 28},
		};

		// Define the bounds of this volume before rotating.
		//pShockwaveVolume->SetSize( -vecShockBounds[(iShockwave * 2)], vecShockBounds[(iShockwave * 2) + 1] );
		pShockwaveVolume->SetSize( -vecShockBounds[iShockwave], vecShockBounds[iShockwave] );

		pShockwaveVolume->SetAbsOrigin( vecShockwave );
		pShockwaveVolume->SetAbsAngles( angShockwave );	// NOTE: This rotates the entity so the particle fx rotate, however it does NOT rotate the BOUNDS of the trigger.
		DispatchSpawn( pShockwaveVolume );
	
		pShockwaveVolume->ShockwaveInit( vecForward );

		if (iShockwave == 0)
		{
			CPASAttenuationFilter filter( this, "NPC_AntlionKing.StompShockwave" );
			EmitSound( filter, pShockwaveVolume->entindex(), "NPC_AntlionKing.StompShockwave", &pShockwaveVolume->GetAbsOrigin() );
		}
	}

	/*
	// For now, we're just gonna fire a hard-coded map event.
	// This is not very good and I will not allow this to ship, but is fast for prototyping.
	CBaseEntity* pShockwaveTemplate = FindNamedEntity("antking_shockwave_template");
	if ( !pShockwaveTemplate )
	{
		DevWarning( "Failed to find necessary point_template to spawn shockwave attack!" );
		return;
	}

	inputdata_t spawnInput;
	spawnInput.pActivator = this;
	spawnInput.pCaller = this;

	// Just create the shockwave, map logic will handle the rest.
	CPointTemplate *pTypedTemplate = dynamic_cast<CPointTemplate *>( pShockwaveTemplate );
	if ( pTypedTemplate )
	{
		pTypedTemplate->InputForceSpawn( spawnInput );
	}*/

	// Shock sound!
	//EmitSound( "NPC_AntlionKing.Slam" );
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_antlionking, CNPC_AntlionKing )

	DECLARE_ANIMEVENT( AE_KING_SPIT_ATTACK )
	DECLARE_ANIMEVENT( AE_KING_PUKE_ATTACK )
	DECLARE_ANIMEVENT( AE_KING_SHOCKWAVE_ATTACK )
	DECLARE_ANIMEVENT( AE_KING_SWIPE_ATTACK )
	DECLARE_ANIMEVENT( AE_KING_STEP_FRONTRIGHT )
	DECLARE_ANIMEVENT( AE_KING_STEP_BACKLEFT )
	DECLARE_ANIMEVENT( AE_KING_STEP_FRONTLEFT )
	DECLARE_ANIMEVENT( AE_KING_STEP_BACKRIGHT )

	DECLARE_ACTIVITY( ACT_SPIT )
	DECLARE_ACTIVITY( ACT_PUKE )
	DECLARE_ACTIVITY( ACT_PAIN )
	DECLARE_ACTIVITY( ACT_STOMP )
	DECLARE_ACTIVITY( ACT_SWIPE )
	DECLARE_ACTIVITY( ACT_WEAKPOINT_START )
	DECLARE_ACTIVITY( ACT_WEAKPOINT_LOOP )
	DECLARE_ACTIVITY( ACT_WEAKPOINT_END )

	DECLARE_TASK( TASK_ANTLIONKING_PUKE_ATTACK )
	DECLARE_TASK( TASK_ANTLIONKING_SHOCKWAVE_ATTACK )
	DECLARE_TASK( TASK_ANTLIONKING_EXPOSE_WEAKPOINT )
	DECLARE_TASK( TASK_ANTLIONKING_WEAKPOINT_LOOP )
	DECLARE_TASK( TASK_ANTLIONKING_END_WEAKPOINT_LOOP )

	DECLARE_TASK( TASK_ANTLIONKING_DRAW_PUKE_DEBUG_LINES )

	// Unused for now
	//DECLARE_CONDITION( COND_ANTLIONKING_HIT_BY_BOMB )

	//Schedules

	//==================================================
	// Spit Attack
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONKING_SPIT,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_FACE_ENEMY						0"
		"		TASK_RANGE_ATTACK1					0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Puke Attack
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONKING_PUKE,

		"	Tasks"
		"		TASK_STOP_MOVING								0"
		"		TASK_FACE_ENEMY									0"
		"		TASK_ANTLIONKING_DRAW_PUKE_DEBUG_LINES			0"
		"		TASK_WAIT										2"
		"		TASK_ANTLIONKING_PUKE_ATTACK					0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)
		
	//==================================================
	// Stomp/Shockwave Attack
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONKING_SHOCKWAVE,

		"	Tasks"
		"		TASK_STOP_MOVING								0"
		"		TASK_FACE_ENEMY									0"
		"		TASK_ANTLIONKING_SHOCKWAVE_ATTACK				0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Expose Weakpoint
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONKING_EXPOSE_WEAKPOINT,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_ANTLIONKING_EXPOSE_WEAKPOINT	0"
		""
		"	Interrupts"
	)

	//==================================================
	// Loop Weakpoint
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONKING_WEAKPOINT_LOOP,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_ANTLIONKING_WEAKPOINT_LOOP		0"
		""
		"	Interrupts"
	)

	//==================================================
	// End Weakpoint
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONKING_WEAKPOINT_END,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_ANTLIONKING_END_WEAKPOINT_LOOP 0"
		""
		"	Interrupts"
	)


AI_END_CUSTOM_NPC()