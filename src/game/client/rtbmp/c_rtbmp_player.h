//========= Copyright RTBMP Team, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef RTBMP_PLAYER_H
#define RTBMP_PLAYER_H
#pragma once

#include "c_hl2mp_player.h"
#include "glow_outline_effect.h"

#if defined( CLIENT_DLL )
#define CRTBMP_Player C_RTBMP_Player
#endif

//=============================================================================
// >> RTBMP_Player
//=============================================================================
class C_RTBMP_Player : public C_HL2MP_Player
{
public:
	DECLARE_CLASS( C_RTBMP_Player, C_HL2MP_Player );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();


	C_RTBMP_Player();
	~C_RTBMP_Player();

	static C_RTBMP_Player* GetLocalRTBMPPlayer();

	virtual void Precache( void );
	virtual void OnDataChanged( DataUpdateType_t type );
	virtual void ClientThink( void );
	virtual void PreThink( void );
	virtual void PostThink( void );

	bool	CanAlwaysRun( void );
	virtual void	HandleSpeedChanges( CMoveData *mv ) OVERRIDE;
	virtual void	ReduceTimers( CMoveData *mv ) OVERRIDE;

	CGlowObject *GetGlowObject( void ) { return m_pGlowEffect; }
	void				UpdateGlowEffect( void );
	void				DestroyGlowEffect( void );

	virtual void ReceiveMessage( int classID, bf_read &msg );

private:

	C_RTBMP_Player( const C_RTBMP_Player & );

	float m_flSpawnProtectTime;
	CGlowObject *m_pGlowEffect;
};

inline C_RTBMP_Player *ToRTBMPPlayer( CBaseEntity *pEntity )
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;

	return dynamic_cast<C_RTBMP_Player*>(pEntity);
}

#endif //RTBMP_PLAYER_H
