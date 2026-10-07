//=============================================================================
//
// Purpose: protozoan
//
//=============================================================================
#include "cbase.h"
#include "c_ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_NPC_Protozoan : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_NPC_Protozoan, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

public:
	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	ClientThink( void );

private:
	bool				m_bDispatchDeathVFX;
	CNewParticleEffect	*m_pIdleVFX;
	CNewParticleEffect	*m_pDeathVFX;
};

//---------------------------------------------------------
// Networking.
//---------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_NPC_Protozoan, DT_NPC_Protozoan, CNPC_Protozoan )
	RecvPropBool( RECVINFO( m_bDispatchDeathVFX ) ),
END_RECV_TABLE()

//---------------------------------------------------------
// Precache.
//---------------------------------------------------------
void C_NPC_Protozoan::Precache( void )
{
	// Particles
	PrecacheParticleSystem( "protozoan_idle" );
	PrecacheParticleSystem( "protozoan_death" );

	BaseClass::Precache();
}

//---------------------------------------------------------
// Spawn.
//---------------------------------------------------------
void C_NPC_Protozoan::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
}

//---------------------------------------------------------
// Handle state changes from the server.
//---------------------------------------------------------
void C_NPC_Protozoan::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		m_pIdleVFX = ParticleProp()->Create( "protozoan_idle", PATTACH_ABSORIGIN_FOLLOW );
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

//---------------------------------------------------------
// Update our VFX.
//---------------------------------------------------------
void C_NPC_Protozoan::ClientThink( void )
{
	if ( m_bDispatchDeathVFX )
	{
		if ( m_pIdleVFX )
		{
			m_pIdleVFX->StopEmission( false, true, true );
			m_pIdleVFX = NULL;
		}

		if ( !m_pDeathVFX )
			m_pDeathVFX = ParticleProp()->Create( "protozoan_death", PATTACH_ABSORIGIN_FOLLOW );
	}
}
