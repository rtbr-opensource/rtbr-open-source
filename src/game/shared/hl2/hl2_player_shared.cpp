//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "hl2_player_shared.h"
#include "in_buttons.h"
#ifdef CLIENT_DLL
#include "c_env_zoom.h"
#else
#include "env_zoom.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool CHL2_Player::CanZoom( CBaseEntity *pRequester )
{
	return !IsZooming();
}

void CHL2_Player::ToggleZoom( void )
{
	if ( IsZooming() )
		StopZooming();
	else
		StartZooming();
}

void CHL2_Player::StartZooming( void )
{
	if ( SetFOV( this, GetSuitZoomFOV(), 0.4f ) )
		m_HL2Local.m_bZooming = true;
}

void CHL2_Player::StopZooming( void )
{
	int iFOV = GetZoomOwnerDesiredFOV( m_hZoomOwner );
	if ( SetFOV( this, iFOV, 0.2f ) )
		m_HL2Local.m_bZooming = false;
}

bool CHL2_Player::IsZooming( void )
{
	return m_hZoomOwner != NULL;
}

void CHL2_Player::CheckSuitZoom( void )
{
	if ( IsSuitEquipped() )
	{
		if ( m_afButtonReleased & IN_ZOOM )
			StopZooming();
		else if ( m_afButtonPressed & IN_ZOOM )
			StartZooming();
	}
}
