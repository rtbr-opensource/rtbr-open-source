#include "hud.h"
#include "cbase.h"
#include "keycard.h"
#include "iclientmode.h"
#include "hud_macros.h"
#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"

#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT(CHudKeycard_1);
static ConVar show_keycard("show_keycard", "0", 0, "Shows keycard. 1 is lvl 1, 2 is lvl 2, 4 is lvl 3. Add together for multiple.");

CHudKeycard_1::CHudKeycard_1(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudKeycard_1")
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetVisible(false);
	SetAlpha(255);

	m_nImport = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nImport, "hud/keycard01", true, true);

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}

void CHudKeycard_1::Paint()
{
	SetPaintBorderEnabled(false);
	surface()->DrawSetTexture(m_nImport);
	surface()->DrawTexturedRect(16, -16, 128, 128);
}

void CHudKeycard_1::togglePrint()
{
	cvvalue = (0 - show_keycard.GetInt()) + 7;
	if (cvvalue == 1 || cvvalue == 3 || cvvalue == 5 || cvvalue == 7)
		this->SetVisible(false);
	else
		this->SetVisible(true);
}

void CHudKeycard_1::OnThink()
{
	togglePrint();

	BaseClass::OnThink();
}

DECLARE_HUDELEMENT(CHudKeycard_2);

CHudKeycard_2::CHudKeycard_2(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudKeycard_2")
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetVisible(false);
	SetAlpha(255);

	m_nImport = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nImport, "hud/keycard02", true, true);

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}

void CHudKeycard_2::Paint()
{
	SetPaintBorderEnabled(false);
	surface()->DrawSetTexture(m_nImport);
	surface()->DrawTexturedRect(16, -16, 128, 128);
}

void CHudKeycard_2::togglePrint()
{
	cvvalue = (0 - show_keycard.GetInt()) + 7;
	if (cvvalue == 2 || cvvalue == 3 || cvvalue == 6 || cvvalue == 7)
		this->SetVisible(false);
	else
		this->SetVisible(true);
}

void CHudKeycard_2::OnThink()
{
	togglePrint();

	BaseClass::OnThink();
}

DECLARE_HUDELEMENT(CHudKeycard_3);

CHudKeycard_3::CHudKeycard_3(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudKeycard_3")
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetVisible(false);
	SetAlpha(255);

	m_nImport = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nImport, "hud/keycard03", true, true);

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}

void CHudKeycard_3::Paint()
{
	SetPaintBorderEnabled(false);
	surface()->DrawSetTexture(m_nImport);
	surface()->DrawTexturedRect(16, -16, 128, 128);
}

void CHudKeycard_3::togglePrint()
{
	cvvalue = (0 - show_keycard.GetInt()) + 7;
	if (cvvalue == 4 || cvvalue == 5 || cvvalue == 6 || cvvalue == 7)
		this->SetVisible(false);
	else
		this->SetVisible(true);
}

void CHudKeycard_3::OnThink()
{
	togglePrint();

	BaseClass::OnThink();
}