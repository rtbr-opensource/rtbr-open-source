//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "c_env_zoom.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_EnvZoom, DT_EnvZoom, CEnvZoom )
	RecvPropFloat( RECVINFO( m_flSpeed ) ),
	RecvPropInt( RECVINFO( m_nFOV ) ),
	RecvPropInt( RECVINFO_NAME( m_nSpawnFlags, m_spawnflags ) ),
END_RECV_TABLE()

bool CanOverrideEnvZoomOwner( C_BaseEntity *pZoomOwner )
{
	C_EnvZoom *pZoom = dynamic_cast<C_EnvZoom *>( pZoomOwner );
	return pZoom && pZoom->CanOverride();
}

float GetZoomOwnerDesiredFOV( C_BaseEntity *pZoomOwner )
{
	C_EnvZoom *pZoom = dynamic_cast<C_EnvZoom *>( pZoomOwner );
	return pZoom && pZoom->CanOverride() ? pZoom->GetFOV() : 0;
}
