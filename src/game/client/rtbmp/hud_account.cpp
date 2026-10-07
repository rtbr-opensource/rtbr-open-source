//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_rtbmp_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "view_scene.h"
#include "view.h"
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ISoundEmitterSystemBase *soundemitterbase;

// Floating delta text items, float off the top of the frame to 
// show changes to the metal account value
typedef struct 
{
	enum eAccountDeltaType_t
	{
		ACCOUNT_DELTA_INVALID,
		ACCOUNT_DELTA_HEALING,
		ACCOUNT_DELTA_DAMAGE,
	};

	// amount of delta
	int m_iAmount;
	
	bool m_bLargeFont;		// display larger font
	eAccountDeltaType_t m_eDataType;

	// die time
	float m_flDieTime;

	// position
	int m_nX;				// X Pos in screen space & world space
	int m_nXEnd;			// Ending X Pos in screen space and world space
	int m_nHStart;			// Starting Y Pos in screen space, Z pos in world space
	int m_nHEnd;			// Ending Y Pos in screen space, Z pos in world space
	int m_nY;				// Y Coord in world space, not used in screen space
	bool m_bWorldSpace;
	float m_flBatchWindow;
	int m_nSourceID;		// Can be entindex, etc
	Color m_color;
	bool m_bShadows;

	// append a bit of extra text to the end
	wchar_t m_wzText[8];

} account_delta_t;

#define NUM_ACCOUNT_DELTA_ITEMS 10
#define HEAVY_DAMAGE_THRESHOLD 25

