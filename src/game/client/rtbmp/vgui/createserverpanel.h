//========= Copyright RTBA Team, All rights reserved. ============//
//
// Purpose:	New listen server creation menu for RTB:A.
//
//================================================================//

#include "icreateserverpanel.h"
#include "materialsystem/imaterial.h"

using namespace vgui;
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>

struct MapInfo
{
    IMaterial *pMapBackground;
    char szMapName[64];
    char szMapDisplayName[32];
    char szGameMode[16];
    char szPlayers[16];
    char szAuthors[64];
    char szDescription[256];
};

class CCreateServerPanel : public vgui::Frame
{
    DECLARE_CLASS_SIMPLE( CCreateServerPanel, vgui::Frame );

    CCreateServerPanel( vgui::VPANEL parent ); 	// Constructor
    ~CCreateServerPanel() {};				    // Destructor

public:
    void Open( void );
    void ParseMapInfo( void );

protected:
    
    virtual void OnTick() OVERRIDE;
    virtual void OnCommand( const char *pcCommand ) OVERRIDE;

private:
    //Other used VGUI control Elements:
    CUtlVector<MapInfo> m_MapInfoList;
};
