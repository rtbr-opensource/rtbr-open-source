//========= Copyright RTBA Team, All rights reserved. ============//
//
// Purpose:	Model selection menu menu for RTB:A.
//
//================================================================//

#include "imodelselectpanel.h"
#include "basemodel_panel.h"
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>

using namespace vgui;
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>

class CModelSelectPanel : public vgui::Frame
{
    DECLARE_CLASS_SIMPLE( CModelSelectPanel, vgui::Frame );

    CModelSelectPanel( vgui::VPANEL parent ); 	// Constructor
    ~CModelSelectPanel() {};				    // Destructor

public:
    void Open( void );

protected:
    void SetupSkinList( void );
    void SetupBodygroupList( void );
    void SetupBodygroupMeshList( void );
    void SelectBodygroup( int bodygroup, int mesh );
    void FindPlayerModels( const char *path );
    void OnModelSelected( const char *szName );
    void RefreshModelOptions( void );
    void PopulateModelSelection( void );

    MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel ); // send by combo box when it changes

private:
    CBaseModelPanel *m_pPreviewPanel;
    int m_nBody;
    ComboBox *m_pSkinList;
    ComboBox *m_pBodygroupSelection;
    ComboBox *m_pBodygroupMeshes;
    ImagePanel *m_pModelImage;
    ComboBox *m_pModelList;
};
