#include "cbase.h"
/*#include "hud.h"
#include "text_message.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>
#include "vguimatsurface/IMatSystemSurface.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMesh.h"
#include "materialsystem/imaterialvar.h"
#include "ieffects.h"
#include "hudelement.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

typedef struct
{
	float		flExpire;
	int			x;
	int			y;
	CHudTexture* icon;
} screendamagefx_t;

//-----------------------------------------------------------------------------
// Purpose: HUD Damage type tiles
//-----------------------------------------------------------------------------
class CHudScreenDamageFX : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudScreenDamageFX, vgui::Panel);

public:
	CHudScreenDamageFX(const char* pElementName);

	void	Init(void);
	void	VidInit(void);
	void	Reset(void);
	bool	ShouldDraw(void);

	// Handler for our message
	void	MsgFunc_Damage(bf_read& msg);

private:
	void	Paint();
	void	ApplySchemeSettings(vgui::IScheme* pScheme);

private:
	CHudTexture* DamageTileIcon(int i);
	long		DamageTileFlags(int i);
	void		UpdateTiles(long bits);

private:
	long			m_bitsDamage;
	damagetile_t	m_dmgTileInfo[NUM_DMG_TYPES];
};


DECLARE_HUDELEMENT(CHudDamageTiles);
DECLARE_HUD_MESSAGE(CHudDamageTiles, Damage);*/