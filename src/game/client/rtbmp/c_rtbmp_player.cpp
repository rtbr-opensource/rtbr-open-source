//========= Copyright RTBMP Team, All rights reserved. ============//
//
// Purpose:		Player for RTBMP.
//
//=============================================================================//

#include "cbase.h"
#include "c_rtbmp_player.h"
#include "rtbmp_gamerules.h"
#include "in_buttons.h"
#include "usermessages.h"
#include <game/client/iviewport.h>

#ifdef RTBMP
#include "rtbmp_player_shared.h"
#endif

// Don't alias here
#if defined( CRTBMP_Player )
#undef CRTBMP_Player	
#endif

IMPLEMENT_CLIENTCLASS_DT( C_RTBMP_Player, DT_RTBMP_Player, CRTBMP_Player )
	RecvPropFloat( RECVINFO( m_flSpawnProtectTime ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_RTBMP_Player )
	DEFINE_PRED_FIELD( m_flSpawnProtectTime, FIELD_TIME, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

static ConVar cl_defaultmelee( "cl_defaultmelee", "default", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Default Melee" );
static ConVar cl_alwaysrun( "cl_alwaysrun", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "If infinite sprint is enabled, always run." );

extern CSuitPowerDevice SuitDeviceSprint;

C_RTBMP_Player::C_RTBMP_Player()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_RTBMP_Player::~C_RTBMP_Player()
{
	DestroyGlowEffect();
}

C_RTBMP_Player* C_RTBMP_Player::GetLocalRTBMPPlayer()
{
	return (C_RTBMP_Player*)C_BasePlayer::GetLocalPlayer();
}

void C_RTBMP_Player::Precache( void )
{
	PrecacheParticleSystem( "shield_break" );
}

void C_RTBMP_Player::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( RTBMPRules()->IsCoOp() && GetLocalPlayer() != this )
		UpdateGlowEffect();
}

#define PLAYER_FAR_GLOW_DIST_SQR 1024.f * 1024.f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_RTBMP_Player::UpdateGlowEffect( void )
{
	// destroy the existing effect
	if ( m_pGlowEffect )
	{
		DestroyGlowEffect();
	}

	// Players should glow if they can't be seen, or are far enough
	bool bShouldGlow = GetLocalPlayer() && (!GetLocalPlayer()->IsLineOfSightClear( this ) 
						|| EyePosition().DistToSqr( GetLocalPlayer()->EyePosition() ) > PLAYER_FAR_GLOW_DIST_SQR);

	if ( bShouldGlow )
	{
		// create a new effect
		Vector4D vecColor( 30, 220, 220, 255 );
		for ( int i = 0; i < 4; i++ )
		{
			if ( vecColor[i] == 0.0f )
				continue;

			vecColor[i] /= 255.0f;
		}

		m_pGlowEffect = new CGlowObject( this, vecColor.AsVector3D(), vecColor.w, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_RTBMP_Player::DestroyGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}

void C_RTBMP_Player::ClientThink( void )
{
	BaseClass::ClientThink();

	if ( IsLocalPlayer() )
	{
		RenderFx_t fx;
		if ( gpGlobals->curtime < m_flSpawnProtectTime )
		{
			fx = kRenderFxDistort;
		}
		else
		{
			fx = kRenderFxNone;
		}

		for ( int i = 0; i < MAX_VIEWMODELS; i++ )
		{
			if ( GetViewModel( i ) )
				GetViewModel( i )->m_nRenderFX = fx;
		}
	}	
}

void C_RTBMP_Player::PreThink( void )
{
	BaseClass::PreThink();

	if (m_fIsSprinting)
	{
		// Disable sprint while ducked unless we're in the air (jumping)
		if (IsDucked() && (GetGroundEntity() != NULL))
		{
			StopSprinting();
		}
	}
}

void C_RTBMP_Player::PostThink( void )
{
	BaseClass::PostThink();

	// Re-enable sprint when we're getting out of the duck state
	if (!m_fIsSprinting &&
		!(m_nButtons & IN_SPEED) &&
		!IsDucked() && !IsDucking() &&
		CanAlwaysRun() &&
		(GetGroundEntity() != NULL))
	{
		StartSprinting();
	}
}

bool C_RTBMP_Player::CanAlwaysRun( void )
{
	return cl_alwaysrun.GetBool() && RTBMPRules()->IsSprintInfinite();
}

extern ConVar sv_maxspeed;

void C_RTBMP_Player::HandleSpeedChanges( CMoveData *mv )
{
	if (CanAlwaysRun())
	{
		int nChangedButtons = mv->m_nButtons ^ mv->m_nOldButtons;

		const bool bWantSprint = ( CanSprint() && IsSuitEquipped() && !( mv->m_nButtons & IN_SPEED ) );
		const bool bIsStandingNotSprinting = !( m_Local.m_bDucked || m_Local.m_bDucking ) && !m_HL2Local.m_bNewSprinting;
		const bool bWantsToChangeSprinting = ( m_HL2Local.m_bNewSprinting != bWantSprint ) && (( nChangedButtons & IN_SPEED ) != 0 || bIsStandingNotSprinting);

		bool bSprinting = m_HL2Local.m_bNewSprinting;
		if ( bWantsToChangeSprinting )
		{
			if ( bWantSprint )
			{
				bSprinting = true;
			}
			else
			{
				bSprinting = false;
			}
		}

		if ( m_HL2Local.m_flSuitPower < 0.01 )
		{
			bSprinting = false;
		}

		bool bWantWalking;

		if ( IsSuitEquipped() )
		{
			bWantWalking = ( mv->m_nButtons & IN_WALK ) && !bSprinting && !( mv->m_nButtons & IN_DUCK );
		}
		else
		{
			bWantWalking = true;
		}

		if ( bWantWalking )
		{
			bSprinting = false;
		}

		m_HL2Local.m_bNewSprinting = bSprinting;

		if ( bSprinting )
		{
			mv->m_flClientMaxSpeed = HL2_SPRINT_SPEED;
		}
		else if ( bWantWalking )
		{
			mv->m_flClientMaxSpeed = HL2_WALK_SPEED;
		}
		else
		{
			mv->m_flClientMaxSpeed = HL2_NORM_SPEED;
		}

		mv->m_flMaxSpeed = sv_maxspeed.GetFloat();
	}
	else
	{
		BaseClass::HandleSpeedChanges(mv);
	}
}

void C_RTBMP_Player::ReduceTimers( CMoveData *mv )
{
	bool bSprinting = mv->m_flClientMaxSpeed == HL2_SPRINT_SPEED;

	if (bSprinting && !RTBMPRules()->IsSprintInfinite())
	{
		SuitPower_AddDevice( SuitDeviceSprint );
	}
	else
	{
		SuitPower_RemoveDevice( SuitDeviceSprint );
	}

	SuitPower_Update();
}

void C_RTBMP_Player::ReceiveMessage( int classID, bf_read &msg )
{
	if (classID != GetClientClass()->m_ClassID)
	{
		// message is for subclass
		BaseClass::ReceiveMessage(classID, msg);
		return;
	}

	if (this == GetLocalPlayer())
		return;

	int MessageType = msg.ReadByte();
	switch (MessageType)
	{
		case DAMAGE_FLINCH:
			{
				int HitGroup = msg.ReadLong();
				SetFlinch(HitGroup);
			}
		break;
		case SHIELD_BREAK_FX:
			{
				CNewParticleEffect *pEffect = ParticleProp()->Create( "shield_break", PATTACH_ABSORIGIN_FOLLOW );
				ParticleProp()->AddControlPoint( pEffect, 1, this, PATTACH_ABSORIGIN_FOLLOW );
			}
		break;
	}
}

USER_MESSAGE( XenHeal )
{
	int iPlayerEntIndex = (int)msg.ReadByte();

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerEntIndex );
	if ( pPlayer )
	{
		pPlayer->SetFX( SFX_XENHEAL, (bool)msg.ReadByte() );
	}
}

CON_COMMAND( changeteam, "Choose a new team." )
{
	C_RTBMP_Player *pPlayer = C_RTBMP_Player::GetLocalRTBMPPlayer();

	if ( !pPlayer )
		return;

	if ( !gViewPortInterface )
		return;

	gViewPortInterface->ShowPanel( PANEL_TEAM, true );
}