//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
/*#include "hud.h"
#include "text_message.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include "KeyValues.h"
#include "vgui_controls/AnimationController.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "IEffects.h"
#include "hudelement.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: HDU Damage indication
//-----------------------------------------------------------------------------
class CInfoPanel : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CInfoPanel, vgui::Panel);

public:
	CInfoPanel(const char* pElementName);
	void Reset(void);
	virtual bool ShouldDraw(void);

private:
	virtual void OnThink();
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);

private:
	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "Bahnschrift");
	CPanelAnimationVar(Color, m_TextColor, "TextColor", "FgColor");
	CPanelAnimationVarAliasType(float, text_xpos, "text_xpos", "1", "proportional_float");
	CPanelAnimationVarAliasType(float, text_ypos, "text_ypos", "8", "proportional_float");
	CPanelAnimationVarAliasType(float, text_ygap, "text_ygap", "14", "proportional_float");

};

DECLARE_HUDELEMENT(CInfoPanel);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CInfoPanel::CInfoPanel(const char* pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudPoisonDamageIndicator")
{
	vgui::Panel* pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInfoPanel::Reset(void)
{
	SetAlpha(0);
	SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CInfoPanel::ShouldDraw(void)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return false;


	return (CHudElement::ShouldDraw());
}

//-----------------------------------------------------------------------------
// Purpose: updates poison damage
//-----------------------------------------------------------------------------
void CInfoPanel::OnThink()
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;
}

//-----------------------------------------------------------------------------
// Purpose: Paints the damage display
//-----------------------------------------------------------------------------
void CInfoPanel::Paint()
{
	// draw the text
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(m_TextColor);
	surface()->DrawSetTextPos(text_xpos, text_ypos);
	int ypos = text_ypos;

	const wchar_t* labelText = L"Raising the Bar: Redux\nDeveloper Build: 11/29/20\nSteamID: 3940104810\nTime: 10:35 AM\nDate:11/26/20";
	Assert(labelText);
	for (const wchar_t* wch = labelText; wch && *wch != 0; wch++)
	{
		if (*wch == '\n')
		{
			ypos += text_ygap;
			surface()->DrawSetTextPos(text_xpos, ypos);
		}
		else
		{
			surface()->DrawUnicodeChar(*wch);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CInfoPanel::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}*/