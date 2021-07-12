//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "c_te_legacytempents.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ShellEjectCallback( const CEffectData &data )
{
	// Use the gun angles to orient the shell
	IClientRenderable *pRenderable = data.GetRenderable();
	if ( pRenderable )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pRenderable->GetRenderAngles(), 0 );
	}
}

DECLARE_CLIENT_EFFECT( "ShellEject", ShellEjectCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RifleShellEjectCallback( const CEffectData &data )
{
	// Use the gun angles to orient the shell
	IClientRenderable *pRenderable = data.GetRenderable();
	if ( pRenderable )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pRenderable->GetRenderAngles(), 1 );
	}
}

DECLARE_CLIENT_EFFECT( "RifleShellEject", RifleShellEjectCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ShotgunShellEjectCallback( const CEffectData &data )
{
	// Use the gun angles to orient the shell
	IClientRenderable *pRenderable = data.GetRenderable();
	if ( pRenderable )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pRenderable->GetRenderAngles(), 2 );
	}
}

DECLARE_CLIENT_EFFECT( "ShotgunShellEject", ShotgunShellEjectCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RevolverShellEjectCallback( const CEffectData &data )
{
	// Use the gun angles to orient the shell
	IClientRenderable *pRenderable = data.GetRenderable();
	if ( pRenderable )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pRenderable->GetRenderAngles(), 3 );
	}
}

DECLARE_CLIENT_EFFECT( "RevolverShellEject", RevolverShellEjectCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FlareShellEjectCallback( const CEffectData &data )
{
	// Use the gun angles to orient the shell
	IClientRenderable *pRenderable = data.GetRenderable();
	if ( pRenderable )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pRenderable->GetRenderAngles(), 3 );
	}
}

DECLARE_CLIENT_EFFECT( "FlareShellEject", FlareShellEjectCallback );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OicwMagEjectCallback( const CEffectData &data )
{
	// Use the gun angles to orient the shell
	IClientRenderable *pRenderable = data.GetRenderable();
	if ( pRenderable )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pRenderable->GetRenderAngles(), 10 );
	}
}

DECLARE_CLIENT_EFFECT( "OicwMagEject", OicwMagEjectCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SMG1MagEjectCallback( const CEffectData &data )
{
	// Use the gun angles to orient the shell
	IClientRenderable *pRenderable = data.GetRenderable();
	if ( pRenderable )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pRenderable->GetRenderAngles(), 11 );
	}
}

DECLARE_CLIENT_EFFECT( "SMG1MagEject", SMG1MagEjectCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PistolMagEjectCallback( const CEffectData &data )
{
	// Use the gun angles to orient the shell
	IClientRenderable *pRenderable = data.GetRenderable();
	if ( pRenderable )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pRenderable->GetRenderAngles(), 12 );
	}
}

DECLARE_CLIENT_EFFECT( "PistolMagEject", PistolMagEjectCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GrenadeMagEjectCallback( const CEffectData &data )
{
	// Use the gun angles to orient the shell
	IClientRenderable *pRenderable = data.GetRenderable();
	if ( pRenderable )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pRenderable->GetRenderAngles(), 13 );
	}
}

DECLARE_CLIENT_EFFECT( "GrenadeMagEject", GrenadeMagEjectCallback );
