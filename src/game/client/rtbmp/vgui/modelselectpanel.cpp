//========= Copyright RTBA Team, All rights reserved. ============//
//
// Purpose:	Model selection menu for RTB:A.
//
//================================================================//

#include "cbase.h"
#include "modelselectpanel.h"
#include "animation.h"
#include "filesystem.h"

extern ConVar cl_playermodel;

ConVar cl_playermodel_skin( "cl_playermodel_skin", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "The playermodel's skin." );
ConVar cl_bodygroup( "cl_bodygroup", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "The playermodel's set bodygroups." );

CModelSelectPanel::CModelSelectPanel( vgui::VPANEL parent )
	: BaseClass( NULL, "ModelSelectPanel" )
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

	LoadControlSettings( "resource/ui/modelselectpanel.res" );

	m_pPreviewPanel = FindControl<CBaseModelPanel>( "PreviewPanel" );
	m_pSkinList = FindControl<ComboBox>( "SkinList" );
	m_pBodygroupSelection = FindControl<ComboBox>( "BodygroupSelection" );
	m_pBodygroupMeshes = FindControl<ComboBox>( "BodygroupMeshes" );
	m_pModelImage = FindControl<ImagePanel>( "ModelImage" );
	m_pModelList = FindControl<ComboBox>( "ModelList" );

	m_nBody = 0;

	m_pModelImage->SetShouldScaleImage( true );
	// TODO: now that our portrait can be any size we need to modify our portraits so we don't need to scale them
	m_pModelImage->SetScaleAmount( 256.f / 200.f );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

class CModelSelectPanelInterface : public IModelSelectPanel
{
private:
	CModelSelectPanel *pModelSelectPanel;
public:
	CModelSelectPanelInterface()
	{
		pModelSelectPanel = NULL;
	}
	void Create( vgui::VPANEL parent )
	{
		pModelSelectPanel = new CModelSelectPanel( parent );
	}
	void Destroy()
	{
		if ( pModelSelectPanel )
		{
			pModelSelectPanel->SetParent( (vgui::Panel *)NULL );
			delete pModelSelectPanel;
		}
	}
	void Activate( void )
	{
		if ( pModelSelectPanel )
		{
			pModelSelectPanel->Activate();
		}
	}
	void Open( void )
	{
		pModelSelectPanel->Open();
	}
};
static CModelSelectPanelInterface g_ModelSelectPanel;
IModelSelectPanel *modelselectpanel = (IModelSelectPanel *)&g_ModelSelectPanel;

void CModelSelectPanel::Open( void )
{
	m_pPreviewPanel->SetMDL( cl_playermodel.GetString() );
	m_nBody = cl_bodygroup.GetInt();
	m_pPreviewPanel->SetBody( m_nBody );

	m_pModelList->RemoveAll();
	FindPlayerModels( "materials/vgui/playermodels/" );
	PopulateModelSelection();
	RefreshModelOptions();
}

void CModelSelectPanel::SetupSkinList( void )
{
	m_pSkinList->RemoveAll();

	CStudioHdr *pStudioHdr = m_pPreviewPanel->GetStudioHdrFull();

	int nSkins = pStudioHdr->numskinfamilies();

	for ( int i = 0; i < nSkins; i++ )
	{
		m_pSkinList->AddItem( CNumStr( i ), NULL );
	}

	m_pSkinList->ActivateItem( cl_playermodel_skin.GetInt() );

	if ( nSkins > 1 )
		m_pSkinList->SetEnabled( true );
	else
		m_pSkinList->SetEnabled( false );
}

void CModelSelectPanel::SetupBodygroupList( void )
{
	m_pBodygroupSelection->RemoveAll();

	CStudioHdr *pStudioHdr = m_pPreviewPanel->GetStudioHdrFull();
	int nBodygroups = GetNumBodyGroups( m_pPreviewPanel->GetStudioHdrFull() );

	// populate our bodygroup categories
	for ( int i = 0; i < nBodygroups; i++ )
	{
		m_pBodygroupSelection->AddItem( GetBodygroupName( pStudioHdr, i ), NULL );
	}

	m_pBodygroupSelection->ActivateItem( 0 );

	if ( nBodygroups > 1 )
		m_pBodygroupSelection->SetEnabled( true );
	else
		m_pBodygroupSelection->SetEnabled( false );
}

void CModelSelectPanel::SetupBodygroupMeshList( void )
{
	m_pBodygroupMeshes->RemoveAll();

	CStudioHdr *pStudioHdr = m_pPreviewPanel->GetStudioHdrFull();
	int nActiveBodygroup = m_pBodygroupSelection->GetActiveItem();
	int nMeshes = GetBodygroupCount( pStudioHdr, nActiveBodygroup );

	// populate our bodygroup meshes
	for ( int i = 0; i < nMeshes; i++ )
	{
		char szName[32];

		if ( i == 0 )
		{
			Q_strncpy( szName, "none", sizeof( szName ) );
		}
		else
		{
			Q_StripExtension( GetBodygroupPartName( pStudioHdr, nActiveBodygroup, i ), szName, sizeof( szName ) );
		}

		m_pBodygroupMeshes->AddItem( szName, NULL );
	}

	m_pBodygroupMeshes->ActivateItem( GetBodygroup( pStudioHdr, m_nBody, nActiveBodygroup ) );

	if ( nMeshes > 1 )
		m_pBodygroupMeshes->SetEnabled( true );
	else
		m_pBodygroupMeshes->SetEnabled( false );
}

