//=============== Copyright RTBMP Team, All rights reserved. ===================//
//
// Purpose: RTB:MP gamerules base class. It inherits HL2DM's code.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "rtbmp_gamerules.h"
#ifdef CLIENT_DLL
#include "c_rtbmp_player.h"
#include "c_team.h"
#define CTeam C_Team
#else
#include "team.h"
#include "rtbmp_player.h"
#include "mapbase/protagonist_system.h"
#endif

#include "rtbr_shareddefs.h"

#ifndef CLIENT_DLL
ConVar sv_rtbmp_spawn_protection_time( "sv_rtbmp_spawn_protection_time", "5", FCVAR_GAMEDLL | FCVAR_NOTIFY );
extern ConVar fraglimit;
extern ConVar mp_winlimit;
#endif
extern ConVar mp_teams_unbalance_limit;
extern ConVar mp_autoteambalance;

REGISTER_GAMERULES_CLASS( CRTBMPRules );

BEGIN_NETWORK_TABLE_NOBASE( CRTBMPRules, DT_RTBMPRules )
	#ifdef CLIENT_DLL
		RecvPropBool( RECVINFO( m_iGameType ) ),
	#else
		SendPropBool( SENDINFO( m_iGameType ) ),
	#endif
END_NETWORK_TABLE()


LINK_ENTITY_TO_CLASS( rtbmp_gamerules, CRTBMPGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( RTBMPGameRulesProxy, DT_RTBMPGameRulesProxy )

#ifdef CLIENT_DLL
void RecvProxy_RTBMPRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CRTBMPRules *pRules = RTBMPRules();
	Assert( pRules );
	*pOut = pRules;
}

BEGIN_RECV_TABLE( CRTBMPGameRulesProxy, DT_RTBMPGameRulesProxy )
RecvPropDataTable( "rtbmp_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_RTBMPRules ), RecvProxy_RTBMPRules )
END_RECV_TABLE()
#else
void* SendProxy_RTBMPRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CRTBMPRules *pRules = RTBMPRules();
	Assert( pRules );
	return pRules;
}

BEGIN_SEND_TABLE( CRTBMPGameRulesProxy, DT_RTBMPGameRulesProxy )
SendPropDataTable( "rtbmp_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_RTBMPRules ), SendProxy_RTBMPRules )
END_SEND_TABLE()
#endif

CRTBMPRules::CRTBMPRules()
{
	m_iGameType = GAMETYPE_DEATHMATCH;
	m_flStartBalancingTeamsAt = 30.0f;
	m_bPrintedUnbalanceWarning = false;
	m_flFoundUnbalancedTeamsTime = -1.0f;
	m_flAutoBalanceQueueTimeEnd = -1.0f;
	m_nAutoBalanceQueuePlayerIndex = -1;
	m_nAutoBalanceQueuePlayerScore = -1;
}

void CRTBMPRules::Think( void )
{
#ifndef CLIENT_DLL

	BaseClass::Think();

	float flLimit;

#if 0
	// todo: refactor to use existing gamerules functions IsDeathmatch(), IsTeamplay(), IsCoOp()
	if (GetGameType() > GAMETYPE_COOP)
		flLimit = mp_winlimit.GetFloat();
	else
#endif
		flLimit = fraglimit.GetFloat();

	if (flLimit)
	{
		if (IsTeamplay())
		{
			CTeam *pCombine = g_Teams[TEAM_COMBINE];
			CTeam *pRebels = g_Teams[TEAM_REBELS];

			if (pCombine->GetScore() >= flLimit || pRebels->GetScore() >= flLimit)
			{
				GoToIntermission();
				return;
			}
		}
		else
		{
			// check if any player is over the frag limit
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if (pPlayer && pPlayer->FragCount() >= flLimit)
				{
					GoToIntermission();
					return;
				}
			}
		}
	}

	if ( IsTeamplay() && m_flNextBalanceTeamsTime < gpGlobals->curtime )
	{
		BalanceTeams( true );
		m_flNextBalanceTeamsTime = gpGlobals->curtime + 1.0f;
	}

#endif
}

