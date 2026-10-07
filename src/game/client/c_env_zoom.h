//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_ENV_ZOOM_H
#define C_ENV_ZOOM_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"

class C_EnvZoom : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_EnvZoom, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	int GetFOV( void ) const { return m_nFOV; }
	float GetSpeed( void ) const { return m_flSpeed; }
	bool CanOverride( void ) const { return ( m_nSpawnFlags & 1 ) != 0; }

private:
	float m_flSpeed;
	int m_nFOV;
	int m_nSpawnFlags;
};

bool CanOverrideEnvZoomOwner( C_BaseEntity *pZoomOwner );
float GetZoomOwnerDesiredFOV( C_BaseEntity *pZoomOwner );

#endif // C_ENV_ZOOM_H
