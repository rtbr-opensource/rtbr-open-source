//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasebasebludgeon.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "animation.h"
#include "rumble_shared.h"
#include "gamestats.h"

#if defined( CLIENT_DLL )
	#include "c_hl2mp_player.h"
	#include "takedamageinfo.h"
	#ifdef MAPBASE_MP
	#include "c_ai_basenpc.h"
	#endif
#else
	#include "hl2mp_player.h"
	#include "ndebugoverlay.h"
	#include "te_effect_dispatch.h"
	#include "ilagcompensationmanager.h"
	#ifdef MAPBASE
	#include "ai_basenpc.h"
	#endif
	#include "soundent.h"
#ifdef RTBMP
	#include "rtbmp_player.h"
#endif
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( BaseHL2MPBludgeonWeapon, DT_BaseHL2MPBludgeonWeapon )

BEGIN_NETWORK_TABLE( CBaseHL2MPBludgeonWeapon, DT_BaseHL2MPBludgeonWeapon )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CBaseHL2MPBludgeonWeapon )
END_PREDICTION_DATA()

#define BLUDGEON_HULL_DIM		16

static const Vector g_bludgeonMins(-BLUDGEON_HULL_DIM,-BLUDGEON_HULL_DIM,-BLUDGEON_HULL_DIM);
static const Vector g_bludgeonMaxs(BLUDGEON_HULL_DIM,BLUDGEON_HULL_DIM,BLUDGEON_HULL_DIM);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CBaseHL2MPBludgeonWeapon::CBaseHL2MPBludgeonWeapon()
{
	m_bFiresUnderwater = true;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the weapon
//-----------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::Spawn( void )
{
	m_fMinRange1	= 0;
	m_fMinRange2	= 0;
	m_fMaxRange1	= 64;
	m_fMaxRange2	= 64;
	//Call base class first
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache the weapon
//-----------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::Precache( void )
{
	//Call base class first
	BaseClass::Precache();
}

#if defined(MAPBASE) && !defined(CLIENT_DLL)
int CBaseHL2MPBludgeonWeapon::CapabilitiesGet()
{ 
	return bits_CAP_WEAPON_MELEE_ATTACK1; 
}


int CBaseHL2MPBludgeonWeapon::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	if (flDist > 64)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}
#endif

//------------------------------------------------------------------------------
// Purpose : Update weapon
//------------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if (pOwner->m_nButtons & IN_RELOAD) {
		if (GetActivity() == ACT_VM_IDLE && m_flNextFidgetReload < gpGlobals->curtime) {
			SendWeaponAnim( ACT_VM_FIDGET );
			m_flNextFidgetReload = gpGlobals->curtime + 2.0f;
		}
	}

#ifdef MAPBASE
	if (pOwner->HasSpawnFlags( SF_PLAYER_SUPPRESS_FIRING ))
	{
		m_bShotDelayed = false;
		WeaponIdle();
		return;
	}

	// See if we need to fire off our secondary round
	if (m_bShotDelayed)
	{
		if (gpGlobals->curtime > m_flDelayedFire)
			DelayedAttack();
	}
	else
#endif
	if ( (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		PrimaryAttack();
	} 
	else if ( (pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime) )
	{
		SecondaryAttack();
	}
	else 
	{
		WeaponIdle();
		return;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::PrimaryAttack()
{
	
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif
	Swing( false );
#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );

	pPlayer->OnMyWeaponFired( this );
#endif
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::SecondaryAttack()
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif
	Swing( true );
#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::PrimaryAttack( int iDamage, int iDamageType )
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif
	Swing( false, iDamage, iDamageType );
#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::SecondaryAttack( int iDamage, int iDamageType )
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif
	Swing( true, iDamage, iDamageType );
#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

//------------------------------------------------------------------------------
// Purpose: Implement impact function
//------------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::Hit( trace_t &traceHit, Activity nHitActivity, bool bIsSecondary )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	//Do view kick
//	AddViewKick();

#if defined(MAPBASE_MP) && !defined(CLIENT_DLL)
	//Make sound for the AI
	CSoundEnt::InsertSound( SOUND_BULLET_IMPACT, traceHit.endpos, 400, 0.2f, pPlayer );
#endif

	CBaseEntity	*pHitEntity = traceHit.m_pEnt;

	//Apply damage to a hit target
	if ( pHitEntity != NULL )
	{
		Vector hitDirection;
		pPlayer->EyeVectors( &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );

#ifndef CLIENT_DLL
#ifdef MAPBASE
		CTakeDamageInfo info( GetOwner(), GetOwner(), GetDamageForActivity( nHitActivity ), GetDamageType() );
#else
		CTakeDamageInfo info( GetOwner(), GetOwner(), GetDamageForActivity( nHitActivity ), DMG_CLUB );
#endif

		if( pPlayer && pHitEntity->IsNPC() )
		{
			// If bonking an NPC, adjust damage.
			info.AdjustPlayerDamageInflictedForSkillLevel();
		}

		CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos );

		pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit ); 
		ApplyMultiDamage();

		// Now hit all triggers along the ray that... 
		TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );
#endif

		if ( this->PlayFleshyHittySoundOnHit() )
			WeaponSound( MELEE_HIT );

#ifdef MAPBASE_MP
		// Don't do impact effect if this is an ally
		if ( pHitEntity->MyNPCPointer() && pHitEntity->MyNPCPointer()->IsPlayerAlly( pPlayer ) )
			return;
#endif
	}

	// Apply an impact effect
	ImpactEffect( traceHit );
}

