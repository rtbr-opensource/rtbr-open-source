//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "c_rtbmp_player.h"
#include "iclientmode.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include "rtbmp_gamerules.h"
#include "vgui_entitypanel.h"
#include "c_hl2mp_playerresource.h"
#include "c_team.h"
#include "hud_macros.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include <game/client/iviewport.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar fraglimit;

// todo: should probably merge the hl2mp scoreboard version and this into a single header file
struct PlayerScore_t
{
	int id;
	int frags;
	int deaths;
};

//-----------------------------------------------------------------------------
// Purpose: Shows the time left, score breakdown, etc.
//-----------------------------------------------------------------------------
class CHudMatchStatus : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudMatchStatus, EditablePanel );

public:
	CHudMatchStatus( const char *pElementName );

	virtual void	Init( void ) OVERRIDE;
	virtual void	LevelInit( void ) OVERRIDE;
	virtual bool	ShouldDraw( void ) OVERRIDE;

	virtual void	ApplySchemeSettings( IScheme *scheme ) OVERRIDE;
	virtual void	OnThink() OVERRIDE;
	virtual void	Paint() OVERRIDE;
	virtual void	PerformLayout() OVERRIDE;
	void			SetupDeathmatchText();
	void			SetupCoOpText();
	void			PopulateStatusLines( const CUtlVector<wchar_t *> &lines );
	void			ResetObjectives();
	void			PulseObjective();

	static int PlayerScoreSortFunc( const PlayerScore_t *p1, const PlayerScore_t *p2 );

	virtual const char *GetResFileName( void ) { return "Resource/UI/HudMatchStatus.res"; }

	void	MsgFunc_ShowObjectives( bf_read &msg );
	void	MsgFunc_ObjectiveChanged( bf_read &msg );
	void	MsgFunc_WaveChanged( bf_read &msg );
	void	MsgFunc_EnemyCountChanged( bf_read &msg );
	void	MsgFunc_MatchStatusUpdate( bf_read &msg );

protected:

private:
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVar( vgui::HFont, m_hTextGlowFont, "TextGlowFont", "DefaultGlow" );
	CPanelAnimationVar( float, m_flBlur, "Blur", "0" );
	vgui::Label *m_pStatusLine1;
	vgui::Label *m_pStatusLine2;
	vgui::Label *m_pStatusLine3;
	vgui::Label *m_pStatusLine4;

	// deathmatch objectives
	int			m_iScore;

	// co-op objectives
	bool		m_bShowMatchStatus;
	char		m_szObjective[128];
	int			m_iCurrentWave;
	int			m_iMaxWaves;
	int			m_iEnemyCount;
};

