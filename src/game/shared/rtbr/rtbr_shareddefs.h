//========= Copyright RTBR Team, 2024, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef RTBR_SHAREDDEFS_H
#define RTBR_SHAREDDEFS_H

#ifdef _WIN32
#pragma once
#endif

#include "hl2_shareddefs.h"


//--------------------------------------------------------------------------
// Collision groups
//--------------------------------------------------------------------------

enum
{
	RTBRCOLLISION_GROUP_IRIFLEPROJECTILE = LAST_HL2_COLLISION_GROUP,
	RTBRCOLLISION_GROUP_FRIENDANTLION,
	RTBRCOLLISION_GROUP_ALYXFRIENDANTLION,
	RTBRCOLLISION_GROUP_SANDBARNACLE, // Used to disable collisions between vehicles and sand barnacles.
};

// radius damage flags
enum
{
	RD_NOFALLOFF = 1 << 0,
	RD_STUNNPC = 1 << 1,
};

// Multi-bit damage types
// NOTE: These are not singular bits; if you want to check for one of these damage types,
// you should check whether the result of a bitwise AND is equal to your desired damage type.
// This is a workaround for all 32 damage types being taken.
#define DMG_STUNSTICK		(DMG_CLUB | DMG_SHOCK)
#define DMG_CHARGEDGAUSS	(DMG_SHOCK | DMG_ENERGYBEAM)
#define DMG_WLSCANNER		(DMG_SHOCK | DMG_DIRECT)

#define GRENADE_TIMER	2.5f //Seconds
#define FRAG_GRENADE_BLIP_FREQUENCY			1.0f
#define FRAG_GRENADE_BLIP_FAST_FREQUENCY	0.3f

#define FRAG_GRENADE_GRACE_TIME_AFTER_PICKUP 1.5f
#define FRAG_GRENADE_WARN_TIME 1.5f

#define RETHROW_DELAY	0.5

#endif // RTBR_SHAREDDEFS_H