void CModelSelectPanel::SelectBodygroup( int bodygroup, int mesh )
{
	CStudioHdr *pStudioHdr = m_pPreviewPanel->GetStudioHdrFull();
	::SetBodygroup( pStudioHdr, m_nBody, bodygroup, mesh );
	m_pPreviewPanel->SetBody( m_nBody );
	cl_bodygroup.SetValue( CNumStr( m_nBody ) );
}

void CModelSelectPanel::FindPlayerModels( const char *path )
{
	FileFindHandle_t findHandle;
	char szCurrentPath[MAX_PATH];
	Q_strncpy( szCurrentPath, path, sizeof( szCurrentPath ) );

	const char *pFileName = g_pFullFileSystem->FindFirst( CFmtStr("%s*", path), &findHandle );

	// recursive search
	while ( pFileName )
	{
		if ( pFileName[0] != '.' )
		{
			if ( g_pFullFileSystem->FindIsDirectory( findHandle ) )
			{
				CFmtStr szNewPath( "%s%s/", szCurrentPath, pFileName );
				FindPlayerModels( szNewPath );
			}
			// ignore the prison guard playermodel since we use a different model name
			else if ( Q_stristr( pFileName, ".vmt" ) && Q_stricmp( pFileName, "combine_soldier_prisonguard.vmt" ) )
			{
				char pStrippedName[MAX_PATH];
				Q_StripExtension( pFileName, pStrippedName, sizeof( pStrippedName ) );

				CFmtStr szFullName( "%s%s", path, pFileName );
				KeyValues *kv = new KeyValues( pFileName );
				kv->SetString( "filename", szFullName );
				m_pModelList->AddItem( pStrippedName, kv );
				kv->deleteThis();
			}
		}

		pFileName = g_pFullFileSystem->FindNext( findHandle );
	}

	g_pFullFileSystem->FindClose( findHandle );
}

void CModelSelectPanel::OnModelSelected( const char *szName )
{
	// strip the leading path... why is it getting automatically fixed up
	if ( !Q_strnicmp( szName, "materials/vgui/", 15 ) )
	{
		szName += 15;
	}

	m_pModelImage->SetImage( szName );

	// remove "playermodels/"
	szName += 13;

	char szShortName[MAX_PATH];
	Q_StripExtension( szName, szShortName, sizeof( szShortName ) );

	CFmtStr szModelName( "models/%s.mdl", szShortName );
	m_pPreviewPanel->SetMDL( szModelName );
	cl_playermodel.SetValue( szModelName );
	// Nuke any customisations we have
	cl_playermodel_skin.SetValue( 0 );
	cl_bodygroup.SetValue( 0 );
	m_nBody = 0;
	m_pPreviewPanel->SetBody( m_nBody );
	RefreshModelOptions();
}

void CModelSelectPanel::RefreshModelOptions( void )
{
	SetupSkinList();
	SetupBodygroupList();
}

void CModelSelectPanel::PopulateModelSelection( void )
{
	// convert our model name to a display name
	const char *szModelName = cl_playermodel.GetString();

	// remove directory separators
	szModelName += 7; // remove "models/"

	if ( !Q_strnicmp( szModelName, "humans/group03/", 15 ) )
	{
		// remove rebel paths
		szModelName += 15;
	}

	// search for our model name
	char szDisplayName[MAX_PATH];
	Q_StripExtension( szModelName, szDisplayName, sizeof( szDisplayName ) );

	for ( int i = 0; i < m_pModelList->GetItemCount(); i++ )
	{
		char szItemName[MAX_PATH];
		m_pModelList->GetItemText( i, szItemName, sizeof( szItemName ) );

		if ( !Q_stricmp( szDisplayName, szItemName ) )
		{
			// found our model, set its option and set the image
			m_pModelList->SilentActivateItem( i );

			KeyValues *kv = m_pModelList->GetItemUserData( i );
			const char *szFileName = kv->GetString( "filename" );
			m_pModelImage->SetImage( szFileName + 15 );
		}
	}
}

void CModelSelectPanel::OnTextChanged( vgui::Panel *panel )
{
	if ( panel == m_pSkinList )
	{
		m_pPreviewPanel->SetSkin( m_pSkinList->GetActiveItem() );
		cl_playermodel_skin.SetValue( CNumStr( m_pSkinList->GetActiveItem() ) );
	}
	else if ( panel == m_pBodygroupSelection )
	{
		SetupBodygroupMeshList();
	}
	else if ( panel == m_pBodygroupMeshes )
	{
		SelectBodygroup( m_pBodygroupSelection->GetActiveItem(), m_pBodygroupMeshes->GetActiveItem() );
	}
	else if ( panel == m_pModelList )
	{
		KeyValues *kv = m_pModelList->GetActiveItemUserData();
		OnModelSelected( kv->GetString( "filename" ) );
	}
}

CON_COMMAND( showmodelselectpanel, "Shows the model selection panel." )
{
	modelselectpanel->Open();
	modelselectpanel->Activate();
}