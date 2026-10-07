//========= Copyright RTBA Team, All rights reserved. ============//
//
// Purpose:	First time loading options menu for RTB : A.
//
//================================================================//

#include "cbase.h"
#include "firsttimeoptionspanel.h"
#include "clientmode.h"

using namespace vgui;
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Button.h>

// Forward declarations
class CFirstTimeOptionsFrame;

class COptionsContinueButton : public vgui::Button
{
	DECLARE_CLASS_SIMPLE( COptionsContinueButton, vgui::Button );

	COptionsContinueButton( vgui::Panel *pParent, const char *pName, const char *pText )
		: BaseClass( pParent, pName, pText )
	{
	}

	virtual void DoClick( void );
};

class CFirstTimeOptionsFrame : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CFirstTimeOptionsFrame, vgui::Frame );

	CFirstTimeOptionsFrame( vgui::VPANEL parent );	// Constructor

	virtual void Close( void );

	void	OpenMenu( void );

	void	SetupDefaultMeleeOptions( void );
	void	SetupDamageTypeOptions( void );

	void	ApplySettings( void );

private:
	Panel	*m_pOptionsPanel;

	// Options and labels
	Label		*m_pDefaultMeleeLabel;
	ComboBox	*m_pDefaultMeleeSelection;

	CheckButton	*m_pRightHandedViewmodels;
	CheckButton *m_pAutoWeaponSwitch;
	CheckButton *m_pImprovedImpactEffectsEnabled;
	CheckButton *m_pHitSoundsEnabled;
	CheckButton *m_pKillSoundsEnabled;
	CheckButton *m_pAlwaysRunEnabled;
	CheckButton *m_pFastWeaponSwitchShowSelectionEnabled;
	CheckButton *m_pDamageNumbersEnabled;
	CheckButton *m_pDamageBatchingEnabled;
	
	Label		*m_pDamageBatchingLabel;
	TextEntry	*m_pDamageBatchingWindowEntry;
	Label		*m_pDamageNumbersTypeLabel;
	ComboBox	*m_pDamageNumbersTypeSelection;

	COptionsContinueButton	*m_pContinueButton;
};

CFirstTimeOptionsFrame::CFirstTimeOptionsFrame( vgui::VPANEL parent )
	: BaseClass( NULL, "FirstTimeOptionsDialog" )
{
	SetParent( parent );

	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	SetProportional( false );
	SetTitleBarVisible( true );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );

	SetScheme( vgui::scheme()->LoadSchemeFromFile( "resource/sourcescheme.res", "SourceScheme" ) );

	LoadControlSettings( "resource/ui/firsttimeoptionspanel.res" );

	m_pOptionsPanel = FindControl<Panel>( "panel_optionspanel" );
	m_pDefaultMeleeLabel = FindControl<Label>( "MeleeOptionsLabel" );
	m_pDefaultMeleeSelection = FindControl<ComboBox>( "MeleeOptions" );
	m_pRightHandedViewmodels = FindControl<CheckButton>( "RightHandedCheckbox" );
	m_pAutoWeaponSwitch = FindControl<CheckButton>( "AutoWeaponSwitchCheckbox" );
	m_pImprovedImpactEffectsEnabled = FindControl<CheckButton>( "ImprovedImpactEffectsCheckbox" );
	m_pHitSoundsEnabled = FindControl<CheckButton>( "EnableHitSoundsCheckbox" );
	m_pKillSoundsEnabled = FindControl<CheckButton>( "EnableKillSoundsCheckbox" );
	m_pAlwaysRunEnabled = FindControl<CheckButton>( "EnableAlwaysRunCheckbox" );
	m_pFastWeaponSwitchShowSelectionEnabled = FindControl<CheckButton>( "EnableFastSwitchShowSelectionCheckbox" );
	m_pDamageNumbersEnabled = FindControl<CheckButton>( "DisplayDamageNumbersCheckbox" );
	m_pDamageBatchingEnabled = FindControl<CheckButton>( "BatchDamageNumbersCheckbox" );
	m_pDamageBatchingLabel = FindControl<Label>( "DamageNumberBatchingLabel" );
	m_pDamageBatchingWindowEntry = FindControl<TextEntry>( "DamageNumberBatchingEntry" );
	m_pDamageNumbersTypeLabel = FindControl<Label>( "DamageNumberTypeLabel" );
	m_pDamageNumbersTypeSelection = FindControl<ComboBox>( "DamageTypeSelection" );

	m_pContinueButton = new COptionsContinueButton( this, "OptionsContinueButton", "#GameUI_Continue" );

	if ( m_pContinueButton )
	{
		m_pContinueButton->SetScheme( GetScheme() );
		m_pContinueButton->SetVisible( true );
		m_pContinueButton->SetEnabled( true );
		m_pContinueButton->SetMouseInputEnabled( true );
		m_pContinueButton->SetKeyBoardInputEnabled( false );

		m_pContinueButton->SetPos( 382, 547 );
	}

	// Setup parents.
	m_pDefaultMeleeLabel->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pDefaultMeleeSelection->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pRightHandedViewmodels->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pAutoWeaponSwitch->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pImprovedImpactEffectsEnabled->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pHitSoundsEnabled->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pKillSoundsEnabled->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pAlwaysRunEnabled->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pFastWeaponSwitchShowSelectionEnabled->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pDamageNumbersEnabled->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pDamageBatchingEnabled->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pDamageBatchingLabel->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pDamageBatchingWindowEntry->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pDamageNumbersTypeLabel->SetParent( m_pOptionsPanel->GetVPanel() );
	m_pDamageNumbersTypeSelection->SetParent( m_pOptionsPanel->GetVPanel() );

	// Setup control data.
	SetupDefaultMeleeOptions();

	m_pRightHandedViewmodels->DoClick();
	m_pAutoWeaponSwitch->DoClick();
	m_pImprovedImpactEffectsEnabled->DoClick();
	m_pAlwaysRunEnabled->DoClick();
	m_pFastWeaponSwitchShowSelectionEnabled->DoClick();
	m_pDamageNumbersEnabled->DoClick();

	m_pDamageBatchingWindowEntry->SetText( "0.2" );
	SetupDamageTypeOptions();
}

