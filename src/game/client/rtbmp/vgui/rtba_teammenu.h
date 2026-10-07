//========= Copyright RTBA Team, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef RTBA_TEAMMENU_H
#define RTBA_TEAMMENU_H
#ifdef _WIN32
#pragma once
#endif

#include "teammenu.h"

//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CRTBATeamMenu : public CTeamMenu
{
private:
	DECLARE_CLASS_SIMPLE( CRTBATeamMenu, CTeamMenu );

public:
	CRTBATeamMenu( IViewPort *pViewPort );
	virtual ~CRTBATeamMenu();

	virtual const char *GetName( void ) { return PANEL_TEAM; }
	virtual void OnThink() OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
	virtual void ShowPanel( bool bShow ) OVERRIDE;
	virtual void OnTick() OVERRIDE;
	virtual void Paint( void ) OVERRIDE;

protected:
	virtual void OnKeyCodePressed( vgui::KeyCode code );

	// helper functions
	virtual void LoadMapPage( const char *mapName );

	// command callbacks
	virtual void OnCommand( const char *command ) OVERRIDE;

private:
	vgui::Button	*m_pAutoAssignButton;
	vgui::Button	*m_pSpecTeamButton;
	vgui::Button	*m_pCombineTeamButton;
	vgui::Button	*m_pRebelTeamButton;
	vgui::Button	*m_pCancelButton;

	vgui::Label		*m_pCombineCountLabel;
	vgui::Label		*m_pRebelCountLabel;

	ButtonCode_t	m_iTeamMenuKey;
};
#endif // RTBA_TEAMMENU_H