DECLARE_HUDELEMENT( CHudMatchStatus );
DECLARE_HUD_MESSAGE( CHudMatchStatus, ShowObjectives );
DECLARE_HUD_MESSAGE( CHudMatchStatus, ObjectiveChanged );
DECLARE_HUD_MESSAGE( CHudMatchStatus, WaveChanged );
DECLARE_HUD_MESSAGE( CHudMatchStatus, EnemyCountChanged );
DECLARE_HUD_MESSAGE( CHudMatchStatus, MatchStatusUpdate );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudMatchStatus::CHudMatchStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMatchStatus" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MATCHSTATUS | HIDEHUD_HEALTH );

	m_pStatusLine1 = new vgui::Label( this, "StatusLine1", "" );
	m_pStatusLine1->SetPaintBackgroundEnabled( false );
	m_pStatusLine1->SetPaintBorderEnabled( false );
	m_pStatusLine1->SizeToContents();
	m_pStatusLine1->SetContentAlignment( vgui::Label::a_center );
	m_pStatusLine1->SetFgColor( GetFgColor() );

	m_pStatusLine2 = new vgui::Label( this, "StatusLine2", "" );
	m_pStatusLine2->SetPaintBackgroundEnabled( false );
	m_pStatusLine2->SetPaintBorderEnabled( false );
	m_pStatusLine2->SizeToContents();
	m_pStatusLine2->SetContentAlignment( vgui::Label::a_center );
	m_pStatusLine2->SetFgColor( GetFgColor() );

	m_pStatusLine3 = new vgui::Label( this, "StatusLine3", "" );
	m_pStatusLine3->SetPaintBackgroundEnabled( false );
	m_pStatusLine3->SetPaintBorderEnabled( false );
	m_pStatusLine3->SizeToContents();
	m_pStatusLine3->SetContentAlignment( vgui::Label::a_center );
	m_pStatusLine3->SetFgColor( GetFgColor() );

	m_pStatusLine4 = new vgui::Label( this, "StatusLine4", "" );
	m_pStatusLine4->SetPaintBackgroundEnabled( false );
	m_pStatusLine4->SetPaintBorderEnabled( false );
	m_pStatusLine4->SizeToContents();
	m_pStatusLine4->SetContentAlignment( vgui::Label::a_center );
	m_pStatusLine4->SetFgColor( GetFgColor() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMatchStatus::Init( void )
{
	HOOK_HUD_MESSAGE( CHudMatchStatus, ShowObjectives );
	HOOK_HUD_MESSAGE( CHudMatchStatus, ObjectiveChanged );
	HOOK_HUD_MESSAGE( CHudMatchStatus, WaveChanged );
	HOOK_HUD_MESSAGE( CHudMatchStatus, EnemyCountChanged );
	HOOK_HUD_MESSAGE( CHudMatchStatus, MatchStatusUpdate );

	ResetObjectives();

	Reset();
}

void CHudMatchStatus::LevelInit( void )
{
	CHudElement::LevelInit();

	ResetObjectives();
}

bool CHudMatchStatus::ShouldDraw( void )
{
	C_RTBMP_Player *pPlayer = C_RTBMP_Player::GetLocalRTBMPPlayer();
	if ( !pPlayer )
	{
		return false;
	}

	if ( RTBMPRules() && RTBMPRules()->IsCoOp() && !m_bShowMatchStatus )
		return false;

	IViewPortPanel *pScoreboard = gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD );
	if ( pScoreboard && pScoreboard->IsVisible() )
		return false;

	IViewPortPanel *pTeamMenu = gViewPortInterface->FindPanelByName( PANEL_TEAM );
	if ( pTeamMenu && pTeamMenu->IsVisible() )
		return false;

	CHudElement *pWeaponSelection = gHUD.FindElement( "CHudWeaponSelection" );
	if ( pWeaponSelection && pWeaponSelection->IsActive() )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMatchStatus::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( GetResFileName() );

	BaseClass::ApplySchemeSettings( pScheme );
}

void CHudMatchStatus::OnThink()
{
	C_RTBMP_Player *pPlayer = C_RTBMP_Player::GetLocalRTBMPPlayer();
	if ( !pPlayer )
		return;

	SetPaintBorderEnabled( true );
	SetPaintBackgroundEnabled( false );

	if ( RTBMPRules()->IsDeathmatch() )
		SetupDeathmatchText();
	else
		SetupCoOpText();

	SetVisible( true );

	InvalidateLayout();
}