void CRTBMPRules::CreateStandardEntities( void )
{

#ifndef CLIENT_DLL
	// Create the entity that will send our data to the client.
	BaseClass::CreateStandardEntities();

#ifdef DBGFLAG_ASSERT
	CBaseEntity *pEnt =
#endif
		CBaseEntity::Create( "rtbmp_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
#endif
}

void CRTBMPRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	CRTBMP_Player *pRTBMPPlayer = ToRTBMPPlayer( pPlayer );

	if ( pRTBMPPlayer == NULL )
		return;

	const char *pCurrentModel = modelinfo->GetModelName( pPlayer->GetModel() );
	const char *szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_playermodel" );

	//If we're different.
	if ( stricmp( szModelName, pCurrentModel ) && pRTBMPPlayer->CanSwitchToModel( szModelName ) )
	{
		//Too soon, set the cvar back to what it was.
		//Note: this will make this function be called again
		//but since our models will match it'll just skip this whole dealio.
		if ( pRTBMPPlayer->GetNextModelChangeTime() >= gpGlobals->curtime )
		{
			char szReturnString[512];

			Q_snprintf( szReturnString, sizeof( szReturnString ), "cl_playermodel %s\n", pCurrentModel );
			engine->ClientCommand( pRTBMPPlayer->edict(), szReturnString );

			Q_snprintf( szReturnString, sizeof( szReturnString ), "Please wait %d more seconds before trying to switch.\n", (int)(pRTBMPPlayer->GetNextModelChangeTime() - gpGlobals->curtime) );
			ClientPrint( pRTBMPPlayer, HUD_PRINTTALK, szReturnString );
			return;
		}

		if ( HL2MPRules()->IsTeamplay() == false )
		{
			pRTBMPPlayer->SetPlayerModel();

			const char *pszCurrentModelName = modelinfo->GetModelName( pRTBMPPlayer->GetModel() );

			char szReturnString[128];
			Q_snprintf( szReturnString, sizeof( szReturnString ), "Your player model is: %s\n", pszCurrentModelName );

			ClientPrint( pRTBMPPlayer, HUD_PRINTTALK, szReturnString );
		}
		else
		{
#ifdef MAPBASE
			if ( szModelName[0] == '#' )
			{
				int nProtagonist = g_ProtagonistSystem.FindProtagonistIndex( szModelName + 1 );
				if ( nProtagonist != -1 )
				{
					int nTeam = g_ProtagonistSystem.GetProtagonist_Team( nProtagonist );
					if ( nTeam == TEAM_REBELS && !WouldChangeUnbalanceTeams( nTeam, pRTBMPPlayer->GetTeamNumber() ) )
					{
						pRTBMPPlayer->ChangeTeam( TEAM_REBELS );
					}
					else if ( nTeam == TEAM_COMBINE && !WouldChangeUnbalanceTeams( nTeam, pRTBMPPlayer->GetTeamNumber() ) )
					{
						pRTBMPPlayer->ChangeTeam( TEAM_COMBINE );
					}
					else if ( WouldChangeUnbalanceTeams( nTeam, pRTBMPPlayer->GetTeamNumber() ) )
					{
						char szReturnString[512];

						Q_snprintf( szReturnString, sizeof( szReturnString ), "cl_playermodel %s\n", pCurrentModel );
						engine->ClientCommand( pRTBMPPlayer->edict(), szReturnString );
						ClientPrint( pRTBMPPlayer, HUD_PRINTTALK, "Cannot change teams as they would be unbalanced." );
					}
				}
			}
			else
#endif

				if ( Q_stristr( szModelName, "models/human" ) )
				{
					pRTBMPPlayer->ChangeTeam( TEAM_REBELS );
				}
				else
				{
					pRTBMPPlayer->ChangeTeam( TEAM_COMBINE );
				}
		}
	}

	BaseClass::ClientSettingsChanged( pPlayer );

	const int nSkin = Q_atoi( engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_playermodel_skin" ) );
	const int nBodygroup = Q_atoi( engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_bodygroup" ) );
	pPlayer->SetSkin( nSkin );
	pPlayer->m_nBody = nBodygroup;
#endif
}

const char *CRTBMPRules::GetGameDescription( void )
{
	if (IsTeamplay())
	{
		if (GetGameType() == GAMETYPE_CTF)
			return "Capture The Flag";
	}

	return BaseClass::GetGameDescription();
}

bool CRTBMPRules::IsSprintInfinite( void )
{
	return true;
}

#ifndef CLIENT_DLL
float CRTBMPRules::FlSpawnProtectionTime( void )
{
	return sv_rtbmp_spawn_protection_time.GetFloat();
}
#endif

#ifdef GAME_DLL
extern int SwitchPlayersSort( CBaseMultiplayerPlayer *const *p1, CBaseMultiplayerPlayer *const *p2 );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRTBMPRules::BalanceTeams( bool bRequireSwitcheesToBeDead )
{
	if ( mp_autoteambalance.GetBool() == false )
	{
		return;
	}

	if ( !IsTeamplay() )
		return;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return;
#endif // _DEBUG || STAGING_ONLY

	// we don't balance for a period of time at the start of the game
	if ( gpGlobals->curtime < m_flStartBalancingTeamsAt )
	{
		return;
	}

	// wrap with this bool, indicates it's a round running switch and not a between rounds insta-switch
	if ( bRequireSwitcheesToBeDead )
	{
#if !defined(CSTRIKE_DLL) && !defined(RTBMP)
		// we don't balance if there is less than 60 seconds on the active timer
		CTeamRoundTimer *pActiveTimer = GetActiveRoundTimer();
		if ( pActiveTimer && pActiveTimer->GetTimeRemaining() < 60 )
		{
			return;
		}
#endif
	}

	int iHeaviestTeam = TEAM_UNASSIGNED, iLightestTeam = TEAM_UNASSIGNED;

	// Figure out if we're unbalanced
	if ( !AreTeamsUnbalanced( iHeaviestTeam, iLightestTeam ) )
	{
		m_flFoundUnbalancedTeamsTime = -1;
		m_bPrintedUnbalanceWarning = false;
		return;
	}

	if ( m_flFoundUnbalancedTeamsTime < 0 )
	{
		m_flFoundUnbalancedTeamsTime = gpGlobals->curtime;
	}

	// if teams have been unbalanced for X seconds, play a warning 
	if ( !m_bPrintedUnbalanceWarning && ((gpGlobals->curtime - m_flFoundUnbalancedTeamsTime) > 1.0) )
	{
		// print unbalance warning
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_auto_team_balance_in", "5" );
		m_bPrintedUnbalanceWarning = true;
	}

	// teams are unblanced, figure out some players that need to be switched

	CTeam *pHeavyTeam = GetGlobalTeam( iHeaviestTeam );
	CTeam *pLightTeam = GetGlobalTeam( iLightestTeam );

	Assert( pHeavyTeam && pLightTeam );

	int iNumSwitchesRequired = (pHeavyTeam->GetNumPlayers() - pLightTeam->GetNumPlayers()) / 2;

	// sort the eligible players and switch the n best candidates
	CUtlVector<CBaseMultiplayerPlayer *> vecPlayers;

	CBaseMultiplayerPlayer *pPlayer;

	int iScore;

	int i;
	for ( i = 0; i < pHeavyTeam->GetNumPlayers(); i++ )
	{
		pPlayer = ToBaseMultiplayerPlayer( pHeavyTeam->GetPlayer( i ) );

		if ( !pPlayer )
			continue;

		if ( !pPlayer->CanBeAutobalanced() )
			continue;

		// calculate a score for this player. higher is more likely to be switched
		iScore = pPlayer->CalculateTeamBalanceScore();

		pPlayer->SetTeamBalanceScore( iScore );

		vecPlayers.AddToTail( pPlayer );
	}

	// sort the vector
	vecPlayers.Sort( SwitchPlayersSort );

	int iNumEligibleSwitchees = iNumSwitchesRequired + 2;

	for ( int i = 0; i < vecPlayers.Count() && iNumSwitchesRequired > 0 && i < iNumEligibleSwitchees; i++ )
	{
		pPlayer = vecPlayers.Element( i );

		Assert( pPlayer );

		if ( !pPlayer )
			continue;

		if ( bRequireSwitcheesToBeDead == false || !pPlayer->IsAlive() || pLightTeam->GetNumPlayers() == 0 )
		{
			// We're trying to avoid picking a player that's recently
			// been auto-balanced by delaying their selection in the hope
			// that a better candidate comes along.
			if ( bRequireSwitcheesToBeDead )
			{
				int nPlayerTeamBalanceScore = pPlayer->CalculateTeamBalanceScore();

				// Do we already have someone in the queue?
				if ( m_nAutoBalanceQueuePlayerIndex > 0 )
				{
					// Is this player's score worse?
					if ( nPlayerTeamBalanceScore < m_nAutoBalanceQueuePlayerScore )
					{
						m_nAutoBalanceQueuePlayerIndex = pPlayer->entindex();
						m_nAutoBalanceQueuePlayerScore = nPlayerTeamBalanceScore;
					}
				}
				// Has this person been switched recently?
				else if ( nPlayerTeamBalanceScore < -10000 )
				{
					// Put them in the queue
					m_nAutoBalanceQueuePlayerIndex = pPlayer->entindex();
					m_nAutoBalanceQueuePlayerScore = nPlayerTeamBalanceScore;
					m_flAutoBalanceQueueTimeEnd = gpGlobals->curtime + 3.0f;

					continue;
				}

				// If this is the player in the queue...
				if ( m_nAutoBalanceQueuePlayerIndex == pPlayer->entindex() )
				{
					// Pass until their timer is up
					if ( m_flAutoBalanceQueueTimeEnd > gpGlobals->curtime )
						continue;
				}
			}

			pPlayer->ChangeTeam( iLightestTeam );
			pPlayer->SetLastForcedChangeTeamTimeToNow();

			m_nAutoBalanceQueuePlayerScore = -1;
			m_nAutoBalanceQueuePlayerIndex = -1;

			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_teambalanced_player" );
			if ( event )
			{
				event->SetInt( "player", pPlayer->entindex() );
				event->SetInt( "team", iLightestTeam );
				gameeventmanager->FireEvent( event );
			}

			// tell people that we've switched this player
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_player_was_team_balanced", pPlayer->GetPlayerName() );

			// tell the switched player, too, since they may not see the chat message
			ClientPrint( pPlayer, HUD_PRINTCENTER, "#game_player_message_balance", pLightTeam->GetName() );

			iNumSwitchesRequired--;
		}
	}
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: returns true if we should even bother to do balancing stuff
//-----------------------------------------------------------------------------
bool CRTBMPRules::ShouldBalanceTeams( void )
{
	if ( !IsTeamplay() )
		return false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return false;
#endif // _DEBUG || STAGING_ONLY

	if ( mp_teams_unbalance_limit.GetInt() <= 0 )
		return false;

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		return false;
#endif // TF_DLL

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the passed team change would cause unbalanced teams
//-----------------------------------------------------------------------------
bool CRTBMPRules::WouldChangeUnbalanceTeams( int iNewTeam, int iCurrentTeam )
{
	// players are allowed to change to their own team
	if ( iNewTeam == iCurrentTeam )
		return false;

	// if mp_teams_unbalance_limit is 0, don't check
	if ( !ShouldBalanceTeams() )
		return false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return false;
#endif // _DEBUG || STAGING_ONLY

	// if they are joining a non-playing team, allow
	if ( iNewTeam < FIRST_GAME_TEAM )
		return false;

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		return false;
#endif // TF_DLL

	CTeam *pNewTeam = GetGlobalTeam( iNewTeam );

	if ( !pNewTeam )
	{
		Assert( 0 );
		return true;
	}

	// add one because we're joining this team
	int iNewTeamPlayers = pNewTeam->GetNumPlayers() + 1;

	// for each game team
	int i = FIRST_GAME_TEAM;

	CTeam *pTeam;

	for ( pTeam = GetGlobalTeam( i ); pTeam != NULL; pTeam = GetGlobalTeam( ++i ) )
	{
		if ( pTeam == pNewTeam )
			continue;

		int iNumPlayers = pTeam->GetNumPlayers();

		if ( i == iCurrentTeam )
		{
			iNumPlayers = MAX( 0, iNumPlayers - 1 );
		}

		if ( (iNewTeamPlayers - iNumPlayers) > mp_teams_unbalance_limit.GetInt() )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRTBMPRules::AreTeamsUnbalanced( int &iHeaviestTeam, int &iLightestTeam )
{
#if !defined(CLIENT_DLL) && !defined(RTBMP)
	if ( IsInCommentaryMode() )
		return false;
#endif

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		return false;
#endif // TF_DLL

	int iMostPlayers = 0;
	int iLeastPlayers = MAX_PLAYERS + 1;

	int i = FIRST_GAME_TEAM;

	for ( CTeam *pTeam = GetGlobalTeam( i ); pTeam != NULL; pTeam = GetGlobalTeam( ++i ) )
	{
		int iNumPlayers = pTeam->GetNumPlayers();

		if ( iNumPlayers < iLeastPlayers )
		{
			iLeastPlayers = iNumPlayers;
			iLightestTeam = i;
		}

		if ( iNumPlayers > iMostPlayers )
		{
			iMostPlayers = iNumPlayers;
			iHeaviestTeam = i;
		}
	}

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	if ( IsInArenaMode() && tf_arena_use_queue.GetBool() )
	{
		if ( iMostPlayers == 0 && iMostPlayers == iLeastPlayers )
			return true;

		if ( iMostPlayers != iLeastPlayers )
			return true;

		return false;
	}
#endif

	if ( (iMostPlayers - iLeastPlayers) > mp_teams_unbalance_limit.GetInt() )
	{
		return true;
	}

	return false;
}