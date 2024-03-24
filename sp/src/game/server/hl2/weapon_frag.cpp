//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "grenade_frag.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "items.h"
#include "in_buttons.h"
#include "soundent.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRENADE_TIMER	3.0f //Seconds
#define GRENADE_BLIP_TIMER 1.0f // seconds

#define GRENADE_PAUSED_NO			0
#define GRENADE_PAUSED_PRIMARY		1
#define GRENADE_PAUSED_SECONDARY	2

#define GRENADE_RADIUS	4.0f // inches



//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CWeaponFrag: public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponFrag, CBaseHLCombatWeapon );
public:
	DECLARE_SERVERCLASS();

public:
	CWeaponFrag();

	void	Precache( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	DecrementAmmo( CBaseCombatCharacter *pOwner );
	void	ItemPostFrame( void );
	float	GetElapsedCookTime( void ) { return gpGlobals->curtime - m_flDrawbackTime; }

	bool Holster(CBaseCombatWeapon * pSwitchingTo);
	
	//First time Pickup Anim function
	void	OnPickedUp(CBaseCombatCharacter *pNewOwner);

	bool	Deploy( void );

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	float	SelectOptimalGrenadeTimer(void);

	bool	Reload( void );

	bool	ShouldDisplayHUDHint() { return true; }

private:
	void	ThrowGrenade( CBasePlayer *pPlayer );
	void	RollGrenade( CBasePlayer *pPlayer );
	void	LobGrenade( CBasePlayer *pPlayer );
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc );

	bool	m_bRedraw;	//Draw the weapon again after throwing a grenade
	
	int		m_AttackPaused;
	bool	m_fDrawbackFinished;
	float	m_flDrawbackTime;
	float	m_fNextBlip;

	DECLARE_ACTTABLE();

	DECLARE_DATADESC();
};


BEGIN_DATADESC( CWeaponFrag )
	DEFINE_FIELD( m_bRedraw, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_AttackPaused, FIELD_INTEGER ),
	DEFINE_FIELD( m_fDrawbackFinished, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flDrawbackTime, FIELD_FLOAT),
	DEFINE_FIELD(m_fNextBlip, FIELD_FLOAT),
END_DATADESC()

acttable_t	CWeaponFrag::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },

#ifdef MAPBASE
	// HL2:DM activities (for third-person animations in SP)
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_GRENADE,                    false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_GRENADE,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_GRENADE,            false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_GRENADE,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_GRENADE,        false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_GRENADE,			false },
#if EXPANDED_HL2DM_ACTIVITIES
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_GRENADE,					false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_GRENADE,    false },
#endif
#endif
};

IMPLEMENT_ACTTABLE(CWeaponFrag);

IMPLEMENT_SERVERCLASS_ST(CWeaponFrag, DT_WeaponFrag)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_frag, CWeaponFrag );
PRECACHE_WEAPON_REGISTER(weapon_frag);



CWeaponFrag::CWeaponFrag() :
	CBaseHLCombatWeapon(),
	m_bRedraw( false )
{
	NULL;
}

//-----------------------------------------------------------------------------
// First pickup anim experiment from Iridium
//-----------------------------------------------------------------------------

void CWeaponFrag::OnPickedUp(CBaseCombatCharacter *pNewOwner)
{
	BaseClass::OnPickedUp(pNewOwner);
	//SendWeaponAnim(ACT_VM_FIRSTDRAW);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::Precache( void )
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "npc_grenade_frag" );

	PrecacheScriptSound("Grenade.Blip");
	PrecacheScriptSound( "WeaponFrag.Throw" );
	PrecacheScriptSound( "WeaponFrag.Roll" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponFrag::Deploy( void )
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Deploy();
}
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	pOwner->m_Local.m_flGrenadeStart = -1;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponFrag::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	bool fThrewGrenade = false;

	switch( pEvent->event )
	{
		case EVENT_WEAPON_SEQUENCE_FINISHED:
			m_fDrawbackFinished = true;
			pOwner->m_Local.m_flGrenadeStart = gpGlobals->curtime;
			m_flDrawbackTime = gpGlobals->curtime;
			EmitSound( "Grenade.Blip" );
			m_fNextBlip = m_flDrawbackTime + GRENADE_BLIP_TIMER;
			break;

		case EVENT_WEAPON_THROW:
			ThrowGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			pOwner->m_Local.m_flGrenadeStart = -1;
			break;

		case EVENT_WEAPON_THROW2:
			RollGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			pOwner->m_Local.m_flGrenadeStart = -1;
			break;

		case EVENT_WEAPON_THROW3:
			LobGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			pOwner->m_Local.m_flGrenadeStart = -1;
			break;

		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}

