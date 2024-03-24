//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl1_grenade_spit.h"
#include "soundent.h"
#include "decals.h"

#include "smoke_trail.h"
#include "hl2_shareddefs.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

extern ConVar sk_bullsquid_dmg_spit;

BEGIN_DATADESC( CGrenadeSpitHL1 )

	// Function pointers
	DEFINE_THINKFUNC( SpitThink ),
	DEFINE_ENTITYFUNC( GrenadeSpitHL1Touch ),

	//DEFINE_FIELD( m_nSquidSpitSprite, FIELD_INTEGER ),

	DEFINE_FIELD( m_fSpitDeathTime, FIELD_TIME ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( grenade_spit_hl1, CGrenadeSpitHL1 );

void CGrenadeSpitHL1::Spawn( void )
{
	Precache( );
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY );

	// FIXME, if these is a sprite, then we need a base class derived from CSprite rather than
	// CBaseAnimating.  pev->scale becomes m_flSpriteScale in that case.
	SetModel( "models/spitball_large.mdl" );
	UTIL_SetSize(this, Vector(-3, -3, -3), Vector(3, 3, 3));

	m_nRenderMode		= kRenderTransAdd;
	SetRenderColor( 255, 255, 255, 255 );
	m_nRenderFX		= kRenderFxNone;

	SetThink( &CGrenadeSpitHL1::SpitThink );
	SetUse( &CBaseGrenade::DetonateUse ); 
	SetTouch( &CGrenadeSpitHL1::GrenadeSpitHL1Touch );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= sk_bullsquid_dmg_spit.GetFloat();
	m_DmgRadius		= 60.0f;
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	SetGravity( SPIT_GRAVITY );
	SetFriction( 0.8 );
	SetSequence( 1 );

	SetCollisionGroup( HL2COLLISION_GROUP_SPIT );
}


void CGrenadeSpitHL1::SetSpitSize(int nSize)
{
	switch (nSize)
	{
		case SPIT_LARGE:
		{
			SetModel( "models/spitball_large.mdl" );
			break;
		}
		case SPIT_MEDIUM:
		{
			SetModel( "models/spitball_medium.mdl" );
			break;
		}
		case SPIT_SMALL:
		{
			SetModel( "models/spitball_small.mdl" );
			break;
		}
	}
}

void CGrenadeSpitHL1::Event_Killed( const CTakeDamageInfo &info )
{
	Detonate( );
}

void CGrenadeSpitHL1::GrenadeSpitHL1Touch( CBaseEntity *pOther )
{
	if (m_fSpitDeathTime != 0)
	{
		return;
	}
	if ( pOther->GetCollisionGroup() == HL2COLLISION_GROUP_SPIT)
	{
		return;
	}
	if ( !pOther->m_takedamage )
	{

		// make a splat on the wall
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() * 10, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
		UTIL_DecalTrace(&tr, "BeerSplash" );

		// make some flecks
		CPVSFilter filter( tr.endpos );
		te->SpriteSpray( filter, 0.0,
			&tr.endpos, &tr.plane.normal, m_nSquidSpitSprite, random->RandomInt( 90, 160 ), 50, random->RandomInt ( 5, 15 ) );
	}
	else
	{
		RadiusDamage ( CTakeDamageInfo( this, GetThrower(), m_flDamage, DMG_BLAST ), GetAbsOrigin(), m_DmgRadius, CLASS_NONE, NULL );
	}

	Detonate();
}

void CGrenadeSpitHL1::SpitThink( void )
{
	if (m_fSpitDeathTime != 0 &&
		m_fSpitDeathTime < gpGlobals->curtime)
	{
		UTIL_Remove( this );
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CGrenadeSpitHL1::Detonate(void)
{
	m_takedamage	= DAMAGE_NO;	

	int		iPitch;

	// splat sound
	iPitch = random->RandomFloat( 90, 110 );

	EmitSound( "GrenadeSpitHL1.Acid" );	
	EmitSound( "GrenadeSpitHL1.Hit" );	

	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeSpitHL1::Precache( void )
{
	m_nSquidSpitSprite = PrecacheModel("sprites/bigspit.vmt");// client side spittle.

	PrecacheModel("models/spitball_large.mdl"); 
	PrecacheModel("models/spitball_medium.mdl"); 
	PrecacheModel("models/spitball_small.mdl"); 

	PrecacheScriptSound( "GrenadeSpitHL1.Acid" );	
	PrecacheScriptSound( "GrenadeSpitHL1.Hit" );	

}


CGrenadeSpitHL1::CGrenadeSpitHL1(void)
{
}
