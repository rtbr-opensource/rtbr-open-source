//========= Copyright (c) RTBR Team, 2024 ============//
//
// Purpose: Client-side version of the Antlion King
// 
// Needed for special particle effects.
// BIG TODO: We need to save-restore all the particles!!!
//
//====================================================//

#include "cbase.h"
#include "c_ai_basenpc.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ANTKING_PUKEZONE_PARTICLE_SYSTEM "npc_antking_spit_AoE_big"

class C_AntKingPukeZone : public C_BaseEntity
{
	DECLARE_CLASS( C_AntKingPukeZone, C_BaseEntity );
	DECLARE_DATADESC();
	DECLARE_CLIENTCLASS();

public:
	virtual void	Precache( void );
	virtual void	Spawn( void );

	// Server to client messaging
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	void			CreateParticleFX( void );

private:
	CNewParticleEffect *m_hPukeZoneEffect;
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( C_AntKingPukeZone )
END_DATADESC()

//---------------------------------------------------------
// Networking
//---------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_AntKingPukeZone, DT_AntKingPukeZone, CAntKingPukeZone )
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS( antking_pukezone, C_AntKingPukeZone );

//---------------------------------------------------------
// Purpose: Precache needed resources
//---------------------------------------------------------
void C_AntKingPukeZone::Precache( void )
{
	PrecacheParticleSystem( ANTKING_PUKEZONE_PARTICLE_SYSTEM );

	BaseClass::Precache();
}

//---------------------------------------------------------
// Purpose: Spawn entity into world
//---------------------------------------------------------
void C_AntKingPukeZone::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
}

//---------------------------------------------------------
// Purpose: Handles Server->Client Messages
//---------------------------------------------------------
void C_AntKingPukeZone::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Start our particles
		CreateParticleFX();
	}
}

//---------------------------------------------------------
// Purpose: Creates our Puke Particle Effect
//			(This will be cleaned up automatically when
//			the zone gets removed by the server)
//---------------------------------------------------------
void C_AntKingPukeZone::CreateParticleFX( void )
{
	// NOTE: Offset the particle by -64 units to set it on the ground.
	m_hPukeZoneEffect = ParticleProp()->Create( ANTKING_PUKEZONE_PARTICLE_SYSTEM, PATTACH_ABSORIGIN, 0, Vector( 0, 0, 0 ) );
}

class C_AntKingShockwaveVolume : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_AntKingShockwaveVolume, C_BaseEntity );
	DECLARE_DATADESC();
	DECLARE_CLIENTCLASS();

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

private:

};

LINK_ENTITY_TO_CLASS( trigger_antking_shockwave_volume, C_AntKingShockwaveVolume );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( C_AntKingShockwaveVolume )
END_DATADESC()

//---------------------------------------------------------
// Networking
//---------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_AntKingShockwaveVolume, DT_AntKingShockwaveVolume, CAntKingShockwaveVolume )
END_RECV_TABLE()

//---------------------------------------------------------
// Purpose: Precache needed resources
//---------------------------------------------------------
void C_AntKingShockwaveVolume::Precache( void )
{
	PrecacheParticleSystem( "antking_shockwave" );

	BaseClass::Precache();
}

//---------------------------------------------------------
// Purpose: Spawn entity into world
//---------------------------------------------------------
void C_AntKingShockwaveVolume::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
}

//---------------------------------------------------------
// Purpose: Handles Server->Client Messages
//---------------------------------------------------------
void C_AntKingShockwaveVolume::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		ParticleProp()->Create( "antking_shockwave", PATTACH_ABSORIGIN_FOLLOW );
	}
}

class C_NPC_AntlionKing : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_NPC_AntlionKing, C_AI_BaseNPC );
	DECLARE_DATADESC();
	DECLARE_CLIENTCLASS();

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

private:
	bool			m_bShieldActive{ false };
	bool			m_bShieldActiveClient{ false };

	CNewParticleEffect	*m_pShieldParticleSystem{ NULL };
};

LINK_ENTITY_TO_CLASS( npc_antlionking, C_NPC_AntlionKing );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( C_NPC_AntlionKing )
	DEFINE_FIELD( m_bShieldActive, FIELD_BOOLEAN ),
	//DEFINE_FIELD( m_bShieldActiveClient, FIELD_BOOLEAN ), Saving this means that the king's shield disappears on load. It gets reset to its proper value on load anyway.
END_DATADESC()

//---------------------------------------------------------
// Networking
//---------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_NPC_AntlionKing, DT_NPC_AntlionKing, CNPC_AntlionKing )
	RecvPropBool( RECVINFO( m_bShieldActive ) ),
END_RECV_TABLE()

//---------------------------------------------------------
// Purpose: Precache needed resources
//---------------------------------------------------------
void C_NPC_AntlionKing::Precache( void )
{
	PrecacheParticleSystem( "antking_shield" );
	PrecacheParticleSystem( "antking_shield_break" );

	BaseClass::Precache();
}

//---------------------------------------------------------
// Purpose: Spawn entity into world
//---------------------------------------------------------
void C_NPC_AntlionKing::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	m_bShieldActiveClient = false;
}

//---------------------------------------------------------
// Purpose: Handles Server->Client Messages
//---------------------------------------------------------
void C_NPC_AntlionKing::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bShieldActive != m_bShieldActiveClient )
	{
		m_bShieldActiveClient = m_bShieldActive;

		Vector vecShieldOffset{ 0.0f, 0.0f, 650.0f };
		if ( m_bShieldActiveClient && !m_pShieldParticleSystem )
		{
			// Create our shield.
			m_pShieldParticleSystem = ParticleProp()->Create( "antking_shield", PATTACH_ABSORIGIN, 0, vecShieldOffset );
		}
		else if ( !m_bShieldActiveClient && m_pShieldParticleSystem )
		{
			// Kill our shield and dispatch the "Pop!"
			ParticleProp()->StopEmission( m_pShieldParticleSystem );
			m_pShieldParticleSystem = NULL;
			ParticleProp()->Create( "antking_shield_break", PATTACH_ABSORIGIN, 0, vecShieldOffset );
		}
	}
}
