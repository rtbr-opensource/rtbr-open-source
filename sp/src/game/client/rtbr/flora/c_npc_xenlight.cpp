//========= Copyright (c) RTBR Team, 2024 ============//
//
// Purpose: Xen Light
// Client-side implementation for  lighting
//
//====================================================//

#include "cbase.h"
#include "c_ai_basenpc.h"
#include "dlight.h"
#include "iefx.h"

// memdbgon must be the last include file in a .cpp!!!
#include "tier0/memdbgon.h"

class C_NPC_XenLight : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_NPC_XenLight, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();
	DECLARE_DATADESC();

public:
	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual void	ClientThink( void );

private:
	bool	m_bIsExtended;			// Sent from the server
	bool	m_bIsExtendedClient;	// Variable to compare with the server

	bool	m_bHasDlight;			// Comes from the server, telling us if we need a dynamic light.
									// We need this because we can't check spawnflags on the client (as far as I can tell).
									// No need to cache/compare it though because it will only be set once.
	dlight_t *m_dlight{ NULL };
};

//---------------------------------------------------------
// Networking
//---------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_NPC_XenLight, DT_NPC_XenLight, CNPC_XenLight )
	RecvPropBool( RECVINFO( m_bIsExtended ) ),
	RecvPropBool( RECVINFO( m_bHasDlight ) ),
END_RECV_TABLE()

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( C_NPC_XenLight )
	DEFINE_FIELD( m_bIsExtended, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsExtendedClient, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHasDlight, FIELD_BOOLEAN ),
	//DEFINE_FIELD( m_dlight, FIELD_EHANDLE )
END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_NPC_XenLight::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( m_bHasDlight )
	{
		if ( ( type == DATA_UPDATE_CREATED ) && m_bIsExtended )
		{
			m_bIsExtendedClient = m_bIsExtended;
			SetNextClientThink( CLIENT_THINK_ALWAYS );
		}

		if ( !m_bIsExtended && m_dlight )
		{
			// If we are retracting, kill our light.
			m_dlight->die = gpGlobals->curtime + 0.5f;
			m_dlight = NULL;
			m_bIsExtendedClient = m_bIsExtended;
		}

		// If we are extending again, setup our light.
		if ( m_bIsExtended && m_bIsExtendedClient != m_bIsExtended )
		{
			m_bIsExtendedClient = m_bIsExtended;
			SetNextClientThink( CLIENT_THINK_ALWAYS );
		}
	}
	else
	{
		// If we don't have a dlight, then the client basically never needs to think!
		SetNextClientThink( TICK_NEVER_THINK );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_NPC_XenLight::ClientThink( void )
{
	// We have either just spawned OR we are extending after being retracted.
	// Re-create our light
	if ( m_bIsExtended && !m_dlight && m_bHasDlight )
	{
		Vector lightAttachment;
		GetAttachment( 1, lightAttachment );

		m_dlight = effects->CL_AllocDlight( index );
		m_dlight->origin = lightAttachment + Vector( 0, 0, 16 );
		m_dlight->color.r = 231;	// original was 225, 202, 111
		m_dlight->color.g = 150;
		m_dlight->color.b = 14;
		m_dlight->die = FLT_MAX;
		m_dlight->radius = 500.0f;
	}

	// The client doesn't think again until state changes.
	SetNextClientThink( TICK_NEVER_THINK );

	BaseClass::ClientThink();
}
