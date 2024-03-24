//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/Panel.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRENADE_DETONATION_TIME 3.0f
#define GRENADE_TICK_TIME 0.5f

//-----------------------------------------------------------------------------
// Purpose: Shows the grenade cooking bar.
//-----------------------------------------------------------------------------
class CHudGrenadeTimer : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudGrenadeTimer, vgui::Panel);

public:
	CHudGrenadeTimer(const char *pElementName);
	virtual void	Init(void);
	virtual void	Reset(void);
	virtual void	OnThink(void);
	bool			ShouldDraw(void);

protected:
	virtual void	Paint();

private:
	CPanelAnimationVar(Color, m_GrenadeTimerColor, "GrenadeTimerColor", "255 220 0 220");
	CPanelAnimationVar(int, m_iGrenadeTimerDisabledAlpha, "GrenadeTimerDisabledAlpha", "70");
	CPanelAnimationVar(Color, m_GrenadeFinalTimerColor, "GrenadeFinalTimerColor", "255 50 0 220");

	CPanelAnimationVarAliasType(float, m_flBarInsetX, "BarInsetX", "8", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarInsetY, "BarInsetY", "8", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarWidth, "BarWidth", "72", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarHeight, "BarHeight", "10", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarChunkWidth, "BarChunkWidth", "10", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarChunkGap, "BarChunkGap", "2", "proportional_float");

	float m_flGrenadeStart;
};

DECLARE_HUDELEMENT(CHudGrenadeTimer);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudGrenadeTimer::CHudGrenadeTimer(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudGrenadeTimer")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}

void CHudGrenadeTimer::Init()
{
	m_flGrenadeStart = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGrenadeTimer::Reset(void)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudGrenadeTimer::ShouldDraw()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return false;

	return (CHudElement::ShouldDraw());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGrenadeTimer::OnThink(void)
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	if (m_flGrenadeStart == -1 && pPlayer->m_Local.m_flGrenadeStart != -1)
	{
		// we've just drawn a grenade
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("GrenadeDrawn");
	}
	else if (m_flGrenadeStart != -1 && pPlayer->m_Local.m_flGrenadeStart == -1)
	{
		// we've just thrown a grenade
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("GrenadeThrown");
	}

	m_flGrenadeStart = pPlayer->m_Local.m_flGrenadeStart;
}

//-----------------------------------------------------------------------------
// Purpose: draws the cooking bar.
//-----------------------------------------------------------------------------
void CHudGrenadeTimer::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;
	SetPaintBackgroundEnabled(false);
	if (m_flGrenadeStart == -1 || gpGlobals->curtime - m_flGrenadeStart < 0.1f){ // don't draw if we just started the grenade timer. allows for instant throwing without the timer showing up
		return;
	}

	// get bar chunks
	int chunkCount = GRENADE_DETONATION_TIME / GRENADE_TICK_TIME; // 6 by default
	int enabledChunks = (int)(chunkCount * (gpGlobals->curtime - m_flGrenadeStart) / GRENADE_DETONATION_TIME);
	if (enabledChunks > chunkCount)
		enabledChunks = chunkCount; // cap at chunkcount
	// draw the grenade cooking bar
	float currChunkTime = m_flGrenadeStart + enabledChunks * GRENADE_TICK_TIME;
	int xpos = m_flBarInsetX, ypos = m_flBarInsetY;
	for (int i = 0; i < enabledChunks; i++)
	{
		if (i == enabledChunks - 1){
			int newAlpha = m_iGrenadeTimerDisabledAlpha + MIN((gpGlobals->curtime - currChunkTime) * 1500, 150); // fade in the latest bar over 0.1 seconds.
			if (i == chunkCount - 1){
				surface()->DrawSetColor(Color(m_GrenadeFinalTimerColor[0], m_GrenadeFinalTimerColor[1], m_GrenadeFinalTimerColor[2], newAlpha)); // active red for final bar
			}
			else{
				surface()->DrawSetColor(Color(m_GrenadeTimerColor[0], m_GrenadeTimerColor[1], m_GrenadeTimerColor[2], newAlpha));
			}
			
		}
		else{
			surface()->DrawSetColor(m_GrenadeTimerColor);
		}
		ypos = m_flBarInsetY - abs(i - 2.5f)*abs(i - 2.5f) * 2;
		if (i == 0 || i == chunkCount - 1) { ypos -= 1; }

		surface()->DrawFilledRect(xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight);
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}
	// draw the exhausted portion of the bar.
	surface()->DrawSetColor(Color(m_GrenadeTimerColor[0], m_GrenadeTimerColor[1], m_GrenadeTimerColor[2], m_iGrenadeTimerDisabledAlpha));
	for (int i = enabledChunks; i < chunkCount; i++)
	{
		if (i == chunkCount - 1){
			surface()->DrawSetColor(Color(m_GrenadeFinalTimerColor[0], m_GrenadeFinalTimerColor[1], m_GrenadeFinalTimerColor[2], m_iGrenadeTimerDisabledAlpha)); // disabled red for final bar
		}
		ypos = m_flBarInsetY - abs(i - 2.5f)*abs(i - 2.5f) * 2;
		if (i == 0 || i == chunkCount - 1) { ypos -= 1; }
		surface()->DrawFilledRect(xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight);
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}
}