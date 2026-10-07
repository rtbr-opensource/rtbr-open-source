//=============== Copyright RTBMP Team, All rights reserved. ==================//
//
// Purpose: CTF gamemode and entity logic.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "capturetheflag.h"
#include "rtbmp_player.h"
#include "team.h"
#include "rtbmp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CCaptureZone)
	DEFINE_OUTPUT( m_OnCapture, "OnCapture" ),
	DEFINE_OUTPUT( m_OnCapTeam1, "OnCapTeam1" ),
	DEFINE_OUTPUT( m_OnCapTeam2, "OnCapTeam2" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_capturezone, CCaptureZone );

CCaptureZone::CCaptureZone()
{
}

void CCaptureZone::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();
	AddSpawnFlags( SF_TRIGGER_ALLOW_ALL );
	Enable();
}

void CCaptureZone::StartTouch( CBaseEntity *pOther )
{
	if (PassesTriggerFilters( pOther ))
	{
		// only players can trigger this trigger so this is a safe cast
		CRTBMP_Player *pPlayer = ToRTBMPPlayer( pOther );
		if (!pPlayer)
			return;

		if (InSameTeam(pPlayer) && pPlayer->GetFlag())
		{
			pPlayer->GetFlag()->OnFlagCaptured();
		}
	}
}

BEGIN_DATADESC( CCaptureFlag )
	DEFINE_KEYFIELD( m_iReturnTime, FIELD_INTEGER, "ReturnTime" ),
	DEFINE_KEYFIELD( m_ModelName, FIELD_MODELNAME, "flag_model" ),
	DEFINE_KEYFIELD( m_bShotClockMode, FIELD_BOOLEAN, "ShotClockMode" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ForceDrop", InputForceDrop ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceReturn", InputForceReturn ),

	DEFINE_OUTPUT( m_OnReturn, "OnReturn" ),
	DEFINE_OUTPUT( m_OnPickup, "OnPickup" ),
	DEFINE_OUTPUT( m_OnPickupTeam1, "OnPickupTeam1" ),
	DEFINE_OUTPUT( m_OnPickupTeam2, "OnPickupTeam2" ),
	DEFINE_OUTPUT( m_OnDrop, "OnDrop" ),
	DEFINE_OUTPUT( m_OnCapture, "OnCapture" ),
	DEFINE_OUTPUT( m_OnCapTeam1, "OnCapTeam1" ),
	DEFINE_OUTPUT( m_OnCapTeam2, "OnCapTeam2" ),
	DEFINE_OUTPUT( m_OnTouchSameTeam, "OnTouchSameTeam" ),

	DEFINE_ENTITYFUNC( FlagTouch ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( item_teamflag, CCaptureFlag );

CCaptureFlag::CCaptureFlag()
{
	m_iFlagState = FLAG_HOME;
}

void CCaptureFlag::Spawn( void )
{
	BaseClass::Spawn();
	m_vecOriginalPosition = GetLocalOrigin();
	m_angOriginalAngles = GetLocalAngles();
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
	RTBMPRules()->SetGameType( GAMETYPE_CTF );
	SetTouch( &CCaptureFlag::FlagTouch );
}

void CCaptureFlag::Precache( void )
{
	PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}

void CCaptureFlag::FlagTouch( CBaseEntity *pOther )
{
	CRTBMP_Player *pPlayer = ToRTBMPPlayer( pOther );
	if (!pPlayer)
		return;

	// players can only carry one flag
	if (pPlayer->GetFlag())
		return;

	// players can't pick up their own team's flag
	if (InSameTeam( pPlayer ))
	{
		m_OnTouchSameTeam.FireOutput( pPlayer, this );
		return;
	}

	pPlayer->PickupFlag( this );
}

void CCaptureFlag::OnFlagPickedUp()
{
	char szFlagMessage[64];
	Q_snprintf( szFlagMessage, sizeof( szFlagMessage ), "The%s flag has been picked up!", GetTeamString() );
	UTIL_ClientPrintAll( HUD_PRINTTALK, szFlagMessage );
	m_iFlagState = FLAG_PICKEDUP;
	SetTouch( NULL );
	m_OnPickup.FireOutput( GetOwnerEntity(), this );
	if (GetOwnerEntity()->GetTeamNumber() == TEAM_COMBINE)
	{
		m_OnPickupTeam1.FireOutput( GetOwnerEntity(), this );
	}
	else if (GetOwnerEntity()->GetTeamNumber() == TEAM_REBELS)
	{
		m_OnPickupTeam2.FireOutput( GetOwnerEntity(), this );
	}
}

void CCaptureFlag::OnFlagDropped()
{
	char szFlagMessage[64];
	Q_snprintf( szFlagMessage, sizeof( szFlagMessage ), "The%s flag has been dropped!", GetTeamString() );
	UTIL_ClientPrintAll( HUD_PRINTTALK, szFlagMessage );
	m_iFlagState = FLAG_DROPPED;
	SetTouch( &CCaptureFlag::FlagTouch );

	m_OnDrop.FireOutput( this, this );
}

void CCaptureFlag::OnFlagCaptured()
{
	// this flag should always have an owner entity if it is capped, so this is a safe cast
	CRTBMP_Player *pPlayer = ToRTBMPPlayer( GetOwnerEntity() );

	GetGlobalTeam( pPlayer->GetTeamNumber() )->AddScore( 1 );

	pPlayer->ReturnFlag();

	ResetFlag();

	if (pPlayer->GetTeamNumber() == TEAM_COMBINE)
	{
		m_OnCapTeam1.FireOutput( pPlayer, this );
		UTIL_ClientPrintAll( HUD_PRINTTALK, "The Combine have captured the flag!" );
	}
	else if (pPlayer->GetTeamNumber() == TEAM_REBELS)
	{
		m_OnCapTeam2.FireOutput( pPlayer, this );
		UTIL_ClientPrintAll( HUD_PRINTTALK, "The Rebels have captured the flag!" );
	}

	m_OnCapture.FireOutput( pPlayer, this );
}

void CCaptureFlag::ReturnFlag()
{
	CRTBMP_Player *pPlayer = ToRTBMPPlayer( GetOwnerEntity() );
	if (pPlayer)
	{
		pPlayer->ReturnFlag();
	}

	ResetFlag();

	char szFlagMessage[64];
	Q_snprintf( szFlagMessage, sizeof( szFlagMessage ), "The%s flag has returned!", GetTeamString() );
	UTIL_ClientPrintAll( HUD_PRINTTALK, szFlagMessage );

	m_OnReturn.FireOutput( this, this );
}

void CCaptureFlag::ResetFlag( void )
{
	SetOwnerEntity( NULL );
	SetParent( NULL );

	SetLocalOrigin( m_vecOriginalPosition );
	SetLocalAngles( m_angOriginalAngles );

	m_iFlagState = FLAG_HOME;

	SetTouch( &CCaptureFlag::FlagTouch );
}

const char *CCaptureFlag::GetTeamString( void )
{
	if (GetTeamNumber() == TEAM_COMBINE)
		return " Combine";
	else if (GetTeamNumber() == TEAM_REBELS)
		return " Rebel";
	return "";
}

void CCaptureFlag::InputForceDrop( inputdata_t &inputdata )
{
	CRTBMP_Player *pPlayer = ToRTBMPPlayer( GetOwnerEntity() );
	if (pPlayer)
		pPlayer->DropFlag();
}