//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared data between client and server side npc_cremator classes.
//
//=============================================================================//

#ifndef NPC_CREMATOR_SHARED_H
#define NPC_CREMATOR_SHARED_H
#ifdef _WIN32
#pragma once
#endif

// Message ID constants used for communciation between client and server.
enum
{
	CREMATOR_MSG_START_IMMO = 10,
	CREMATOR_MSG_STOP_IMMO,
	CREMATOR_MSG_KILLED,
	CREMATOR_MSG_START_TANK,
	CREMATOR_MSG_STOP_TANK,
};
#endif
