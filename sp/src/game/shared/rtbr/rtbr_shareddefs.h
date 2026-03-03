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
	RTBRCOLLISION_GROUP_FRIENDANTLION = LAST_HL2_COLLISION_GROUP,
	RTBRCOLLISION_GROUP_ALYXFRIENDANTLION,
	RTBRCOLLISION_GROUP_SANDBARNACLE, // Used to disable collisions between vehicles and sand barnacles.
};

// Multi-bit damage types
// NOTE: These are not singular bits; if you want to check for one of these damage types,
// you should check whether the result of a bitwise AND is equal to your desired damage type.
// This is a workaround for all 32 damage types being taken.
#define DMG_STUNSTICK		(DMG_CLUB | DMG_SHOCK)
#define DMG_CHARGEDGAUSS	(DMG_SHOCK | DMG_ENERGYBEAM)
#define DMG_WLSCANNER		(DMG_SHOCK | DMG_DIRECT)

#endif // RTBR_SHAREDDEFS_H