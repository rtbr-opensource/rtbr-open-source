//========= Copyright RTBA Team, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "rtba_teammenu.h"
#include "c_playerresource.h"
#include "rtbmp_gamerules.h"
#include "c_rtbmp_player.h"
#include "IGameUIFuncs.h" // for key bindings
#include "c_team.h"
#include <vgui/ILocalize.h>
#include <vgui/IVGui.h>

#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CRTBATeamMenu::CRTBATeamMenu( IViewPort *pViewPort ) : CTeamMenu( pViewPort )
{
	m_iTeamMenuKey = BUTTON_CODE_INVALID;

	m_pAutoAssignButton = FindControl<vgui::Button>( "AutoAssignButton" );
	m_pSpecTeamButton = FindControl<vgui::Button>( "SpecTeamButton" );
	m_pCombineTeamButton = FindControl<vgui::Button>( "CombineTeamButton" );
	m_pRebelTeamButton = FindControl<vgui::Button>( "RebelTeamButton" );
	m_pCancelButton = FindControl<vgui::Button>( "CancelButton" );

	m_pAutoAssignButton->SetPaintBorderEnabled( false );
	m_pSpecTeamButton->SetPaintBorderEnabled( false );
	m_pCombineTeamButton->SetPaintBorderEnabled( false );
	m_pRebelTeamButton->SetPaintBorderEnabled( false );

	m_pCombineCountLabel = FindControl<vgui::Label>( "CombineCountLabel" );
	m_pRebelCountLabel = FindControl<vgui::Label>( "RebelCountLabel" );
	
	m_pCombineCountLabel->SetParent( m_pCombineTeamButton );
	m_pRebelCountLabel->SetParent( m_pRebelTeamButton );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CRTBATeamMenu::~CRTBATeamMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: Update button text and player numbers every frame
//-----------------------------------------------------------------------------
void CRTBATeamMenu::OnThink( void )
{
	if ( RTBMPRules()->IsTeamplay() )
	{
		m_pCombineTeamButton->SetText( GetGlobalTeam( TEAM_COMBINE )->Get_Name() );
		m_pRebelTeamButton->SetText( GetGlobalTeam( TEAM_REBELS )->Get_Name() );
		m_pAutoAssignButton->SetText( g_pVGuiLocalize->Find( "#RTBA_AutoAssign" ) );
			
		wchar_t wNumCombine[6];
		_snwprintf( wNumCombine, ARRAYSIZE( wNumCombine ), L"%i", GetGlobalTeam( TEAM_COMBINE )->Get_Number_Players() );
		m_pCombineCountLabel->SetText( wNumCombine );

		wchar_t wNumRebels[6];
		_snwprintf( wNumRebels, ARRAYSIZE( wNumRebels ), L"%i", GetGlobalTeam( TEAM_REBELS )->Get_Number_Players() );
		m_pRebelCountLabel->SetText( wNumRebels );
	}
	else
	{
		m_pAutoAssignButton->SetText( g_pVGuiLocalize->Find( "#RTBA_JoinGame" ) );
	}
}

void CRTBATeamMenu::PerformLayout( void )
{
	BaseClass::PerformLayout();

	if ( RTBMPRules() && RTBMPRules()->IsTeamplay() )
	{
		m_pCombineTeamButton->SetVisible( true );
		m_pRebelTeamButton->SetVisible( true );

		int panelWide, panelTall, buttonWide, buttonTall;
		GetSize( panelWide, panelTall );
		m_pAutoAssignButton->GetSize( buttonWide, buttonTall );

		// stack the team buttons side-by-side
		int team_xpos = panelWide / 2 - buttonWide;
		int ypos = panelTall / 2 - buttonTall;
		m_pRebelTeamButton->SetPos( team_xpos, ypos );
		m_pCombineTeamButton->SetPos( team_xpos + buttonWide, ypos );

		// stack the other buttons top-to-bottom
		int xpos = (panelWide - buttonWide) / 2;
		m_pAutoAssignButton->SetPos( xpos, ypos + buttonTall );
		m_pSpecTeamButton->SetPos( xpos, ypos + buttonTall * 2 );
		m_pCancelButton->SetPos( xpos, ypos + buttonTall * 3 );
	}
	else if ( RTBMPRules() && !RTBMPRules()->IsTeamplay() )
	{
		m_pCombineTeamButton->SetVisible( false );
		m_pRebelTeamButton->SetVisible( false );
		
		// stack buttons top-to-bottom
		int panelWide, panelTall, buttonWide, buttonTall;
		GetSize( panelWide, panelTall );
		m_pAutoAssignButton->GetSize( buttonWide, buttonTall );

		// centre the auto-assign button
		int xpos = (panelWide - buttonWide) / 2;
		int ypos = panelTall / 2 - buttonTall;

		m_pAutoAssignButton->SetPos( xpos, ypos );
		m_pSpecTeamButton->SetPos( xpos, ypos + buttonTall );
		m_pCancelButton->SetPos( xpos, ypos + buttonTall * 2 );
	}
}

void CRTBATeamMenu::ShowPanel( bool bShow )
{
	BaseClass::ShowPanel( bShow );

	if ( bShow )
	{
		m_iTeamMenuKey = gameuifuncs->GetButtonCodeForBind( "changeteam" );

		// can't set the colour in the constructor since the game resources doesn't exist
		m_pAutoAssignButton->SetDefaultColor( COLOR_WHITE, GameResources()->GetTeamColor( TEAM_UNASSIGNED ) );
		m_pSpecTeamButton->SetDefaultColor( COLOR_WHITE, GameResources()->GetTeamColor( TEAM_SPECTATOR ) );
		m_pCombineTeamButton->SetDefaultColor( COLOR_WHITE, GameResources()->GetTeamColor( TEAM_COMBINE ) );
		m_pRebelTeamButton->SetDefaultColor( COLOR_WHITE, GameResources()->GetTeamColor( TEAM_REBELS ) );

		InvalidateLayout();
	}
}

void CRTBATeamMenu::OnTick()
{
	// update the number of players on each team

// enable or disable buttons based on team limit

	if ( !IsVisible() )
		return;

	CRTBMPRules *pRules = RTBMPRules();

	if ( !pRules || !pRules->IsTeamplay() )
		return;

	C_Team *pCombine = GetGlobalTeam( TEAM_COMBINE );
	C_Team *pRebels = GetGlobalTeam( TEAM_REBELS );

	if ( !pCombine || !pRebels )
		return;

	C_RTBMP_Player *pLocalPlayer = C_RTBMP_Player::GetLocalRTBMPPlayer();

	if ( !pLocalPlayer )
		return;

	int iHeavyTeam, iLightTeam;

	bool bUnbalanced = pRules->AreTeamsUnbalanced( iHeavyTeam, iLightTeam );

	int iCurrentTeam = pLocalPlayer->GetTeamNumber();

	if ( !bUnbalanced )
	{
		m_pCombineTeamButton->SetEnabled( true );
		m_pRebelTeamButton->SetEnabled( true );
	}

	if ( (bUnbalanced && iHeavyTeam == TEAM_COMBINE) ||
		(pRules->WouldChangeUnbalanceTeams( TEAM_COMBINE, iCurrentTeam )))
	{
		m_pCombineTeamButton->SetEnabled( false );
	}

	if ( (bUnbalanced && iHeavyTeam == TEAM_REBELS) ||
		(pRules->WouldChangeUnbalanceTeams( TEAM_REBELS, iCurrentTeam )))
	{
		m_pRebelTeamButton->SetEnabled( false );
	}
}

void CRTBATeamMenu::Paint()
{
	BaseClass::Paint();

	if ( RTBMPRules() && RTBMPRules()->IsTeamplay() )
	{
		// set our button colours
		if ( m_pCombineTeamButton->IsEnabled() )
		{
			m_pCombineTeamButton->SetDefaultColor( COLOR_WHITE, GameResources()->GetTeamColor( TEAM_COMBINE ) );
			m_pCombineCountLabel->SetFgColor( m_pCombineTeamButton->GetButtonDefaultFgColor() );
		}
		else
		{
			Color c = GameResources()->GetTeamColor( TEAM_COMBINE );

			// desaturate this colour, since it's disabled
			for ( int i = 0; i < 4; i++ )
			{
				c[i] *= 0.75f;
			}

			m_pCombineTeamButton->SetDefaultColor( COLOR_GREY, c );
			m_pCombineTeamButton->SetDisabledFgColor1( COLOR_GREY );
			m_pCombineCountLabel->SetFgColor( m_pCombineTeamButton->GetButtonDefaultFgColor() );
		}

		if ( m_pRebelTeamButton->IsEnabled() )
		{
			m_pRebelTeamButton->SetDefaultColor( COLOR_WHITE, GameResources()->GetTeamColor( TEAM_REBELS ) );
			m_pRebelCountLabel->SetFgColor( m_pRebelTeamButton->GetButtonDefaultFgColor() );
		}
		else
		{
			Color c = GameResources()->GetTeamColor( TEAM_REBELS );

			// desaturate this colour, since it's disabled
			for ( int i = 0; i < 4; i++ )
			{
				c[i] *= 0.75f;
			}

			m_pRebelTeamButton->SetDefaultColor( COLOR_GREY, c );
			m_pRebelTeamButton->SetDisabledFgColor1( COLOR_GREY );
			m_pRebelCountLabel->SetFgColor( m_pRebelTeamButton->GetButtonDefaultFgColor() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: chooses and loads the text page to display that describes mapName map
//-----------------------------------------------------------------------------
void CRTBATeamMenu::LoadMapPage( const char *mapName )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRTBATeamMenu::OnKeyCodePressed( vgui::KeyCode code )
{
	if ( (m_iTeamMenuKey != BUTTON_CODE_INVALID && m_iTeamMenuKey == code) ||
		code == KEY_XBUTTON_BACK ||
		code == KEY_XBUTTON_B ||
		code == STEAMCONTROLLER_B )
	{
		C_RTBMP_Player *pLocalPlayer = C_RTBMP_Player::GetLocalRTBMPPlayer();

		if ( pLocalPlayer )
		{
			ShowPanel( false );
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

void CRTBATeamMenu::OnCommand( const char *command )
{
	if ( V_stricmp( command, "vguicancel" ) )
	{
		engine->ClientCmd( command );
	}

	BaseClass::OnCommand( command );
	ShowPanel( false );
	OnClose();
}