void CHudMatchStatus::SetupDeathmatchText( void )
{
	C_RTBMP_Player *pPlayer = C_RTBMP_Player::GetLocalRTBMPPlayer();

	CUtlVector<wchar_t *> statusLineText;

	wchar_t wszTimeLeft[20];
	int iTimeRemaining = (int)HL2MPRules()->GetMapRemainingTime();

	if ( iTimeRemaining > 0 )
	{
		int iMinutes, iSeconds;
		iMinutes = iTimeRemaining / 60;
		iSeconds = iTimeRemaining % 60;

		wchar_t wszMinutes[4];
		wchar_t wszSeconds[4];

		_snwprintf( wszMinutes, ARRAYSIZE( wszMinutes ), L"%d", iMinutes );
		_snwprintf( wszSeconds, ARRAYSIZE( wszSeconds ), L"%2.2d", iSeconds );

		g_pVGuiLocalize->ConstructString_safe( wszTimeLeft, g_pVGuiLocalize->Find( "#RTBA_MatchStatus_TimeLeft" ), 2, wszMinutes, wszSeconds );

		statusLineText.AddToTail( MAKE_STRING( wszTimeLeft ) );
	}

	int iMyScore = 0;
	int iRank = 0;
	int iTheirHighestScore = 0;

	// gather points
	if ( RTBMPRules()->IsTeamplay() )
	{
		int iMyTeam = GetLocalPlayerTeam();
		int iTheirTeam = iMyTeam == TEAM_COMBINE ? TEAM_REBELS : TEAM_COMBINE;

		iMyScore = GetGlobalTeam( iMyTeam )->Get_Score();
		iTheirHighestScore = GetGlobalTeam( iTheirTeam )->Get_Score();
	}
	else
	{
		CUtlVector<PlayerScore_t> scores;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			if ( !g_HL2MP_PR->IsConnected( i ) )
				continue;

			int iCurrScore = g_HL2MP_PR->GetPlayerScore( i );
			if ( UTIL_PlayerByIndex( i ) == pPlayer )
			{
				iMyScore = iCurrScore;
			}

			PlayerScore_t score;
			score.id = i;
			score.frags = iCurrScore;
			score.deaths = g_HL2MP_PR->GetDeaths( i );
			scores.AddToTail( score );
		}

		if ( scores.Count() > 1 )
		{
			// This is pretty disgusting considering we run this code every frame.
			// However, to run it on an event becomes messy due to how events are fired, so we're going with this.
			scores.Sort( PlayerScoreSortFunc );

			for ( int i = 0; i < scores.Count(); i++ )
			{
				int iCurrId = scores[i].id;
				int iCurrFrags = scores[i].frags;
				int iCurrDeaths = scores[i].deaths;
				if ( iTheirHighestScore == 0 && UTIL_PlayerByIndex( iCurrId ) != pPlayer )
					iTheirHighestScore = iCurrFrags;

				if ( UTIL_PlayerByIndex( iCurrId ) == pPlayer )
				{
					// determine our rank - first check if we're at the top
					if ( i == 0 )
					{
						iRank = 1;
					}
					else
					{
						// then check if we're tied with anyone
						iRank = i + 1;
						int j = i - 1;
						while ( j > 0 && scores[j].frags == iCurrFrags && scores[j].deaths == iCurrDeaths )
						{
							iRank = j-- + 1;
						}
					}
				}
			}
		}
		else
		{
			iRank = 1;
			iTheirHighestScore = iMyScore;
		}
	}

	if ( iMyScore > m_iScore )
		PulseObjective();

	m_iScore = iMyScore;

	wchar_t wszScore[16];
	const char *szFormatString = iMyScore == 1 ? "#RTBA_MatchStatus_Point" : "#RTBA_MatchStatus_Points";

	wchar_t wszMyScore[4];
	_snwprintf( wszMyScore, ARRAYSIZE( wszMyScore ), L"%d", iMyScore );

	g_pVGuiLocalize->ConstructString_safe( wszScore, g_pVGuiLocalize->Find( szFormatString ), 1, wszMyScore );
	statusLineText.AddToTail( MAKE_STRING( wszScore ) );

	wchar_t wszRank[32];

	// gather rank and difference
	if ( RTBMPRules()->IsTeamplay() )
	{
		int iMyTeam = GetLocalPlayerTeam();
		int iTheirTeam = iMyTeam == TEAM_COMBINE ? TEAM_REBELS : TEAM_COMBINE;

		const char *szTheirTeamName = GetGlobalTeam( iTheirTeam )->Get_Name();
		wchar_t wszTheirTeamName[10];
		g_pVGuiLocalize->ConvertANSIToUnicode( szTheirTeamName, wszTheirTeamName, sizeof( wszTheirTeamName ) );

		int iScoreDifference = iMyScore - iTheirHighestScore;
		wchar_t wszScoreDifference[4];
		_snwprintf( wszScoreDifference, ARRAYSIZE( wszScoreDifference ), L"%d", abs( iScoreDifference ) );
		

		if ( iScoreDifference == 0 )
		{
			g_pVGuiLocalize->ConstructString_safe( wszRank, g_pVGuiLocalize->Find( "#RTBA_MatchStatus_Team_Tied" ), 1, wszTheirTeamName );
		}
		else if ( iScoreDifference > 0 )
		{
			g_pVGuiLocalize->ConstructString_safe( wszRank, g_pVGuiLocalize->Find( "#RTBA_MatchStatus_Team_Leading" ), 2, wszTheirTeamName, wszScoreDifference );
		}
		else
		{
			g_pVGuiLocalize->ConstructString_safe( wszRank, g_pVGuiLocalize->Find( "#RTBA_MatchStatus_Team_Trailing" ), 2, wszTheirTeamName, wszScoreDifference );
		}
	}
	else
	{
		// construct our ordinal rank
		wchar_t wszOrd[6];
		char *szFormatString = "#RTBA_MatchStatus_Rank_4th";
		if ( iRank / 10 != 1 )
		{
			switch ( iRank % 10 )
			{
			case 1:
				szFormatString = "#RTBA_MatchStatus_Rank_1st";
				break;
			case 2:
				szFormatString = "#RTBA_MatchStatus_Rank_2nd";
				break;
			case 3:
				szFormatString = "#RTBA_MatchStatus_Rank_3rd";
				break;
			}
		}

		wchar_t wszNumRank[3];
		_snwprintf( wszNumRank, ARRAYSIZE( wszNumRank ), L"%d", iRank );

		g_pVGuiLocalize->ConstructString_safe( wszOrd, g_pVGuiLocalize->Find( szFormatString ), 1, wszNumRank );

		// display our points difference
		int iScoreDifference = iMyScore - iTheirHighestScore;
		wchar_t wszScoreDifference[4];
		_snwprintf( wszScoreDifference, ARRAYSIZE( wszScoreDifference ), L"%d", abs( iScoreDifference ) );

		wchar_t wszLead[25];
		if ( iScoreDifference == 0 )
		{
			_snwprintf( wszLead, ARRAYSIZE( wszLead ), g_pVGuiLocalize->Find( "#RTBA_MatchStatus_DM_Tied" ) );
		}
		else if ( iScoreDifference > 0 )
		{
			g_pVGuiLocalize->ConstructString_safe( wszLead, g_pVGuiLocalize->Find( "#RTBA_MatchStatus_DM_Leading" ), 1, wszScoreDifference );
		}
		else
		{
			g_pVGuiLocalize->ConstructString_safe( wszLead, g_pVGuiLocalize->Find( "#RTBA_MatchStatus_DM_Trailing" ), 1, wszScoreDifference );
		}

		// combine it all together
		_snwprintf( wszRank, ARRAYSIZE( wszRank ), L"%s | %s", wszOrd, wszLead );
	}

	statusLineText.AddToTail( MAKE_STRING( wszRank ) );

	int iFragLimit = fraglimit.GetInt();
	wchar_t wszScoreLimit[24];

	if ( iFragLimit > 0 )
	{
		wchar_t wszFragLimit[4];
		_snwprintf( wszFragLimit, ARRAYSIZE( wszFragLimit ), L"%d", iFragLimit );

		g_pVGuiLocalize->ConstructString_safe( wszScoreLimit, g_pVGuiLocalize->Find( "#RTBA_MatchStatus_ScoreLimit" ), 1, wszFragLimit );
		statusLineText.AddToTail( MAKE_STRING( wszScoreLimit ) );
	}

	PopulateStatusLines( statusLineText );
}