#define RETHROW_DELAY	0.5
	if( fThrewGrenade )
	{
		m_flNextPrimaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!

		// Make a sound designed to scare snipers back into their holes!
		CBaseCombatCharacter *pOwner = GetOwner();

		if( pOwner )
		{
			Vector vecSrc = pOwner->Weapon_ShootPosition();
			Vector	vecDir;

			AngleVectors( pOwner->EyeAngles(), &vecDir );

			trace_t tr;

			UTIL_TraceLine( vecSrc, vecSrc + vecDir * 1024, MASK_SOLID_BRUSHONLY, pOwner, COLLISION_GROUP_NONE, &tr );

			CSoundEnt::InsertSound( SOUND_DANGER_SNIPERONLY, tr.endpos, 384, 0.2, pOwner );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Reload( void )
{
	if ( !HasPrimaryAmmo() )
		return false;

	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::SecondaryAttack( void )
{
	if (m_fDrawbackFinished) { return; } // no changing your mind!!!
	if ( m_bRedraw )
		return;

	if ( !HasPrimaryAmmo() )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();

	if ( pOwner == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOwner );
	
	if ( pPlayer == NULL )
		return;

	// Note that this is a secondary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_SECONDARY;
	SendWeaponAnim( ACT_VM_PULLBACK_LOW );

	// Don't let weapon idle interfere in the middle of a throw!
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextSecondaryAttack	= FLT_MAX;

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::PrimaryAttack( void )
{
	if (m_fDrawbackFinished) { return; } // no changing your mind!!!
	if ( m_bRedraw )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
	{ 
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );;

	if ( !pPlayer )
		return;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_PRIMARY;
	SendWeaponAnim( ACT_VM_PULLBACK_HIGH );
	
	// Put both of these off indefinitely. We do not know how long
	// the player will hold the grenade.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponFrag::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::ItemPostFrame( void )
{
	if( m_fDrawbackFinished )
	{
		// blip with same blip timer system as a thrown grenade, i.e. start slow, go fast
		if (GetElapsedCookTime() <= GRENADE_TIMER)
		{
			if ( gpGlobals->curtime >= m_fNextBlip )
			{
				EmitSound("Grenade.Blip");
				if ( GetElapsedCookTime() > FRAG_GRENADE_WARN_TIME ){
					m_fNextBlip += FRAG_GRENADE_BLIP_FAST_FREQUENCY;
				}
				else{
					m_fNextBlip += FRAG_GRENADE_BLIP_FREQUENCY;
				}
			}
		}
		
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

		if (pOwner)
		{
			switch( m_AttackPaused )
			{
			case GRENADE_PAUSED_PRIMARY:
				if( !(pOwner->m_nButtons & IN_ATTACK) )
				{
					SendWeaponAnim( ACT_VM_THROW );
					m_fDrawbackFinished = false;
					m_flNextSecondaryAttack = gpGlobals->curtime + RETHROW_DELAY;
				}
				break;

			case GRENADE_PAUSED_SECONDARY:
				if( !(pOwner->m_nButtons & IN_ATTACK2) )
				{
					//See if we're ducking
					if ( pOwner->m_nButtons & IN_DUCK )
					{
						//Send the weapon animation
						SendWeaponAnim( ACT_VM_SECONDARYATTACK );
					}
					else
					{
						//Send the weapon animation
						SendWeaponAnim( ACT_VM_HAULBACK );
					}

					m_fDrawbackFinished = false;
					m_flNextPrimaryAttack = gpGlobals->curtime + RETHROW_DELAY;
				}
				break;

			default:
				break;
			}
		}
	}

	BaseClass::ItemPostFrame();

	if ( m_bRedraw )
	{
		if ( IsViewModelSequenceFinished() )
		{
			Reload();
		}
	}
}

	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponFrag::CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc )
{
	trace_t tr;

	UTIL_TraceHull( vecEye, vecSrc, -Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), 
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
	{
		vecSrc = tr.endpos;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::ThrowGrenade( CBasePlayer *pPlayer )
{
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
//	vForward[0] += 0.1f;
	vForward[2] += 0.1f;

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * 1200;
	Fraggrenade_Create( vecSrc, vec3_angle, vecThrow, AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ), pPlayer, GRENADE_TIMER - GetElapsedCookTime(), false, m_flDrawbackTime );

	m_bRedraw = true;

	WeaponSound( SINGLE );

#ifdef MAPBASE
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
#endif

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::LobGrenade( CBasePlayer *pPlayer )
{
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f + Vector( 0, 0, -8 );
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
	
	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * 350 + Vector( 0, 0, 50 );
	Fraggrenade_Create( vecSrc, vec3_angle, vecThrow, AngularImpulse( 200, random->RandomInt( -600, 600 ), 0 ), pPlayer, GRENADE_TIMER - GetElapsedCookTime(), false, m_flDrawbackTime );

	WeaponSound( WPN_DOUBLE );

#ifdef MAPBASE
	pPlayer->SetAnimation( PLAYER_ATTACK2 );
#endif

	m_bRedraw = true;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::RollGrenade( CBasePlayer *pPlayer )
{
	// BUGBUG: Hardcoded grenade width of 4 - better not change the model :)
	Vector vecSrc;
	pPlayer->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &vecSrc );
	vecSrc.z += GRENADE_RADIUS;

	Vector vecFacing = pPlayer->BodyDirection2D( );
	// no up/down direction
	vecFacing.z = 0;
	VectorNormalize( vecFacing );
	trace_t tr;
	UTIL_TraceLine( vecSrc, vecSrc - Vector(0,0,16), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 )
	{
		// compute forward vec parallel to floor plane and roll grenade along that
		Vector tangent;
		CrossProduct( vecFacing, tr.plane.normal, tangent );
		CrossProduct( tr.plane.normal, tangent, vecFacing );
	}
	vecSrc += (vecFacing * 18.0);
	CheckThrowPosition( pPlayer, pPlayer->WorldSpaceCenter(), vecSrc );

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vecFacing * 700;
	// put it on its side
	QAngle orientation(0,pPlayer->GetLocalAngles().y,-90);
	// roll it
	AngularImpulse rotSpeed(0,0,720);
	Fraggrenade_Create( vecSrc, orientation, vecThrow, rotSpeed, pPlayer, GRENADE_TIMER - GetElapsedCookTime(), false, m_flDrawbackTime );

	WeaponSound( SPECIAL1 );

#ifdef MAPBASE
	pPlayer->SetAnimation( PLAYER_ATTACK2 );
#endif

	m_bRedraw = true;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
}

//
// unused until i work out how best to integrate this with the sound system, if i ever do
//
float CWeaponFrag::SelectOptimalGrenadeTimer(void){
	if (m_flDrawbackTime - gpGlobals->curtime >= -0.5f){
		return GRENADE_TIMER; // 1/2 second grace period for maximum frag timer
	}
	else{
		return MAX(MIN(m_flDrawbackTime + GRENADE_TIMER - gpGlobals->curtime + 0.125f, GRENADE_TIMER), 0.01f);	// otherwise just clamp the timer to [0.01, 3] sec.	
																												// extra 0.125 seconds under the hood so the player doesn't take damage if they release
																												// just before hitting the red zone
	}
}