void CFirstTimeOptionsFrame::SetupDefaultMeleeOptions( void )
{
	m_pDefaultMeleeSelection->AddItem( "#RTBMP_PMDefault", NULL );
	m_pDefaultMeleeSelection->AddItem( "#RTBMP_Crowbar", NULL );
	m_pDefaultMeleeSelection->AddItem( "#RTBMP_Stunstick", NULL );

	m_pDefaultMeleeSelection->ActivateItem( 0 );
}

void CFirstTimeOptionsFrame::SetupDamageTypeOptions( void )
{
	m_pDamageNumbersTypeSelection->AddItem( "#RTBA_BaseDamage", NULL );
	m_pDamageNumbersTypeSelection->AddItem( "#RTBA_TrueDamage", NULL );

	m_pDamageNumbersTypeSelection->ActivateItem( 0 );
}

void CFirstTimeOptionsFrame::OpenMenu( void )
{
	// Place the panel in the center of the screen.
	int viewportWidth, viewportHeight;
	g_pClientMode->GetViewport()->GetPaintSize( viewportWidth, viewportHeight );

	int topLeftPosX = ( viewportWidth / 2 );
	int topLeftPosY = ( viewportHeight / 2 );
	topLeftPosX -= ( GetWide() / 2 );
	topLeftPosY -= ( GetTall() / 2 );

	SetPos( topLeftPosX, topLeftPosY );
}