void CBaseHL2MPBludgeonWeapon::Hit( trace_t &traceHit, Activity nHitActivity, bool bIsSecondary, int iDamage, int iDamageType )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	//Do view kick
	AddViewKick();
#ifndef CLIENT_DLL
	//Make sound for the AI
	CSoundEnt::InsertSound( SOUND_BULLET_IMPACT, traceHit.endpos, 400, 0.2f, pPlayer );
#endif
#ifndef CLIENT_DLL
	// This isn't great, but it's something for when the crowbar hits.
	pPlayer->RumbleEffect( RUMBLE_AR2, 0, RUMBLE_FLAG_RESTART );
#endif
	CBaseEntity *pHitEntity = traceHit.m_pEnt;
	//Apply damage to a hit target
	if (pHitEntity != NULL)
	{
		Vector hitDirection;
		pPlayer->EyeVectors( &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );
#ifndef CLIENT_DLL
		CTakeDamageInfo info( GetOwner(), GetOwner(), iDamage, iDamageType );
		if (pPlayer && pHitEntity->IsNPC())
		{
			// If bonking an NPC, adjust damage.
			info.AdjustPlayerDamageInflictedForSkillLevel();
		}
		CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos );
		pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit );
		ApplyMultiDamage();
		// Now hit all triggers along the ray that... 
		TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );
		if (ToBaseCombatCharacter( pHitEntity ))
		{
			gamestats->Event_WeaponHit( pPlayer, !bIsSecondary, GetClassname(), info );
		}
#endif
		WeaponSound( MELEE_HIT );
	}
	// Apply an impact effect
	ImpactEffect( traceHit );
}

Activity CBaseHL2MPBludgeonWeapon::ChooseIntersectionPointAndActivity( trace_t &hitTrace, const Vector &mins, const Vector &maxs, CBasePlayer *pOwner )
{
	int			i, j, k;
	float		distance;
	const float	*minmaxs[2] = {mins.Base(), maxs.Base()};
	trace_t		tmpTrace;
	Vector		vecHullEnd = hitTrace.endpos;
	Vector		vecEnd;

	distance = 1e6f;
	Vector vecSrc = hitTrace.startpos;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace );
	if ( tmpTrace.fraction == 1.0 )
	{
		for ( i = 0; i < 2; i++ )
		{
			for ( j = 0; j < 2; j++ )
			{
				for ( k = 0; k < 2; k++ )
				{
					vecEnd.x = vecHullEnd.x + minmaxs[i][0];
					vecEnd.y = vecHullEnd.y + minmaxs[j][1];
					vecEnd.z = vecHullEnd.z + minmaxs[k][2];

					UTIL_TraceLine( vecSrc, vecEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace );
					if ( tmpTrace.fraction < 1.0 )
					{
						float thisDistance = (tmpTrace.endpos - vecSrc).Length();
						if ( thisDistance < distance )
						{
							hitTrace = tmpTrace;
							distance = thisDistance;
						}
					}
				}
			}
		}
	}
	else
	{
		hitTrace = tmpTrace;
	}


	return ACT_VM_HITCENTER;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &traceHit - 
