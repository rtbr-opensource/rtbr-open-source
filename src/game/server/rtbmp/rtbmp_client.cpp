//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== rtbmp_client.cpp ========================================================

RTBMP client/server game specific stuff

*/

#include "cbase.h"
#include "rtbmp_player.h"
#include "rtbmp_gamerules.h"
#include "team.h"
#include "viewport_panel_names.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_motd_unload_on_dismissal( "sv_motd_unload_on_dismissal", "0", 0, "If enabled, the MOTD contents will be unloaded when the player closes the MOTD." );

void FinishClientPutInServer( CRTBMP_Player *pPlayer )
{
	pPlayer->InitialSpawn();
	pPlayer->Spawn();

	char sName[128];
	Q_strncpy( sName, pPlayer->GetPlayerName(), sizeof( sName ) );

	// First parse the name and remove any %'s
	for (char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++)
	{
		// Replace it with a space
		if (*pApersand == '%')
			*pApersand = ' ';
	}

	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>" );

	if (HL2MPRules()->IsTeamplay() == true)
	{
		ClientPrint( pPlayer, HUD_PRINTTALK, "You are on team %s1\n", pPlayer->GetTeam()->GetName() );
	}

	HL2MPRules()->PlayerCountChanged();

	const ConVar *hostname = cvar->FindVar( "hostname" );
	const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

	KeyValues *data = new KeyValues( "data" );
	data->SetString( "title", title );		// info panel title
	data->SetString( "type", "1" );			// show userdata from stringtable entry
	data->SetString( "msg", "motd" );		// use this stringtable entry
	data->SetBool( "unload", sv_motd_unload_on_dismissal.GetBool() );

	if ( gpGlobals->eLoadType != MapLoad_Background )
		pPlayer->ShowViewPortPanel( PANEL_INFO, true, data );

	data->deleteThis();
}

/*
===========
ClientPutInServer

called each time a player is spawned into the game
============
*/
void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	// Allocate a CBaseTFPlayer for pev, and call spawn
	CRTBMP_Player *pPlayer = CRTBMP_Player::CreatePlayer( "player", pEdict );
	pPlayer->SetPlayerName( playername );
}


void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	// Can't load games in CS!
	Assert( !bLoadGame );

	CRTBMP_Player *pPlayer = ToRTBMPPlayer( CBaseEntity::Instance( pEdict ) );
	FinishClientPutInServer( pPlayer );
}


/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	if (g_pGameRules) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "Raising The Bar: Arena";
}

// called by ClientKill and DeadThink
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	CRTBMP_Player *pPlayer = ToRTBMPPlayer( pEdict );

	if (pPlayer)
	{
		if (gpGlobals->curtime > pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME)
		{
			// respawn player
			pPlayer->Spawn();
		}
		else
		{
			pPlayer->SetNextThink( gpGlobals->curtime + 0.1f );
		}
	}
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
	// vanilla deathmatch
	CreateGameRulesObject( "CRTBMPRules" );
}