void CHudMatchStatus::SetupCoOpText( void )
{
	CUtlVector<wchar_t *> statusLineText;

	wchar_t wszObjective[128];

	if ( !FStrEq( m_szObjective, "" ) )
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( m_szObjective, wszObjective, sizeof( wszObjective ) );
		statusLineText.AddToTail( MAKE_STRING( wszObjective ) );
	}

	wchar_t wszWave[28];

	if ( m_iCurrentWave > 0 && m_iMaxWaves > 0 )
	{
		wchar_t wszCurrentWave[4];
		wchar_t wszMaxWaves[4];
		
		_snwprintf( wszCurrentWave, ARRAYSIZE( wszCurrentWave ), L"%d", m_iCurrentWave );
		_snwprintf( wszMaxWaves, ARRAYSIZE( wszMaxWaves ), L"%d", m_iMaxWaves );

		g_pVGuiLocalize->ConstructString_safe( wszWave, g_pVGuiLocalize->Find( "#RTBA_MatchStatus_CoOp_Wave" ), 2, wszCurrentWave, wszMaxWaves );
		statusLineText.AddToTail( MAKE_STRING( wszWave ) );
	}

	wchar_t wszEnemies[20];

	if ( m_iEnemyCount > 0 )
	{
		wchar_t wszEnemyCount[4];
		_snwprintf( wszEnemyCount, ARRAYSIZE( wszEnemyCount ), L"%d", m_iEnemyCount );
		g_pVGuiLocalize->ConstructString_safe( wszEnemies, g_pVGuiLocalize->Find( "#RTBA_MatchStatus_CoOp_EnemiesLeft" ), 1, wszEnemyCount );
		statusLineText.AddToTail( MAKE_STRING( wszEnemies ) );
	}

	PopulateStatusLines( statusLineText );
}