ConVar hud_combattext( "hud_combattext", "1", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );
ConVar hud_combattext_batching( "hud_combattext_batching", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX, "If set to 1, numbers that are too close together are merged." );
ConVar hud_combattext_batching_window( "hud_combattext_batching_window", "0.2", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX, "Maximum delay between damage events in order to batch numbers.", true, 0.1, true, 2.0 );
ConVar hud_combattext_doesnt_block_overhead_text( "hud_combattext_doesnt_block_overhead_text", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "If set to 1, allow text like \"CRIT\" to still show over a victim's head." );
ConVar hud_combattext_red( "hud_combattext_red", "255", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );
ConVar hud_combattext_green( "hud_combattext_green", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );
ConVar hud_combattext_blue( "hud_combattext_blue", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );
ConVar hud_combattext_type( "hud_combattext_type", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Which damage value to display when receiving damage numbers." );

ConVar hud_damagemeter( "hud_damagemeter", "0", FCVAR_CHEAT, "Display damage-per-second information in the lower right corner of the screen." );
ConVar hud_damagemeter_period( "hud_damagemeter_period", "0", FCVAR_NONE, "When set to zero, average damage-per-second across all recent damage events, otherwise average damage across defined period (number of seconds)." );
ConVar hud_damagemeter_ooctimer( "hud_damagemeter_ooctimer", "1", FCVAR_NONE, "How many seconds after the last damage event before we consider the player out of combat." );
ConVar hud_damagemeter_report( "hud_damagemeter_report", "1", FCVAR_NONE, "Display end-of-combat DPS result (from first damage even to last before OOC timer hit)." );

ConVar cl_hitsound( "cl_hitsound", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Whether to play a sound when an enemy is hit." );
ConVar cl_killsound( "cl_killsound", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Whether to play a sound when an enemy is killed." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAccountPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CAccountPanel, EditablePanel );

public:
	CAccountPanel( Panel *parent, const char *name )
		: EditablePanel( parent, name )
	{
		m_nBGTexture = -1;
		m_bNegativeFlipDir = false;
		SetDialogVariable( "metal", 0 );
		SetPaintBackgroundEnabled(false);
	}

	virtual void	ApplySchemeSettings( IScheme *scheme ) OVERRIDE;
	virtual void	ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void	Paint( void ) OVERRIDE;

	virtual account_delta_t *OnAccountValueChanged( int iOldValue, int iNewValue, account_delta_t::eAccountDeltaType_t type );

	virtual const char *GetResFileName( void ) { return "resource/UI/HudAccountPanel.res"; }

protected:
	virtual Color GetColor( const account_delta_t::eAccountDeltaType_t &type, const int iDeltaValue = 0 );

	CUtlVector <account_delta_t> m_AccountDeltaItems;

	int m_nBGTexture;
	bool m_bNegativeFlipDir;

	CPanelAnimationVarAliasType( float, m_flDeltaItemStartPos, "delta_item_start_y", "100", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDeltaItemEndPos, "delta_item_end_y", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flDeltaItemX, "delta_item_x", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDeltaItemXEndPos, "delta_item_end_x", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBGImageX, "bg_image_x", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBGImageY, "bg_image_y", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBGImageWide, "bg_image_wide", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBGImageTall, "bg_image_tall", "0", "proportional_float" );

	CPanelAnimationVar( Color, m_DeltaPositiveColor, "PositiveColor", "0 255 0 255" );
	CPanelAnimationVar( Color, m_DeltaNegativeColor, "NegativeColor", "255 0 0 255" );

	CPanelAnimationVar( float, m_flDeltaLifetime, "delta_lifetime", "2.0" );

	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFont, "delta_item_font", "Default" );
	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFontBig, "delta_item_font_big", "Default" );
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudAccountPanel : public CHudElement, public CAccountPanel
{
	DECLARE_CLASS_SIMPLE( CHudAccountPanel, CAccountPanel );

public:
	CHudAccountPanel( const char *pElementName ) 
		: CHudElement( pElementName )
		, CAccountPanel( NULL, pElementName )
	{
		Panel *pParent = g_pClientMode->GetViewport();
		SetParent( pParent );
		SetHiddenBits( HIDEHUD_MISCSTATUS );
		ListenForGameEvent( "player_account_changed" );
	}

	virtual void LevelInit( void ) OVERRIDE
	{
		CHudElement::LevelInit();
	}

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE
	{
		const char * type = event->GetName();

		if ( Q_strcmp(type, "player_account_changed") == 0 )
		{
			int iOldValue = event->GetInt( "old_account" );
			int iNewValue = event->GetInt( "new_account" );
			account_delta_t::eAccountDeltaType_t deltaType = ( iNewValue - iOldValue >= 0 ) ? account_delta_t::ACCOUNT_DELTA_HEALING
																							: account_delta_t::ACCOUNT_DELTA_DAMAGE;

			OnAccountValueChanged( iOldValue, iNewValue, deltaType );
		}
		else
		{
			CHudElement::FireGameEvent( event );
		}
	}

	virtual bool ShouldDraw( void ) OVERRIDE
	{
		C_RTBMP_Player *pPlayer = C_RTBMP_Player::GetLocalRTBMPPlayer();
		if ( !pPlayer || !pPlayer->IsAlive() )
		{
			m_AccountDeltaItems.RemoveAll();
			return false;
		}

		// more conditions here

		return CHudElement::ShouldDraw();
	}
};

DECLARE_HUDELEMENT( CHudAccountPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDamageAccountPanel : public CHudAccountPanel
{
	DECLARE_CLASS_SIMPLE( CDamageAccountPanel, CHudAccountPanel );
public:
	CDamageAccountPanel( const char *pElementName ) : CHudAccountPanel(pElementName)
	{
		ListenForGameEvent( "player_hurt" );
		ListenForGameEvent( "npc_hurt" );

		ResetDamageVars();
		vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
	}

	virtual void OnTick( void );
	virtual const char *GetResFileName( void ) { return "resource/UI/HudDamageAccount.res"; }
	virtual void Paint( void );


	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	bool ShouldDrawDPSMeter( void )
	{
		return hud_damagemeter.GetBool();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void DisplayDamageFeedback( CRTBMP_Player *pAttacker, CBaseCombatCharacter *pVictim, int iDamage, int iHealth, bool bIsCrit )
	{
		if ( iDamage <= 0 ) // zero value (invuln?)
			return;

		CRTBMP_Player *pLocalPlayer = C_RTBMP_Player::GetLocalRTBMPPlayer();
		if ( !pLocalPlayer || !pLocalPlayer->IsAlive() )
			return;

		if ( !pAttacker || !pVictim )
			return;

		// Show the attacker, or when healing the player that is
		if ( pAttacker == pLocalPlayer )
		{
			C_RTBMP_Player *pVictimPlayer = ToRTBMPPlayer( pVictim );
			if ( pVictimPlayer )
			{
				// Player hurt self
				if ( pAttacker == pVictimPlayer )
					return;
			}

			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "DamagedPlayer" );

			if ( hud_combattext.GetBool() )
			{
				// Ignore damage events on targets that we can't see, so it's not a cheat
				trace_t	tr;
				UTIL_TraceLine( pVictim->WorldSpaceCenter(), MainViewOrigin(), MASK_SOLID_BRUSHONLY & ~CONTENTS_GRATE, NULL, COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction >= 1.f )
				{
					account_delta_t *pNewAccount = OnAccountValueChanged( 0, -iDamage, account_delta_t::ACCOUNT_DELTA_DAMAGE );
					if ( pNewAccount )
					{
						Vector vecPos = pVictim->GetAbsOrigin();
						Vector vecDistance = vecPos - pLocalPlayer->GetAbsOrigin();
						int nHeightoffset = RemapValClamped( vecDistance.LengthSqr(), 0.0f, (200.0f * 200.0f), 1, 8 );
						vecPos.z += (VEC_HULL_MAX_SCALED( pVictim ).z + nHeightoffset);
						pNewAccount->m_nX = vecPos.x;
						pNewAccount->m_nXEnd = pNewAccount->m_nX;
						pNewAccount->m_nY = vecPos.y;
						pNewAccount->m_nHStart = vecPos.z;
						pNewAccount->m_nHEnd = pNewAccount->m_nHStart + 32;	// How many units to float up
						pNewAccount->m_bWorldSpace = true;
						pNewAccount->m_nSourceID = pVictim->entindex();
						pNewAccount->m_flBatchWindow = hud_combattext_batching.GetBool() ? hud_combattext_batching_window.GetFloat() : 0.f;
						pNewAccount->m_bLargeFont = bIsCrit;
						//	V_swprintf_safe( pNewAccount->m_wzText, L" (%d)", m_nQueuedDamageEvents );
					}
				}
			}

			m_flLastDamageEventTime = gpGlobals->curtime;

			// Damage meter tracking
			if ( hud_damagemeter_period.GetFloat() > 0.f )
			{
				// Store events and average across a sliding window
				DamageHistory_t damage = { (float)iDamage, m_flLastDamageEventTime };
				m_DamageHistory.AddToTail( damage );
			}
			else
			{
				// Running tally until we hit the out-of-combat timer
				if ( m_flFirstDamageEventTime == 0.f )
				{
					m_flFirstDamageEventTime = m_flLastDamageEventTime;
					m_flDamagePerSecond = 0.f;
					m_flDamageMeterTotal = 0.f;
				}
				
				m_flDamageMeterTotal += iDamage;
			}

			// play hit/killsound
			CLocalPlayerFilter filter;

			if ( cl_hitsound.GetBool() && iHealth > 0 )
			{
				if ( iDamage >= HEAVY_DAMAGE_THRESHOLD )
				{
					pLocalPlayer->EmitSound( filter, pLocalPlayer->entindex(), "Player.HitSound_Heavy" );
				}
				else
				{
					pLocalPlayer->EmitSound( filter, pLocalPlayer->entindex(), "Player.HitSound_Light" );
				}
			}
			else if ( cl_killsound.GetBool() && iHealth <= 0 )
			{
				pLocalPlayer->EmitSound( filter, pLocalPlayer->entindex(), "Player.KillSound" );
			}
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void FireGameEvent( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_hurt" ) )
		{
			int iDamage; 
			if ( hud_combattext_type.GetInt() == 0 )
				iDamage = event->GetInt( "damageamount_base" );
			else
				iDamage = event->GetInt( "damageamount_true" );

			const int iHealth = event->GetInt( "health" );

			const int iAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
			C_RTBMP_Player *pAttacker = ToRTBMPPlayer( UTIL_PlayerByIndex( iAttacker ) );

			const int iVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			C_RTBMP_Player *pVictim = ToRTBMPPlayer( UTIL_PlayerByIndex( iVictim ) );

			const bool bHeadshot = event->GetInt( "headshot" );

			DisplayDamageFeedback( pAttacker, pVictim, iDamage, iHealth, bHeadshot );
		}
		else if ( FStrEq( event->GetName(), "npc_hurt" ) )
		{
			int iDamage = event->GetInt( "damageamount" );

			const int iHealth = event->GetInt( "health" );

			const int iAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
			C_RTBMP_Player *pAttacker = ToRTBMPPlayer( UTIL_PlayerByIndex( iAttacker ) );

			C_BaseCombatCharacter *pVictim = ToBaseCombatCharacter( ClientEntityList().GetBaseEntity( event->GetInt( "entindex" ) ) );

			const bool bHeadshot = event->GetInt( "headshot" );

			DisplayDamageFeedback( pAttacker, pVictim, iDamage, iHealth, bHeadshot );
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	bool ShouldDraw( void )
	{
		C_RTBMP_Player *pPlayer = C_RTBMP_Player::GetLocalRTBMPPlayer();
		if ( !pPlayer || !pPlayer->IsAlive() )
		{
			m_AccountDeltaItems.RemoveAll();
		}

		if ( ShouldDrawDPSMeter() )
			return true;

		if ( !m_AccountDeltaItems.Count() )
			return false;

		return CHudElement::ShouldDraw();
	}

	//-----------------------------------------------------------------------------
	// Purpose: called whenever a new level is starting
	//-----------------------------------------------------------------------------
	virtual void LevelInit( void ) OVERRIDE
	{
		ResetDamageVars();

		BaseClass::LevelInit();
	}

private:

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void ResetDamageVars( void )
	{
		m_flFirstDamageEventTime = 0.f;
		m_flLastDamageEventTime = 0.f;
		m_flDamagePerSecond = 0.f;
		m_flDamageMeterTotal = 0.f;
	}

private:

	// DamageMeter
	float				m_flFirstDamageEventTime;
	float				m_flLastDamageEventTime;
	float				m_flDamagePerSecond;
	float				m_flDamageMeterTotal;
	struct DamageHistory_t
	{
		float flDamage;
		float flDamageTime;
	};
	CUtlVector< DamageHistory_t >	m_DamageHistory;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDamageAccountPanel::OnTick( void )
{
	if ( ShouldDrawDPSMeter() )
	{
		// We're out of combat - nuke everything
		if ( m_flFirstDamageEventTime != 0.f && m_flLastDamageEventTime < gpGlobals->curtime - hud_damagemeter_ooctimer.GetFloat() )
		{
			if ( hud_damagemeter_report.GetBool() )
			{
				DevMsg( "-------\n" );
				DevMsg( "%3.2f DPS over %3.2f seconds\n" , m_flDamagePerSecond, ( m_flLastDamageEventTime - m_flFirstDamageEventTime ) );
				DevMsg( "-------\n" );
			}
			m_DamageHistory.RemoveAll();
			m_flFirstDamageEventTime = 0.f;
		}
		else
		{
			float flPeriod = hud_damagemeter_period.GetFloat();
			
			// Period-based calculation (averaged across a defined range)
			if ( flPeriod > 0.f )
			{
				m_flDamageMeterTotal = 0.f;

				FOR_EACH_VEC_BACK( m_DamageHistory, i )
				{
					if ( flPeriod > 0.f )
					{
						// This method averages across a fixed period, so nuke entires outside the period
						if ( gpGlobals->curtime - flPeriod > m_DamageHistory[i].flDamageTime )
						{
							m_DamageHistory.Remove( i );
							continue;
						}
					}

					// What's left is within the period (sliding window)
					m_flDamageMeterTotal += m_DamageHistory[i].flDamage;
				}
				
				m_flDamagePerSecond = m_flDamageMeterTotal / flPeriod;
			}
			// Event-based calculation (absolute dps)
			else if ( m_flFirstDamageEventTime > 0.f )
			{
				flPeriod = Max( m_flLastDamageEventTime - m_flFirstDamageEventTime, 0.01f );
				m_flDamagePerSecond = m_flDamageMeterTotal / flPeriod;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDamageAccountPanel::Paint( void )
{
	BaseClass::Paint();

	if ( ShouldDrawDPSMeter() )
	{
		int iScreenWide, iScreenTall;
		GetHudSize( iScreenWide, iScreenTall );
		int nX = iScreenWide / 1.15;
		int nY = iScreenTall / 1.20;

		int r = 255, g = 255, b = 255;
		if ( m_flLastDamageEventTime < gpGlobals->curtime - hud_damagemeter_ooctimer.GetFloat() )
		{
			r = 255, g = 0, b = 0;
		}
		Color cDPS( r, g, b, 255 );

		vgui::surface()->DrawSetTextFont( m_hDeltaItemFontBig );
		vgui::surface()->DrawSetTextColor( cDPS );
		vgui::surface()->DrawSetTextPos( nX, nY );

		wchar_t wDPSBuf[20];
		V_swprintf_safe( wDPSBuf, L"%d DPS", (int)m_flDamagePerSecond );
		vgui::surface()->DrawPrintText( wDPSBuf, wcslen( wDPSBuf ), FONT_DRAW_NONADDITIVE );
	}
}

DECLARE_HUDELEMENT( CDamageAccountPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAccountPanel::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( GetResFileName() );

	BaseClass::ApplySchemeSettings( pScheme );
}

void CAccountPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	// Backwards compatibility.  If we DONT find "delta_item_end_x" specified in the keyvalues,
	// then just take the starting x-pos as the ending x-pos.
	if ( inResourceData->FindKey( "delta_item_end_x" ) == NULL )
	{
		m_flDeltaItemXEndPos = m_flDeltaItemX;
	}

	m_bNegativeFlipDir = inResourceData->FindKey( "negative_flip_dir", false );

	const char *pszBGTextureName = inResourceData->GetString( "bg_texture", NULL );
	if ( m_nBGTexture == -1 && pszBGTextureName && pszBGTextureName[0] )
	{
		m_nBGTexture = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_nBGTexture , pszBGTextureName, true, false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
account_delta_t *CAccountPanel::OnAccountValueChanged( int iOldValue, int iNewValue, account_delta_t::eAccountDeltaType_t type )
{
	// update the account value
	SetDialogVariable( "metal", iNewValue ); 

	int iDelta = iNewValue - iOldValue;

	C_RTBMP_Player *pPlayer = C_RTBMP_Player::GetLocalRTBMPPlayer();
	if ( iDelta != 0 && pPlayer && pPlayer->IsAlive() )
	{
		int index = m_AccountDeltaItems.AddToTail();
		account_delta_t *pNewDeltaItem = &m_AccountDeltaItems[index];

		pNewDeltaItem->m_flDieTime = gpGlobals->curtime + m_flDeltaLifetime;
		pNewDeltaItem->m_iAmount = iDelta;
		pNewDeltaItem->m_nX = m_flDeltaItemX;
		pNewDeltaItem->m_nXEnd = m_flDeltaItemXEndPos;
		pNewDeltaItem->m_nHStart = m_flDeltaItemStartPos;
		pNewDeltaItem->m_nHEnd = m_flDeltaItemEndPos;
		pNewDeltaItem->m_bWorldSpace = false;
		pNewDeltaItem->m_nSourceID = -1;
		pNewDeltaItem->m_flBatchWindow = 0.f;
		pNewDeltaItem->m_bLargeFont = false;
		pNewDeltaItem->m_eDataType = type;
		pNewDeltaItem->m_wzText[0] = NULL;
		pNewDeltaItem->m_color = GetColor( type, iDelta );
		pNewDeltaItem->m_bShadows = false;
		return &m_AccountDeltaItems[index];
	}

	return NULL;
}

Color CAccountPanel::GetColor( const account_delta_t::eAccountDeltaType_t &type, const int iDeltaValue )
{
	if ( type == account_delta_t::ACCOUNT_DELTA_HEALING )
	{
		return ( iDeltaValue < 0 ) ? m_DeltaNegativeColor : m_DeltaPositiveColor;
	}
	else if ( type == account_delta_t::ACCOUNT_DELTA_DAMAGE )
	{
		return Color( hud_combattext_red.GetInt(), hud_combattext_green.GetInt(), hud_combattext_blue.GetInt() );
	}
	
	return Color( 255, 255, 255, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: Paint the deltas
//-----------------------------------------------------------------------------
void CAccountPanel::Paint( void )
{
	BaseClass::Paint();

	FOR_EACH_VEC_BACK( m_AccountDeltaItems, i )
	{
		// Reduce lifetime when count grows too high
		float flTimeMod = m_AccountDeltaItems.Count() > NUM_ACCOUNT_DELTA_ITEMS ? RemapValClamped( m_AccountDeltaItems.Count(), 10.f, 15.f, 0.5f, 1.5f ) : 0.f;

		// update all the valid delta items
		if ( ( m_AccountDeltaItems[i].m_flDieTime - flTimeMod ) > gpGlobals->curtime )
		{
			// position and alpha are determined from the lifetime
			Color c = m_AccountDeltaItems[i].m_color;

			float flLifetimePercent = ( m_flDeltaLifetime - ( m_AccountDeltaItems[i].m_flDieTime - gpGlobals->curtime ) ) / m_flDeltaLifetime;
			// fade out after half our lifetime
			int nAlpha = flLifetimePercent > 0.5 ? (int)( 255.0f * ( ( 0.5f - flLifetimePercent ) / 0.5f ) ) : 255;
			c[3] = nAlpha;

			// Some items want to be batched together as they're super frequent (i.e. damage events from a flamethrower, or minigun)
			if ( m_AccountDeltaItems[i].m_flBatchWindow > 0.f && m_AccountDeltaItems[i].m_nSourceID != -1 && m_AccountDeltaItems.IsValidIndex( i - 1 ) )
			{
				// If next item is from the same source and too close, merge
				float flDelay = m_AccountDeltaItems[i].m_flBatchWindow;
				if ( m_AccountDeltaItems[i].m_flDieTime - m_AccountDeltaItems[i-1].m_flDieTime <= flDelay &&
					 m_AccountDeltaItems[i-1].m_nSourceID == m_AccountDeltaItems[i].m_nSourceID )
				{
					m_AccountDeltaItems[i].m_iAmount += m_AccountDeltaItems[i-1].m_iAmount;
					m_AccountDeltaItems.Remove( i - 1 );
					continue;
				}
			}

			float flHeight = m_AccountDeltaItems[i].m_nHEnd - m_AccountDeltaItems[i].m_nHStart;
			float flWidth = m_AccountDeltaItems[i].m_nXEnd - m_AccountDeltaItems[i].m_nX;

			// We can be told to go the opposite direction if we're negative
			if ( m_bNegativeFlipDir && m_AccountDeltaItems[i].m_iAmount < 0 )
			{
				flHeight = -flHeight;
				flWidth = -flWidth;
			}

			float flYPos = m_AccountDeltaItems[i].m_nHStart + ( flLifetimePercent * flHeight );
			float flXPos = m_AccountDeltaItems[i].m_nX + ( flLifetimePercent * flWidth );
			if ( m_AccountDeltaItems[i].m_bWorldSpace )
			{
				Vector vecWorld( m_AccountDeltaItems[i].m_nX, m_AccountDeltaItems[i].m_nY, flYPos );
				int iX,iY;
				if ( !GetVectorInHudSpace( vecWorld, iX, iY ) )				// Tested - NOT GetVectorInScreenSpace
					continue;

				flXPos = iX;
				flYPos = iY;
			}

			// If we have a background texture, then draw it!
			if ( m_nBGTexture != -1 )
			{
				vgui::surface()->DrawSetColor(255,255,255,nAlpha);
				vgui::surface()->DrawSetTexture(m_nBGTexture);
				vgui::surface()->DrawTexturedRect( flXPos + m_flBGImageX, flYPos + m_flBGImageY, flXPos + m_flBGImageX + m_flBGImageWide, flYPos + m_flBGImageY + m_flBGImageTall );
			}

			wchar_t wBuf[20];

			if ( m_AccountDeltaItems[i].m_iAmount > 0 )
			{
				V_swprintf_safe( wBuf, L"+%d", m_AccountDeltaItems[i].m_iAmount );
			}
			else
			{
				V_swprintf_safe( wBuf, L"%d", m_AccountDeltaItems[i].m_iAmount );
			}

			// Append?
			if ( m_AccountDeltaItems[i].m_wzText[0] )
			{
				wchar_t wAppend[8] = { 0 };
				V_swprintf_safe( wAppend, L"%ls", m_AccountDeltaItems[i].m_wzText );
				V_wcscat_safe( wBuf, wAppend );
			}


			if ( m_AccountDeltaItems[i].m_bLargeFont )
			{
				vgui::surface()->DrawSetTextFont( m_hDeltaItemFontBig );
			}
			else
			{
				vgui::surface()->DrawSetTextFont( m_hDeltaItemFont );
			}

			// If we're supposed to have shadows, then draw the text as black and offset a bit first.
			// Things get ugly as we approach 0 alpha, so stop drawing the shadow a bit early.
			if ( m_AccountDeltaItems[i].m_bShadows && c[3] > 10 )
			{
				vgui::surface()->DrawSetTextPos( (int)flXPos + XRES(1), (int)flYPos + YRES(1) );
				vgui::surface()->DrawSetTextColor( COLOR_BLACK );
				vgui::surface()->DrawPrintText( wBuf, wcslen(wBuf), FONT_DRAW_NONADDITIVE );
			}

			vgui::surface()->DrawSetTextPos( (int)flXPos, (int)flYPos );
			vgui::surface()->DrawSetTextColor( c );
			vgui::surface()->DrawPrintText( wBuf, wcslen(wBuf), FONT_DRAW_NONADDITIVE );
		}
		else
		{
			m_AccountDeltaItems.Remove( i );
		}
	}
}

