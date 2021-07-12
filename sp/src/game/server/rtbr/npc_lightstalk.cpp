//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "npc_lightstalk.h"
#include "Sprite.h"
#include "ndebugoverlay.h"

#define	LIGHTSTALK_MODEL			"models/props_xen/xen_light.mdl"
#define LIGHTSTALK_GLOW_SPRITE		"sprites/glow03.vmt"
#define LIGHTSTALK_HIDE_TIME		5
#define	LIGHTSTALK_FADE_TIME		2
#define LIGHTSTALK_MAX_BRIGHT		120

LINK_ENTITY_TO_CLASS(npc_lightstalk, CNPC_LightStalk);

BEGIN_DATADESC(CNPC_LightStalk)

DEFINE_FIELD(m_pGlow, FIELD_CLASSPTR),
DEFINE_FIELD(m_flStartFadeTime, FIELD_TIME),
DEFINE_FIELD(m_nFadeDir, FIELD_INTEGER),
DEFINE_FIELD(m_flHideEndTime, FIELD_TIME),

// outputs
DEFINE_OUTPUT(m_OnRise, "OnRise"),
DEFINE_OUTPUT(m_OnLower, "OnLower"),

// Function Pointers

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::Precache(void)
{
	engine->PrecacheModel(LIGHTSTALK_MODEL);
	engine->PrecacheModel(LIGHTSTALK_GLOW_SPRITE);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::Spawn(void)
{
	Precache();
	SetModel(LIGHTSTALK_MODEL);

	SetMoveType(MOVETYPE_NONE);
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_TRIGGER);
	AddSolidFlags(FSOLID_NOT_SOLID);

	UTIL_SetSize(this, Vector(-80, -80, 0), Vector(80, 80, 32));
	SetActivity(ACT_IDLE);
	SetNextThink(gpGlobals->curtime + 0.1f);

	m_pGlow = CSprite::SpriteCreate(LIGHTSTALK_GLOW_SPRITE, GetLocalOrigin() + Vector(0, 0, (WorldAlignMins().z + WorldAlignMaxs().z)*0.5), false);
	m_pGlow->SetTransparency(kRenderGlow, m_clrRender->r, m_clrRender->g, m_clrRender->b, m_clrRender->a, kRenderFxGlowShell);
	m_pGlow->SetScale(1.0);
	m_pGlow->SetAttachment(this, 1);
	LightRise();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::Think(void)
{
	StudioFrameAdvance();
	SetNextThink(gpGlobals->curtime + 0.1f);

	switch (GetActivity())
	{
	case ACT_CROUCH:
		if (IsActivityFinished())
		{
			SetActivity(ACT_CROUCHIDLE);
		}
		break;

	case ACT_CROUCHIDLE:
		if (gpGlobals->curtime > m_flHideEndTime)
		{
			SetActivity(ACT_STAND);
			LightRise();
		}
		break;

	case ACT_STAND:
		if (IsActivityFinished())
		{
			SetActivity(ACT_IDLE);
		}
		break;

	case ACT_IDLE:
	default:
		break;
	}

	if (m_nFadeDir && m_pGlow)
	{
		float flFadePercent = (gpGlobals->curtime - m_flStartFadeTime) / LIGHTSTALK_FADE_TIME;

		if (flFadePercent > 1)
		{
			m_nFadeDir = FADE_NONE;
		}
		else
		{
			if (m_nFadeDir == FADE_IN)
			{
				m_pGlow->SetBrightness(LIGHTSTALK_MAX_BRIGHT*flFadePercent);
			}
			else
			{
				//m_pGlow->SetBrightness(LIGHTSTALK_MAX_BRIGHT*(1-flFadePercent));
				// Fade out immedately
				m_pGlow->SetBrightness(0);
			}
		}
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::Touch(CBaseEntity *pOther)
{
	if (pOther->IsPlayer())
	{
		m_flHideEndTime = gpGlobals->curtime + LIGHTSTALK_HIDE_TIME;
		if (GetActivity() == ACT_IDLE || GetActivity() == ACT_STAND)
		{
			SetActivity(ACT_CROUCH);
			LightLower();
		}
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::LightRise(void)
{
	m_OnRise.FireOutput(this, this);
	m_nFadeDir = FADE_IN;
	m_flStartFadeTime = gpGlobals->curtime;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_LightStalk::LightLower(void)
{
	m_OnLower.FireOutput(this, this);
	m_nFadeDir = FADE_OUT;
	m_flStartFadeTime = gpGlobals->curtime;
}