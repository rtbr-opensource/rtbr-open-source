//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Shield used by the mortar synth 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	LIGHTSTALK_H
#define	LIGHTSTALK_H

#include "ai_basenpc.h"
#include "EntityOutput.h"

#define NUM_CAGE_BEAMS 2

enum LightstalkFade_t
{
	FADE_NONE,
	FADE_OUT,
	FADE_IN,
};

class CSprite;

class CNPC_LightStalk : public CAI_BaseNPC
{
public:
	DECLARE_CLASS(CNPC_LightStalk, CAI_BaseNPC);

	void		Spawn(void);
	void		Precache(void);
	void		Touch(CBaseEntity *pOther);
	void		Think(void);
	void		LightRise(void);
	void		LightLower(void);

	COutputEvent m_OnRise;
	COutputEvent m_OnLower;

private:
	float				m_flHideEndTime;
	float				m_flStartFadeTime;
	LightstalkFade_t	m_nFadeDir;
	CSprite*			m_pGlow;

public:
	DECLARE_DATADESC();
};
#endif	//LIGHTSTALK_H