void CHudMatchStatus::PopulateStatusLines( const CUtlVector<wchar_t *> &lines )
{
	vgui::Label *statusLines[] =
	{
		m_pStatusLine1,
		m_pStatusLine2,
		m_pStatusLine3,
		m_pStatusLine4,
	};

	// populate our status lines top to bottom
	int i;
	for ( i = 0; i < lines.Count(); i++ )
	{
		statusLines[i]->SetText( lines.Element( i ) );
		statusLines[i]->SetVisible( true );
		statusLines[i]->SizeToContents();
	}

	// clear out the ones we haven't populated
	for ( ; i < ARRAYSIZE( statusLines ); i++ )
	{
		statusLines[i]->SetText( "" );
		statusLines[i]->SetVisible( false );
	}
}

void CHudMatchStatus::Paint()
{
	BaseClass::Paint();

	if ( RTBMPRules() )
	{
		vgui::Label *pObjectiveLine = NULL;

		if ( RTBMPRules()->IsDeathmatch() )
		{
			// find our points line
			int iTimeRemaining = (int)HL2MPRules()->GetMapRemainingTime();

			if ( iTimeRemaining > 0 )
			{
				pObjectiveLine = m_pStatusLine2; 
			}
			else
			{
				// if the time remaining line is missing this one will be first
				pObjectiveLine = m_pStatusLine1;
			}
		}
		else if ( RTBMPRules()->IsCoOp() && !FStrEq( m_szObjective, "" ) )
		{
			pObjectiveLine = m_pStatusLine1;
		}

		if ( pObjectiveLine )
		{
			wchar_t objective[128];
			pObjectiveLine->GetText( objective, sizeof( objective ) );

			Color clr = pObjectiveLine->GetFgColor();
			vgui::HFont font = m_hTextFont;
			int xpos, ypos;
			pObjectiveLine->GetPos( xpos, ypos );

			vgui::surface()->DrawSetTextColor( clr );
			vgui::surface()->DrawSetTextFont( font );
			vgui::surface()->DrawSetTextPos( xpos, ypos );

			// draw the overbright blur
			for ( float fl = m_flBlur; fl > 0.0f; fl -= 1.0f )
			{
				if ( fl >= 1.0f )
				{
					vgui::surface()->DrawSetTextFont( m_hTextGlowFont );
					vgui::surface()->DrawSetTextPos( xpos, ypos );
					vgui::surface()->DrawUnicodeString( objective );
				}
				else
				{
					// draw a percentage of the last one
					Color col = clr;
					col[3] *= fl;
					vgui::surface()->DrawSetTextColor( col );
					vgui::surface()->DrawSetTextFont( m_hTextGlowFont );
					vgui::surface()->DrawSetTextPos( xpos, ypos );
					vgui::surface()->DrawUnicodeString( objective );
				}
			}
		}
	}
}

