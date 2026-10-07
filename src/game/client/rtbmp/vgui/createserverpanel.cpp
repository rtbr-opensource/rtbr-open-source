//========= Copyright RTBA Team, All rights reserved. ============//
//
// Purpose:	New listen server creation menu for RTB:A.
//
//================================================================//

#include "cbase.h"
#include "createserverpanel.h"
#include "filesystem.h"

CCreateServerPanel::CCreateServerPanel( vgui::VPANEL parent )
	: BaseClass( NULL, "CreateServerPanel" )
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

	LoadControlSettings( "resource/ui/createserverpanel.res" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	DevMsg( "CreateServerPanel has been constructed\n" );
}

class CCreateServerPanelInterface : public ICreateServerPanel
{
private:
	CCreateServerPanel *pCreateServerPanel;
public:
	CCreateServerPanelInterface()
	{
		pCreateServerPanel = NULL;
	}
	void Create( vgui::VPANEL parent )
	{
		pCreateServerPanel = new CCreateServerPanel( parent );
	}
	void Destroy()
	{
		if ( pCreateServerPanel )
		{
			pCreateServerPanel->SetParent( (vgui::Panel *)NULL );
			delete pCreateServerPanel;
		}
	}
	void Activate( void )
	{
		if ( pCreateServerPanel )
		{
			pCreateServerPanel->Activate();
		}
	}
	void Open( void )
	{
		pCreateServerPanel->Open();
	}
};
static CCreateServerPanelInterface g_CreateServerPanel;
ICreateServerPanel *createserverpanel = (ICreateServerPanel *)&g_CreateServerPanel;

void CCreateServerPanel::OnTick()
{
	BaseClass::OnTick();
}

void CCreateServerPanel::OnCommand( const char *pcCommand )
{
	BaseClass::OnCommand( pcCommand );
}

void CCreateServerPanel::Open( void )
{
	SetVisible( true );
	ParseMapInfo();
}

void CCreateServerPanel::ParseMapInfo( void )
{
	//FileFindHandle_t findHandle;
	//filesystem->FindFirstEx( "maps/*.bsp", "MOD", &findHandle );
	//KeyValues *pKV = new KeyValues();
}

CON_COMMAND( showcreateserverpanel, "Shows the new listen server creation panel." )
{
	createserverpanel->Open();
	createserverpanel->Activate();
}