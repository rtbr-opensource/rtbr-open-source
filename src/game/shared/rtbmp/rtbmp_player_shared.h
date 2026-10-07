//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared data between client and server side RTB:A player classes.
//
//=============================================================================//

#ifndef RTBMP_PLAYER_SHARED_H
#define RTBMP_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

// Message ID constants used for communciation between client and server.
enum
{
	DAMAGE_FLINCH = 10,
	SHIELD_BREAK_FX,
};
#endif // RTBMP_PLAYER_SHARED_H
