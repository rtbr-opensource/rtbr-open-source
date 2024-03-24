//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot by bullsquid 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	GrenadeSpitHL1_H
#define	GrenadeSpitHL1_H

#include "hl1_basegrenade.h"

enum SpitSize_e
{
	SPIT_SMALL,
	SPIT_MEDIUM,
	SPIT_LARGE,
};

#define SPIT_GRAVITY 0.9

class CGrenadeSpitHL1 : public CHL1BaseGrenade
{
public:
	DECLARE_CLASS( CGrenadeSpitHL1, CHL1BaseGrenade );

	void		Spawn( void );
	void		Precache( void );
	void		SpitThink( void );
	void 		GrenadeSpitHL1Touch( CBaseEntity *pOther );
	void		Event_Killed( const CTakeDamageInfo &info );
	void		SetSpitSize(int nSize);

	int			m_nSquidSpitSprite;
	float		m_fSpitDeathTime;		// If non-zero won't detonate

	void EXPORT				Detonate(void);
	CGrenadeSpitHL1(void);

	DECLARE_DATADESC();
};

#endif	//GrenadeSpitHL1_H