void CFirstTimeOptionsFrame::ApplySettings( void )
{
	// Default melee.
	{
		ConVarRef cl_defaultmelee( "cl_defaultmelee" );
		int selectedMelee = m_pDefaultMeleeSelection->GetActiveItem();

		switch( selectedMelee )
		{
		case 0:
			cl_defaultmelee.SetValue( "default" );
			break;
		case 1:
			cl_defaultmelee.SetValue( "crowbar" );
			break;
		case 2:
			cl_defaultmelee.SetValue( "stunstick" );
			break;
		}
	}

	// Handedness.
	{
		ConVarRef cl_righthand( "cl_righthand" );
		cl_righthand.SetValue( m_pRightHandedViewmodels->IsDepressed() );
	}

	// Auto weapon switch.
	{
		ConVarRef cl_autowepswitch( "cl_autowepswitch" );
		cl_autowepswitch.SetValue( m_pAutoWeaponSwitch->IsDepressed() );
	}

	// New impact effects.
	{
		ConVarRef cl_new_impact_effects( "cl_new_impact_effects" );
		cl_new_impact_effects.SetValue( m_pImprovedImpactEffectsEnabled->IsDepressed() );
	}

	// Enable hit sounds.
	{
		ConVarRef cl_hitsound( "cl_hitsound" );
		cl_hitsound.SetValue( m_pHitSoundsEnabled->IsDepressed() );
	}

	// Enable kill sounds.
	{
		ConVarRef cl_killsound( "cl_killsound" );
		cl_killsound.SetValue( m_pKillSoundsEnabled->IsDepressed() );
	}

	// Always run.
	{
		ConVarRef cl_alwaysrun( "cl_alwaysrun" );
		cl_alwaysrun.SetValue( m_pAlwaysRunEnabled->IsDepressed() );
	}

	// Show weapon selection on fast switch.
	{
		ConVarRef hud_fastswitch_showselection( "hud_fastswitch_showselection" );
		hud_fastswitch_showselection.SetValue( m_pFastWeaponSwitchShowSelectionEnabled->IsDepressed() );
	}

	// Damage numbers.
	{
		ConVarRef hud_combattext( "hud_combattext" );
		hud_combattext.SetValue( m_pDamageNumbersEnabled->IsDepressed() );
	}

	// Damage numbers batching.
	{
		ConVarRef hud_combattext_batching( "hud_combattext_batching" );
		hud_combattext_batching.SetValue( m_pDamageBatchingEnabled->IsDepressed() );
	}

	// Damage numbers batching window.
	{
		ConVarRef hud_combattext_batching_window( "hud_combattext_batching_window" );
		char textBuf[1024];
		m_pDamageBatchingWindowEntry->GetText( textBuf, 1024 );
		float flValue = atof( textBuf );
		flValue = clamp( flValue, 0.0f, 60.0f );
		hud_combattext_batching_window.SetValue( flValue );
	}

	// Damage numbers type.
	{
		ConVarRef hud_combattext_type( "hud_combattext_type" );
		hud_combattext_type.SetValue( m_pDamageNumbersTypeSelection->GetActiveItem() );
	}

	DevMsg( "Initial settings applied!\n" );
}

void CFirstTimeOptionsFrame::Close( void )
{
	// Still apply the settings, just in case.
	ApplySettings();

	BaseClass::Close();
}

class CFirstTimeOptionsMenu : public IFirstTimeOptionsMenu
{
public:
	CFirstTimeOptionsMenu()
	{
		m_pOptionsPanel = NULL;		
	}

	virtual void Create( vgui::VPANEL parent )
	{
		m_pOptionsPanel = new CFirstTimeOptionsFrame( parent );
	}

	virtual void Destroy( void )
	{

	}

	virtual void Activate( void )
	{
		if ( m_pOptionsPanel )
		{
			m_pOptionsPanel->Activate();
		}
	}

	virtual void Open( void )
	{
		if ( m_pOptionsPanel )
		{
			m_pOptionsPanel->OpenMenu();
		}
	}

	void ApplySettings( void )
	{
		if ( m_pOptionsPanel )
		{
			m_pOptionsPanel->ApplySettings();

			// We're done! close the window.
			m_pOptionsPanel->Close();
		}
	}

private:
	CFirstTimeOptionsFrame *m_pOptionsPanel;
};
static CFirstTimeOptionsMenu s_FirstTimeOptionsPanel;
IFirstTimeOptionsMenu *firsttimeroptionsmenu = ( IFirstTimeOptionsMenu *)&s_FirstTimeOptionsPanel;

// NOTE: Defined all the way down here so we can access s_FirstTimeOptionsPanel.
void COptionsContinueButton::DoClick( void )
{
	s_FirstTimeOptionsPanel.ApplySettings();

	BaseClass::DoClick();
}

CON_COMMAND( showfirsttimeoptionsmenu, "Shows the first time options menu." )
{
	firsttimeroptionsmenu->Open();
	firsttimeroptionsmenu->Activate();
}