//-----------------------------------------------------------------------------
bool CBaseHL2MPBludgeonWeapon::ImpactWater( const Vector &start, const Vector &end )
{
	//FIXME: This doesn't handle the case of trying to splash while being underwater, but that's not going to look good
	//		 right now anyway...
	
	// We must start outside the water
	if ( UTIL_PointContents( start ) & (CONTENTS_WATER|CONTENTS_SLIME))
		return false;

	// We must end inside of water
	if ( !(UTIL_PointContents( end ) & (CONTENTS_WATER|CONTENTS_SLIME)))
		return false;

	trace_t	waterTrace;

	UTIL_TraceLine( start, end, (CONTENTS_WATER|CONTENTS_SLIME), GetOwner(), COLLISION_GROUP_NONE, &waterTrace );

	if ( waterTrace.fraction < 1.0f )
	{
#ifndef CLIENT_DLL
		CEffectData	data;

		data.m_fFlags  = 0;
		data.m_vOrigin = waterTrace.endpos;
		data.m_vNormal = waterTrace.plane.normal;
		data.m_flScale = 8.0f;

		// See if we hit slime
		if ( waterTrace.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		DispatchEffect( "watersplash", data );			
#endif
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::ImpactEffect( trace_t &traceHit )
{
	// See if we hit water (we don't do the other impact effects in this case)
	if ( ImpactWater( traceHit.startpos, traceHit.endpos ) )
		return;

	//FIXME: need new decals
	UTIL_ImpactTrace( &traceHit, DMG_CLUB );
}


//------------------------------------------------------------------------------
// Purpose : Starts the swing of the weapon and determines the animation
// Input   : bIsSecondary - is this a secondary attack?
//------------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::Swing( int bIsSecondary )
{
	trace_t traceHit;

	// Try a ray
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	Vector swingStart = pOwner->Weapon_ShootPosition( );
	Vector forward;

	pOwner->EyeVectors( &forward, NULL, NULL );

	Vector swingEnd = swingStart + forward * GetRange();
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );
	Activity nHitActivity = ACT_VM_HITCENTER;

#ifndef CLIENT_DLL
	// Like bullets, bludgeon traces have to trace against triggers.
#ifdef MAPBASE
	CTakeDamageInfo triggerInfo( GetOwner(), GetOwner(), GetDamageForActivity( nHitActivity ), GetDamageType() );
#else
	CTakeDamageInfo triggerInfo( GetOwner(), GetOwner(), GetDamageForActivity( nHitActivity ), DMG_CLUB );
#endif
	TraceAttackToTriggers( triggerInfo, traceHit.startpos, traceHit.endpos, vec3_origin );

	if ( traceHit.fraction == 1.0 )
	{
		float bludgeonHullRadius = 1.732f * BLUDGEON_HULL_DIM;  // hull is +/- 16, so use cuberoot of 2 to determine how big the hull is from center to the corner point

		// Back off by hull "radius"
		swingEnd -= forward * bludgeonHullRadius;

		UTIL_TraceHull( swingStart, swingEnd, g_bludgeonMins, g_bludgeonMaxs, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );
		if ( traceHit.fraction < 1.0 && traceHit.m_pEnt )
		{
			Vector vecToTarget = traceHit.m_pEnt->GetAbsOrigin() - swingStart;
			VectorNormalize( vecToTarget );

			float dot = vecToTarget.Dot( forward );

			// YWB:  Make sure they are sort of facing the guy at least...
			if ( dot < 0.70721f )
			{
				// Force amiss
				traceHit.fraction = 1.0f;
			}
			else
			{
				nHitActivity = ChooseIntersectionPointAndActivity( traceHit, g_bludgeonMins, g_bludgeonMaxs, pOwner );
			}
		}
	}
#endif

#ifndef MAPBASE
	WeaponSound( SINGLE );
#endif

	// -------------------------
	//	Miss
	// -------------------------
	if ( traceHit.fraction == 1.0f )
	{
		nHitActivity = bIsSecondary ? ACT_VM_MISSCENTER2 : ACT_VM_MISSCENTER;

#ifdef MAPBASE
		//Play swing sound
		WeaponSound( SINGLE );
#else
		// We want to test the first swing again
		Vector testEnd = swingStart + forward * GetRange();
		
		// See if we happened to hit water
		ImpactWater( swingStart, testEnd );
#endif
	}
#ifndef MAPBASE
	else
	{
		Hit( traceHit, nHitActivity, bIsSecondary );
	}
#endif

	// Send the anim
	SendWeaponAnim( nHitActivity );

	pOwner->SetAnimation( PLAYER_ATTACK1 );

	//Setup our next attack times
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();

#if !defined(CLIENT_DLL) && defined(RTBMP)
	CRTBMP_Player *pMPPlayer = ToRTBMPPlayer( pOwner );
	if (pMPPlayer)
		pMPPlayer->DisableSpawnProtection();
#endif

#ifdef MAPBASE
	if (GetHitDelay() > 0.f)
	{
		//Play swing sound
		WeaponSound(SINGLE);

		m_flDelayedFire = gpGlobals->curtime + GetHitDelay();
		m_bShotDelayed = true;
	}
	else
	{
		if (traceHit.fraction == 1.0f)
		{
			// We want to test the first swing again
			Vector testEnd = swingStart + forward * GetRange();

			//Play swing sound
			WeaponSound(SINGLE);

			// See if we happened to hit water
			ImpactWater(swingStart, testEnd);
		}
		else
		{
			// Other melee sounds
			if (traceHit.m_pEnt && traceHit.m_pEnt->IsWorld())
				WeaponSound(MELEE_HIT_WORLD);
#ifndef CLIENT_DLL
			else if (traceHit.m_pEnt && !traceHit.m_pEnt->PassesDamageFilter(triggerInfo))
				WeaponSound(MELEE_MISS);
#endif
			else
				WeaponSound(MELEE_HIT);

			Hit( traceHit, nHitActivity, bIsSecondary );
		}
	}
#endif
}

void CBaseHL2MPBludgeonWeapon::Swing( int bIsSecondary, int iDamage, int iDamageType )
{
	trace_t traceHit;
	// Try a ray
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
		return;
#ifndef CLIENT_DLL
	pOwner->RumbleEffect( RUMBLE_CROWBAR_SWING, 0, RUMBLE_FLAG_RESTART );
#endif
	Vector swingStart = pOwner->Weapon_ShootPosition();
	Vector forward;
	pOwner->EyeVectors( &forward, NULL, NULL );
	Vector swingEnd = swingStart + forward * GetRange();
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );
	Activity nHitActivity = ACT_VM_HITCENTER;
#ifndef CLIENT_DLL
	// Like bullets, bludgeon traces have to trace against triggers.
	CTakeDamageInfo triggerInfo( GetOwner(), GetOwner(), GetDamageForActivity( nHitActivity ), iDamageType );
	triggerInfo.SetDamagePosition( traceHit.startpos );
	triggerInfo.SetDamageForce( forward );
	TraceAttackToTriggers( triggerInfo, traceHit.startpos, traceHit.endpos, forward );
#endif
	if (traceHit.fraction == 1.0)
	{
		float bludgeonHullRadius = 1.732f * BLUDGEON_HULL_DIM;  // hull is +/- 16, so use cuberoot of 2 to determine how big the hull is from center to the corner point
		// Back off by hull "radius"
		swingEnd -= forward * bludgeonHullRadius;
		UTIL_TraceHull( swingStart, swingEnd, g_bludgeonMins, g_bludgeonMaxs, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );
		if (traceHit.fraction < 1.0 && traceHit.m_pEnt)
		{
			Vector vecToTarget = traceHit.m_pEnt->GetAbsOrigin() - swingStart;
			VectorNormalize( vecToTarget );
			float dot = vecToTarget.Dot( forward );
			// YWB:  Make sure they are sort of facing the guy at least...
			if (dot < 0.70721f)
			{
				// Force amiss
				traceHit.fraction = 1.0f;
			}
			else
			{
				nHitActivity = ChooseIntersectionPointAndActivity( traceHit, g_bludgeonMins, g_bludgeonMaxs, pOwner );
			}
		}
	}
#ifndef CLIENT_DLL
	gamestats->Event_WeaponFired( pOwner, !bIsSecondary, GetClassname() );
#endif
	WeaponSound( SINGLE );
	// -------------------------
	//	Miss
	// -------------------------
	if (traceHit.fraction == 1.0f)
	{
		nHitActivity = bIsSecondary ? ACT_VM_MISSCENTER2 : ACT_VM_MISSCENTER;
		// We want to test the first swing again
		Vector testEnd = swingStart + forward * GetRange();
#ifdef MAPBASE
		// Sound has been moved here since we're using the other melee sounds now
		WeaponSound( SINGLE );
#endif
		// See if we happened to hit water
		ImpactWater( swingStart, testEnd );
	}
	else
	{
#ifndef MAPBASE // currently broken...?
		CTakeDamageInfo triggerInfo( GetOwner(), GetOwner(), GetDamageForActivity( GetActivity() ), GetDamageType() );
		triggerInfo.SetDamagePosition( traceHit.startpos );
		triggerInfo.SetDamageForce( forward );

		// Other melee sounds
		if (traceHit.m_pEnt && traceHit.m_pEnt->IsWorld())
			WeaponSound( MELEE_HIT_WORLD );
		else if (traceHit.m_pEnt && !traceHit.m_pEnt->PassesDamageFilter( triggerInfo ))
			WeaponSound( MELEE_MISS );
		else
			WeaponSound( MELEE_HIT );
#endif
		if (iDamage != NULL) {
			Hit( traceHit, nHitActivity, bIsSecondary ? true : false, iDamage, iDamageType );
		}
		else {
			Hit( traceHit, nHitActivity, bIsSecondary ? true : false );
		}
	}
	// Send the anim
	SendWeaponAnim( nHitActivity );
	//Setup our next attack times
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
#if !defined(CLIENT_DLL) && defined(RTBMP)
	CRTBMP_Player *pMPPlayer = ToRTBMPPlayer( pOwner );
	if (pMPPlayer)
		pMPPlayer->DisableSpawnProtection();
#endif
}

#ifdef MAPBASE
void CBaseHL2MPBludgeonWeapon::DelayedAttack(void)
{
	m_bShotDelayed = false;

	trace_t traceHit;

	// Try a ray
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	Vector swingStart = pOwner->Weapon_ShootPosition();
	Vector forward;

	pOwner->EyeVectors( &forward, NULL, NULL );

	Vector swingEnd = swingStart + forward * GetRange();
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );

	if (traceHit.fraction == 1.0)
	{
		float bludgeonHullRadius = 1.732f * BLUDGEON_HULL_DIM;  // hull is +/- 16, so use cuberoot of 2 to determine how big the hull is from center to the corner point

		// Back off by hull "radius"
		swingEnd -= forward * bludgeonHullRadius;

		UTIL_TraceHull(swingStart, swingEnd, g_bludgeonMins, g_bludgeonMaxs, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit);
		if (traceHit.fraction < 1.0 && traceHit.m_pEnt)
		{
			Vector vecToTarget = traceHit.m_pEnt->GetAbsOrigin() - swingStart;
			VectorNormalize(vecToTarget);

			float dot = vecToTarget.Dot(forward);

			// YWB:  Make sure they are sort of facing the guy at least...
			if (dot < 0.70721f)
			{
				// Force amiss
				traceHit.fraction = 1.0f;
			}
			else
			{
				ChooseIntersectionPointAndActivity(traceHit, g_bludgeonMins, g_bludgeonMaxs, pOwner);
			}
		}
	}

	if (traceHit.fraction == 1.0f)
	{
		// We want to test the first swing again
		Vector testEnd = swingStart + forward * GetRange();

		// See if we happened to hit water
		ImpactWater(swingStart, testEnd);
	}
	else
	{
#ifndef CLIENT_DLL
		CTakeDamageInfo triggerInfo(GetOwner(), GetOwner(), GetDamageForActivity(GetActivity()), GetDamageType());
		triggerInfo.SetDamagePosition(traceHit.startpos);
		triggerInfo.SetDamageForce(forward);
#endif

		// Other melee sounds
		if (traceHit.m_pEnt && traceHit.m_pEnt->IsWorld())
			WeaponSound(MELEE_HIT_WORLD);
#ifndef CLIENT_DLL
		else if (traceHit.m_pEnt && !traceHit.m_pEnt->PassesDamageFilter(triggerInfo))
			WeaponSound(MELEE_MISS);
#endif
		else
			WeaponSound(MELEE_HIT);

		Hit( traceHit, GetActivity(), true );
	}
}

bool CBaseHL2MPBludgeonWeapon::CanHolster(void)
{
	if (m_bShotDelayed)
		return false;

	return BaseClass::CanHolster();
}
#endif // MAPBASE
