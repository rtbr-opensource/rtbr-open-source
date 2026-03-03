//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Combine guard gun, strider destroyer
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "player.h"
#include "grenade_ar2.h"
#include "soundent.h"
#include "explode.h"
#include "shake.h"
#include "energy_wave.h"
#include "IEffects.h"
#include "te_particlesystem.h"
#include "te_effect_dispatch.h"
#include "ndebugoverlay.h"
#include "gamestats.h"

class CTEConcussiveExplosion : public CTEParticleSystem
{
public:
	DECLARE_CLASS( CTEConcussiveExplosion, CTEParticleSystem );
	DECLARE_SERVERCLASS();

	CTEConcussiveExplosion( const char *name );
	virtual	~CTEConcussiveExplosion( void );

	CNetworkVector( m_vecNormal );
	CNetworkVar( float, m_flScale );
	CNetworkVar( int, m_nRadius );
	CNetworkVar( int, m_nMagnitude );
};

IMPLEMENT_SERVERCLASS_ST( CTEConcussiveExplosion, DT_TEConcussiveExplosion )
	SendPropVector( SENDINFO(m_vecNormal), -1, SPROP_COORD ),
	SendPropFloat( SENDINFO(m_flScale), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO(m_nRadius), 32, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nMagnitude), 32, SPROP_UNSIGNED ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTEConcussiveExplosion::CTEConcussiveExplosion( const char *name ) : BaseClass( name )
{
	m_nRadius		= 0;
	m_nMagnitude	= 0;
	m_flScale		= 0.0f;

	m_vecNormal.Init();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTEConcussiveExplosion::~CTEConcussiveExplosion( void )
{
}


// Singleton to fire TEExplosion objects
static CTEConcussiveExplosion g_TEConcussiveExplosion( "ConcussiveExplosion" );

void TE_ConcussiveExplosion( IRecipientFilter& filter, float delay,
	const Vector* pos, float scale, int radius, int magnitude, const Vector* normal )
{
	g_TEConcussiveExplosion.m_vecOrigin		= *pos;
	g_TEConcussiveExplosion.m_flScale			= scale;
	g_TEConcussiveExplosion.m_nRadius			= radius;
	g_TEConcussiveExplosion.m_nMagnitude		= magnitude;

	if ( normal )
		g_TEConcussiveExplosion.m_vecNormal	= *normal;
	else 
		g_TEConcussiveExplosion.m_vecNormal	= Vector(0,0,1);

	// Send it over the wire
	g_TEConcussiveExplosion.Create( filter, delay );
}

//Temp ent for the blast

#ifdef MAPBASE
#define SF_CONCUSSIVEBLAST_REPEATABLE 0x00000001
#endif

class CConcussiveBlast : public CBaseEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CConcussiveBlast, CBaseEntity );

	int		m_spriteTexture;

#ifdef MAPBASE
	float	m_flDamage = 200;
	float	m_flRadius = 256;
	float	m_flMagnitude = 1.0;
	string_t	m_iszSoundName;
#endif

	CConcussiveBlast( void ) {}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output :
	//-----------------------------------------------------------------------------
	void Precache( void )
	{
		m_spriteTexture = PrecacheModel( "sprites/lgtning.vmt" );

#ifdef MAPBASE
		if (m_iszSoundName != NULL_STRING)
			PrecacheScriptSound(STRING(m_iszSoundName));
#endif

		BaseClass::Precache();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output :
	//-----------------------------------------------------------------------------

	void Explode( float magnitude )
	{
		//Create a concussive explosion
		CPASFilter filter( GetAbsOrigin() );

		Vector vecForward;
		AngleVectors( GetAbsAngles(), &vecForward );
		TE_ConcussiveExplosion( filter, 0.0,
			&GetAbsOrigin(),//position
			1.0f,	//scale
			256*magnitude,	//radius
			175*magnitude,	//magnitude
			&vecForward );	//normal
		
		int	colorRamp = random->RandomInt( 128, 255 );

		//Shockring
		CBroadcastRecipientFilter filter2;
		te->BeamRingPoint( filter2, 0, 
			GetAbsOrigin(),	//origin
			16,			//start radius
			300*magnitude,		//end radius
			m_spriteTexture, //texture
			0,			//halo index
			0,			//start frame
			2,			//framerate
			0.3f,		//life
			128,		//width
			16,			//spread
			0,			//amplitude
			colorRamp,	//r
			colorRamp,	//g
			255,		//g
			24,			//a
			128			//speed
			);

		//Do the radius damage
#ifdef MAPBASE
		RadiusDamage( CTakeDamageInfo( this, GetOwnerEntity(), m_flDamage, DMG_BLAST|DMG_DISSOLVE ), GetAbsOrigin(), m_flRadius, CLASS_NONE, NULL );
#else
		RadiusDamage( CTakeDamageInfo( this, GetOwnerEntity(), 200, DMG_BLAST|DMG_DISSOLVE ), GetAbsOrigin(), 256, CLASS_NONE, NULL );
#endif

#ifdef MAPBASE
		if (m_iszSoundName != NULL_STRING)
			EmitSound(STRING(m_iszSoundName));

		if (!HasSpawnFlags(SF_CONCUSSIVEBLAST_REPEATABLE))
#endif
		UTIL_Remove( this );
	}

#ifdef MAPBASE
	void InputExplode( inputdata_t &inputdata )
	{
		Explode(m_flMagnitude);
	}

	void InputExplodeWithMagnitude( inputdata_t &inputdata )
	{
		Explode(inputdata.value.Int());
	}
#endif
};

LINK_ENTITY_TO_CLASS( concussiveblast, CConcussiveBlast );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CConcussiveBlast )

//	DEFINE_FIELD( m_spriteTexture,	FIELD_INTEGER ),

#ifdef MAPBASE
	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "damage" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( m_flMagnitude, FIELD_FLOAT, "magnitude" ),
	DEFINE_KEYFIELD( m_iszSoundName, FIELD_SOUNDNAME, "soundname" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Explode", InputExplode ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "ExplodeWithMagnitude", InputExplodeWithMagnitude ),
#endif

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Create a concussive blast entity and detonate it
//-----------------------------------------------------------------------------
void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude )
{
	QAngle angles;
	VectorAngles( surfaceNormal, angles );
	CConcussiveBlast *pBlast = (CConcussiveBlast *) CBaseEntity::Create( "concussiveblast", origin, angles, pOwner );

	if ( pBlast )
	{
		pBlast->Explode( magnitude );
	}
}