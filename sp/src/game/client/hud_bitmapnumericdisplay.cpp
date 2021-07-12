//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud_bitmapnumericdisplay.h"
#include "iclientmode.h"

#include <Color.h>
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar xoffset("xoffset", 0, 0);
ConVar yoffset("yoffset", 0, 0);
ConVar hud_draw_backgrounds("hud_draw_backgrounds", 0, 0);

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CHudBitmapNumericDisplay::CHudBitmapNumericDisplay(vgui::Panel *parent, const char *name) : vgui::Panel(parent, name)
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_iValue = 0;
	m_iSecondaryValue = 0;
	m_iLabelType = 0;
	m_iNumZeros = 3;
	m_bIsAmmo = false;
	m_bDisplaySecondaryValue = false;
	m_bDisplayValue = true;
	m_bHalfSize = false;
	memset( m_pNumbers, 0, 10*sizeof(CHudTexture *) );
}

//-----------------------------------------------------------------------------
// Purpose: Resets values on restore/new map
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::Reset()
{
	m_flBlur = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::SetShouldDisplayValue(bool state)
{
	m_bDisplayValue = state;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::SetHalfSize(bool state)
{
	m_bHalfSize = state;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::SetIsAmmo(bool state)
{
	m_bIsAmmo = state;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::SetNumZeros(int value)
{
	m_iNumZeros = value;
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::SetDisplayValue(int value)
{
	m_iValue = value;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::SetShouldDisplaySecondaryValue(bool state)
{
	m_bDisplaySecondaryValue = state;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::SetSecondaryValue(int value)
{
	m_iSecondaryValue = value;
}

//-----------------------------------------------------------------------------
// Purpose: renders the vgui panel
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::Paint()
{
	float alpha = m_flAlphaOverride / 255;
	Color fgColor = GetFgColor();
	fgColor[3] *= alpha;
	SetFgColor( fgColor );

	if (m_bDisplayValue)
	{
		// draw our numbers
		//	surface()->DrawSetTextColor(GetFgColor());
		PaintNumbers(digit_xpos, digit_ypos, m_iValue, GetFgColor(), m_bHalfSize);
		PaintZeros(digit_xpos, digit_ypos, GetFgColor(), m_bHalfSize, m_iNumZeros);

		// draw the overbright blur
		for (float fl = m_flBlur; fl > 0.0f; fl -= 1.0f)
		{
			if (fl >= 1.0f)
			{
				PaintNumbers(digit_xpos, digit_ypos, m_iValue, GetFgColor(), m_bHalfSize);
				PaintZeros(digit_xpos, digit_ypos, GetFgColor(), m_bHalfSize, m_iNumZeros);
			}
			else
			{
				// draw a percentage of the last one
				Color col = GetFgColor();
				col[3] *= fl;
				PaintNumbers(digit_xpos, digit_ypos, m_iValue, col, m_bHalfSize);
				PaintZeros(digit_xpos, digit_ypos, col, m_bHalfSize, m_iNumZeros);
			}
		}
	}

	// total ammo
	if (m_bDisplaySecondaryValue)
	{
		PaintNumbers(digit2_xpos, digit2_ypos, m_iSecondaryValue, GetFgColor(), true);
		PaintZeros(digit2_xpos, digit2_ypos, GetFgColor(), true, m_iNumZeros);
	}

	if (m_bIsAmmo)
	{
		PaintAmmoBar();
	}

	PaintLabel();

}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::PaintAmmoBar(void)
{
	CHudTexture *ammoBar = gHUD.GetIcon("ammo_bar");

	if (!ammoBar)
	{
		Msg("Missing hud texture\n");
		return;
	}

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();

	if (player == NULL)
		return;

	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

	if (pWeapon == NULL)
		return;

	int width, height;

	Color col = GetFgColor();

	//Set ammo bar same height as the large numbers next to it
	width = ammoBar->Width();
	height = ammoBar->Height();

	//Get our ammo value
	int	ammo = pWeapon->Clip1();

	float ammoPerc = 1 - ((float)ammo / (float)pWeapon->GetMaxClip1());

	gHUD.DrawIconProgressBar(digit_xpos + xoffset.GetInt(), digit_ypos + yoffset.GetInt(), width, height, ammoBar, ammoPerc, col, CHud::HUDPB_VERTICAL);
	//ammoBar->DrawSelf(digit_xpos + xoffset.GetInt(), digit_ypos + yoffset.GetInt(), width, height, col);

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::PaintBackground( void )
{
	if (hud_draw_backgrounds.GetInt())
	{
		int alpha = m_flAlphaOverride / 255;
		Color bgColor = GetBgColor();
		bgColor[3] *= alpha;
		SetBgColor(bgColor);

		BaseClass::PaintBackground();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::PaintNumbers(int xpos, int ypos, int value, Color col, bool secondary, int numSigDigits)
{
	if( !m_pNumbers[0] )
	{
		int i;
		char a[16];

		for( i=0;i<10;i++ )
		{
			sprintf( a, "number_%d", i );

			m_pNumbers[i] = gHUD.GetIcon( a );
		}

		if( !m_pNumbers[0] )
			return;
	}

	if( value > 100000 )
	{
		value = 99999;
	}

	int pos = 10000;

	float scale = ( digit_height / (float)m_pNumbers[0]->Height());

	if (secondary)
		scale /= 2;

	int digit;
	Color color = GetFgColor();
	int width = m_pNumbers[0]->Width() * scale;
	int height = m_pNumbers[0]->Height() * scale;
	bool bStart = false;

	//right align to xpos

	int numdigits = 1;

	int x = pos;
	while( x >= 10 )
	{
		if( value >= x )
			numdigits++;

		x /= 10;
	}

	if( numdigits < numSigDigits )
		numdigits = numSigDigits;

	xpos -= numdigits * width;

	//draw the digits
	while( pos >= 1 )
	{
		digit = value / pos;
		value = value % pos;

		if( bStart || digit > 0 || pos <= pow(10.0f,numSigDigits-1) )
		{
			bStart = true;
			m_pNumbers[digit]->DrawSelf( xpos, ypos, width, height, col );
			xpos += width;
		}

		pos /= 10;
	}
}

//-----------------------------------------------------------------------------
// Purpose:  paint background zeros
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::PaintZeros(int xpos, int ypos, Color col, bool secondary, int numZeros)
{
	CHudTexture *faintZero = gHUD.GetIcon("number_0_faint");
	CHudTexture *faintZeroTwo = gHUD.GetIcon("number_0_faint_2");

	float scale = (digit_height / (float)m_pNumbers[0]->Height());

	if (secondary)
		scale /= 2;

	Color color = GetFgColor();
	int width = faintZero->Width() * scale;
	int height = faintZero->Height() * scale;

	for (int i = 0; i<numZeros; i++)
	{
		xpos -= width;
		if (i > 1)
		{
			faintZeroTwo->DrawSelf(xpos, ypos, width, height, col);
		}
		else
		{
			faintZero->DrawSelf(xpos, ypos, width, height, col);
		}

	}

}

//-----------------------------------------------------------------------------
// Purpose: set label type
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::SetLabel(int label)
{
	m_iLabelType = label;
}


//-----------------------------------------------------------------------------
// Purpose:  paint labels
//-----------------------------------------------------------------------------
void CHudBitmapNumericDisplay::PaintLabel(void)
{
	CHudTexture *healthLabel = gHUD.GetIcon("health_label");
	CHudTexture *batteryLabel = gHUD.GetIcon("battery_label");
	CHudTexture *ammoLabel = gHUD.GetIcon("ammo_label");

	if (!healthLabel || !batteryLabel || !ammoLabel)
	{
		Msg("Missing label\n");
		return;
	}

	int width;
	int height;
	Color col = GetFgColor();

	switch (m_iLabelType)
	{
	case 1:
		width = healthLabel->Width();
		height = healthLabel->Height();
		healthLabel->DrawSelf(text_xpos, text_ypos, width, height, col);
		break;
	case 2:
		width = batteryLabel->Width();
		height = batteryLabel->Height();
		batteryLabel->DrawSelf(text_xpos, text_ypos, width, height, col);
		break;
	case 3:
		width = ammoLabel->Width();
		height = ammoLabel->Height();
		ammoLabel->DrawSelf(text_xpos, text_ypos, width, height, col);
		break;
	}

}