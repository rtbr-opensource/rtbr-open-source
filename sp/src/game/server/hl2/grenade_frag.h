//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GRENADE_FRAG_H
#define GRENADE_FRAG_H
#pragma once

#define FRAG_GRENADE_BLIP_FREQUENCY			1.0f
#define FRAG_GRENADE_BLIP_FAST_FREQUENCY	0.3f

#define FRAG_GRENADE_GRACE_TIME_AFTER_PICKUP 1.5f
#define FRAG_GRENADE_WARN_TIME 1.5f

class CBaseGrenade;
struct edict_t;

CBaseGrenade *Fraggrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, bool combineSpawned, float startTime = -1 );
bool	Fraggrenade_WasPunted( const CBaseEntity *pEntity );
bool	Fraggrenade_WasCreatedByCombine( const CBaseEntity *pEntity );

#endif // GRENADE_FRAG_H