void CHudMatchStatus::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize( wide, tall );

	// find the total height
	int fontTall = vgui::surface()->GetFontTall( m_hTextFont );

	// find the widest line
	int labelWide = m_pStatusLine1->GetWide();
	int xOffset = (wide - labelWide) / 2;
	m_pStatusLine1->SetPos( xOffset, 4 );

	labelWide = m_pStatusLine2->GetWide();
	xOffset = (wide - labelWide) / 2;
	m_pStatusLine2->SetPos( xOffset, 4 + fontTall );

	labelWide = m_pStatusLine3->GetWide();
	xOffset = (wide - labelWide) / 2;
	m_pStatusLine3->SetPos( xOffset, 4 + fontTall * 2 );

	labelWide = m_pStatusLine4->GetWide();
	xOffset = (wide - labelWide) / 2;
	m_pStatusLine4->SetPos( xOffset, 4 + fontTall * 3 );
}

int CHudMatchStatus::PlayerScoreSortFunc( const PlayerScore_t *p1, const PlayerScore_t *p2 )
{
	if ( p1->frags > p2->frags )
		return -1;
	else if ( p1->frags < p2->frags )
		return 1;

	if ( p1->deaths < p2->deaths )
		return -1;
	else if ( p1->deaths > p2->deaths )
		return 1;

	if ( p1->id < p2->id )
		return -1;

	return 1;
}

void CHudMatchStatus::ResetObjectives()
{
	if ( RTBMPRules() )
	{
		if ( RTBMPRules()->IsDeathmatch() )
		{
			m_iScore = 0;
		}
		else
		{
			m_bShowMatchStatus = true;
			Q_strncpy( m_szObjective, "", sizeof( m_szObjective ) );
			m_iCurrentWave = 0;
			m_iEnemyCount = 0;
			m_flBlur = 0;
		}
	}
}

void CHudMatchStatus::PulseObjective()
{
	C_RTBMP_Player *pPlayer = C_RTBMP_Player::GetLocalRTBMPPlayer();

	if ( !pPlayer )
		return;

	if ( !RTBMPRules() )
		return;

	if ( pPlayer->IsAlive() && 
		( RTBMPRules()->IsDeathmatch() || (RTBMPRules()->IsCoOp() && !FStrEq( m_szObjective, "" ) && m_bShowMatchStatus) ) )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ObjectivePulse" );
	}
}

void CHudMatchStatus::MsgFunc_ShowObjectives( bf_read &msg )
{
	m_bShowMatchStatus = (bool)msg.ReadByte();
}

void CHudMatchStatus::MsgFunc_ObjectiveChanged( bf_read &msg )
{
	msg.ReadString( m_szObjective, sizeof( m_szObjective ) );
	PulseObjective();
}

void CHudMatchStatus::MsgFunc_WaveChanged( bf_read &msg )
{
	m_iCurrentWave = msg.ReadShort();
	m_iMaxWaves = msg.ReadShort();
}

void CHudMatchStatus::MsgFunc_EnemyCountChanged( bf_read &msg )
{
	m_iEnemyCount = msg.ReadShort();
}

void CHudMatchStatus::MsgFunc_MatchStatusUpdate( bf_read &msg )
{
	msg.ReadString( m_szObjective, sizeof( m_szObjective ) );
	PulseObjective();

	m_iCurrentWave = msg.ReadShort();
	m_iMaxWaves = msg.ReadShort();

	m_iEnemyCount = msg.ReadShort();

	m_bShowMatchStatus = (bool)msg.ReadByte();
}