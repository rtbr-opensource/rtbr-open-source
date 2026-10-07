//========= Copyright RTBMP Team, All rights reserved. ============//
//
// Purpose:		Player for RTBMP.
//
//=============================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "rtbmp_player.h"
#include "rtbmp_gamerules.h"
#include "gamestats.h"
#include "team.h"
#include "particle_parse.h"
#include "rtbr_shareddefs.h"
#include "maprules.h"
#ifdef RTBMP
#include "rtbmp_player_shared.h"
#endif
#include "mapbase/protagonist_system.h"

#define	HL2_WALK_SPEED hl2_walkspeed.GetFloat()
#define	HL2_NORM_SPEED hl2_normspeed.GetFloat()
#define	HL2_SPRINT_SPEED hl2_sprintspeed.GetFloat()
// afterburn convars
#define AFTERBURN_DURATION 3
#define AFTERBURN_TICK 0.5
#define AFTERBURN_DAMAGE 3

LINK_ENTITY_TO_CLASS( player, CRTBMP_Player );

IMPLEMENT_SERVERCLASS_ST( CRTBMP_Player, DT_RTBMP_Player )
	SendPropFloat( SENDINFO( m_flSpawnProtectTime ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CRTBMP_Player )
DEFINE_FIELD( m_flLastIgniteTick, FIELD_FLOAT ),
END_DATADESC()

BEGIN_ENT_SCRIPTDESC( CRTBMP_Player, CHL2MP_Player, "Raising The Bar: Arena Player" )
END_SCRIPTDESC();

extern CSuitPowerDevice SuitDeviceSprint;
extern ConVar sk_player_head;
extern ConVar hl2_walkspeed; 
extern ConVar hl2_normspeed;
extern ConVar hl2_sprintspeed;

extern int g_iLastCitizenModel;
extern int g_iLastCombineModel;
// TODO: turn these hardcoded numbers into constants
extern const char *g_ppszRandomCitizenModels[15];
extern const char *g_ppszRandomCombineModels[6];

CRTBMP_Player::CRTBMP_Player( void )
{
	m_flSpawnProtectTime = 0.0f;
	m_flIgniteBegin = 0.0f;
	m_flLastIgniteTick = 0.0f;
	m_bHasForcedModel = false;
}

void CRTBMP_Player::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "Player.HitSound_Light" );
	PrecacheScriptSound( "Player.HitSound_Heavy" );
	PrecacheScriptSound( "Player.HitSound_Armour" );
	PrecacheScriptSound( "Player.HitSound_ArmourBreak" );
	PrecacheScriptSound( "Player.KillSound" );

	PrecacheParticleSystem( "shield_impact" );
}

//-----------------------------------------------------------------------------
// Purpose: Called the first time the player's created
//-----------------------------------------------------------------------------
void CRTBMP_Player::InitialSpawn( void )
{
	BaseClass::InitialSpawn();

	if ( RTBMPRules()->IsCoOp() )
	{
		if ( g_CoopObjective )
			g_CoopObjective->SendMatchStatusToPlayer( this );

		if ( RTBMPRules()->IsAcceptingReadyVote() )
			RTBMPRules()->SetPlayerReady( entindex(), false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets HL2 specific defaults.
//-----------------------------------------------------------------------------
void CRTBMP_Player::Spawn( void )
{
	BaseClass::Spawn();

	if (RTBMPRules()->FlSpawnProtectionTime() > 0)
	{
		m_flSpawnProtectTime = gpGlobals->curtime + RTBMPRules()->FlSpawnProtectionTime();
		SetContextThink( &CRTBMP_Player::DisableSpawnProtection, m_flSpawnProtectTime, "SpawnProtectionContext" );
		m_nRenderFX = kRenderFxDistort;
	}

	if (CanAlwaysRun())
	{
		StartSprinting();
	}

	// Have we spawned inside a physics prop? If so, temporarily disable our physics collisions.
	trace_t tr;
	UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin(),
		GetPlayerMins(), GetPlayerMaxs(), MASK_PLAYERSOLID, this, COLLISION_GROUP_NONE, &tr );
	if (tr.fraction < 1.0f)
	{
		CPhysicsProp *pProp = dynamic_cast<CPhysicsProp *>( tr.m_pEnt );
		if (pProp)
		{
			m_Local.m_bStuckSpawn = true;
		}
	}

	// Hide our HUD on background maps
	if ( gpGlobals->eLoadType == MapLoad_Background && !IsBot() )
	{
		m_Local.m_iHideHUD |= HIDEHUD_ALL;
	}

	const int nSkin = Q_atoi( engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel_skin" ) );
	const int nBodygroup = Q_atoi( engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_bodygroup" ) );
	SetSkin( nSkin );
	m_nBody = nBodygroup;

	const char *szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

	if ( RTBMPRules()->IsCoOp() && 
		RTBMPRules()->GetForcedPlayerModelTeam() != TEAM_UNASSIGNED &&
		RTBMPRules()->GetForcedPlayerModelTeam() != GetDesiredPlayerModelTeam( szModelName ) )
	{
		SetPlayerModel();
	}
}

void CRTBMP_Player::PostThink( void )
{
	BaseClass::PostThink();

	// Re-enable sprint when we're getting out of the duck state
	if (!IsSprinting() &&
		!(m_nButtons & IN_SPEED) &&
		!IsDucked() && !IsDucking() &&
		CanAlwaysRun() &&
		(GetGroundEntity() != NULL))
	{
		StartSprinting();
	}

	if (IsAlive())
		TakeIgniteDamage();

	// Re-enable the player's physics collisions when they leave the prop.
	if (m_Local.m_bStuckSpawn)
	{
		trace_t tr;
		UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin(),
			GetPlayerMins(), GetPlayerMaxs(), MASK_PLAYERSOLID, this, COLLISION_GROUP_NONE, &tr );
		if (tr.fraction == 1.0f)
		{
			m_Local.m_bStuckSpawn = false;
		}
	}

	if ( m_bBeingXenHealed && gpGlobals->curtime > m_flXenHealExpireTimer )
	{
		XenHealTimerExpired();
	}

	if ( IsBot() && RTBMPRules()->CanPlayerVoteReady( this ) )
	{
		RTBMPRules()->SetPlayerReady( entindex(), true );
	}
}

bool CRTBMP_Player::ClientCommand( const CCommand &args )
{
	if ( FStrEq( args[0], "player_ready_toggle" ) && RTBMPRules()->IsCoOp() )
	{
		if ( RTBMPRules()->CanPlayerVoteReady( this ) )
		{
			RTBMPRules()->SetPlayerReady( entindex(), true );
		}
		return true;
	}

	return BaseClass::ClientCommand( args );
}

//=========================================================
// TraceAttack
//=========================================================
void CRTBMP_Player::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	CTakeDamageInfo info = inputInfo;

	if ( ( m_takedamage && ptr->hitgroup == HITGROUP_HEAD ) &&
		( ( info.GetAttacker() && info.GetAttacker()->IsNPC() ) || info.GetInflictor()->ClassMatches( "weapon_gauss" ) ) )
	{
		// nullify whatever headshot damage this attack would've done
		info.ScaleDamage( 1.0f / sk_player_head.GetFloat() );
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

void CRTBMP_Player::FireBullets( const FireBulletsInfo_t &info )
{
	BaseClass::FireBullets( info );
	DisableSpawnProtection();
}

void CRTBMP_Player::ChangeTeam( int iTeam )
{
	int iOldTeam = GetTeamNumber();

	BaseClass::ChangeTeam( iTeam );

	if ( GetTeamNumber() == TEAM_SPECTATOR && RTBMPRules()->IsCoOp() && RTBMPRules()->IsPlayerReady( this ) )
	{
		RTBMPRules()->SetPlayerReady( entindex(), false );
	}
	else if ( iOldTeam == TEAM_SPECTATOR && GetTeamNumber() != TEAM_SPECTATOR )
	{
		// instantly respawn if we're coming out of spectator
		Spawn();
	}
}

void CRTBMP_Player::Event_Killed( const CTakeDamageInfo &info )
{
	FlashlightTurnOff();
	
	BaseClass::Event_Killed( info );

	CBaseEntity *pAttacker = info.GetAttacker();

	if (pAttacker && RTBMPRules()->GetGameType() == GAMETYPE_DEATHMATCH)
	{
		int iScoreToAdd = 1;

		if (pAttacker == this)
		{
			iScoreToAdd = -1;
		}

		GetGlobalTeam( pAttacker->GetTeamNumber() )->AddScore( iScoreToAdd );
	}

	DropFlag();
}

void CRTBMP_Player::SetIgniteBegin( CBaseCombatCharacter *pAttacker ) {
	m_flIgniteBegin = gpGlobals->curtime;
	m_Local.m_bOnFireImmolator = true;
	m_hAfterburnAttacker = pAttacker;
	TakeIgniteDamage();
}

void CRTBMP_Player::RemoveAfterburn( void ) {
	m_flIgniteBegin = 0.0f;
	m_Local.m_bOnFireImmolator = false;
	m_hAfterburnAttacker = NULL;
	SetImmolatorBurning( false );
}

//--------------------------------------------------
// Purpose: Take damage when ignited by Immolator.
//--------------------------------------------------
void CRTBMP_Player::TakeIgniteDamage( void ) {
	if (m_Local.m_bOnFireImmolator && gpGlobals->curtime <= m_flIgniteBegin + AFTERBURN_DURATION) {
		if (gpGlobals->curtime >= m_flLastIgniteTick + AFTERBURN_TICK) {
			// record the NPC inflictor for death notices
			CBaseEntity *pInflictor = this;
			if ( m_hAfterburnAttacker && m_hAfterburnAttacker->IsNPC() )
				pInflictor = m_hAfterburnAttacker; 

			TakeDamage( CTakeDamageInfo( pInflictor, m_hAfterburnAttacker ? m_hAfterburnAttacker : this, AFTERBURN_DAMAGE, DMG_BURN ) );
			m_flLastIgniteTick = gpGlobals->curtime;
		}
	}
	else {
		RemoveAfterburn();
	}
}

int CRTBMP_Player::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	// return here if the player is in the respawn grace period, does not count self-damage (e.g. fall damage) or world damage.
	if (IsSpawnProtected() && !(inputInfo.GetAttacker() == ToBasePlayer( this ) 
		|| (inputInfo.GetAttacker() && !(inputInfo.GetAttacker()->IsPlayer() || inputInfo.GetAttacker()->IsNPC()))))
		return 0;

	bool bHadArmor = ArmorValue() > 0;

	// Determine if the player died because of the damage taken.
	int retVal = BaseClass::OnTakeDamage( inputInfo );

	if ( retVal != 0 )
	{
		int bitsDamage = inputInfo.GetDamageType();
		// Send the damage message to the client for the hud damage indicator
		// Try and figure out where the damage is coming from
		Vector vecDamageOrigin = inputInfo.GetReportedPosition();

		// If we didn't get an origin to use, try using the attacker's origin
		if ( vecDamageOrigin == vec3_origin && inputInfo.GetInflictor() )
		{
			vecDamageOrigin = inputInfo.GetInflictor()->GetAbsOrigin();
		}

		CSingleUserRecipientFilter user( this );
		UserMessageBegin( user, "Damage" );
		WRITE_SHORT( clamp( (int)inputInfo.GetDamage(), 0, 32000 ) );
		WRITE_LONG( inputInfo.GetDamageType() );
		// Tell the client whether they should show it in the indicator
		if ( bitsDamage != DMG_GENERIC && !(bitsDamage & (DMG_DROWN | DMG_FALL | DMG_BURN)) )
		{
			WRITE_BOOL( true );
			WRITE_VEC3COORD( vecDamageOrigin );
		}
		else
		{
			WRITE_BOOL( false );
		}
		MessageEnd();

		if ( inputInfo.GetAttacker() && inputInfo.GetAttacker()->IsPlayer() && inputInfo.GetAttacker() != this && IsAlive() )
		{
			// always play sounds on armour impact from the hit player, so others can hear
			CPASAttenuationFilter filter( this, "Player.HitSound_Armour" );
			// except the player, of course
			filter.RemoveRecipient( this );

			if ( bHadArmor && ArmorValue() > 0 )
			{
				EmitSound( filter, entindex(), "Player.HitSound_Armour" );
				DispatchParticleEffect( "shield_impact", inputInfo.GetDamagePosition(), vec3_angle );
			}
			else if ( bHadArmor && ArmorValue() <= 0 )
			{
				EntityMessageBegin( this, true );
				WRITE_BYTE(SHIELD_BREAK_FX);
				MessageEnd();

				EmitSound( filter, entindex(), "Player.HitSound_ArmourBreak" );
			}
			
#ifdef RTBMP
			EntityMessageBegin( this, true );
			WRITE_BYTE(DAMAGE_FLINCH);
			WRITE_LONG(LastHitGroup());
			MessageEnd();
#endif
		}
	}

	return retVal;
}

bool CRTBMP_Player::CanAlwaysRun( void )
{
	return atoi( engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_alwaysrun" ) ) != 0 &&
		RTBMPRules()->IsSprintInfinite();
}

extern ConVar sv_maxspeed;

//----------------------------------------
// Purpose: Override to handle always run
//----------------------------------------
void CRTBMP_Player::HandleSpeedChanges( CMoveData *mv )
{
	if (CanAlwaysRun())
	{
		int nChangedButtons = mv->m_nButtons ^ mv->m_nOldButtons;

		const bool bWantSprint = ( CanSprint() && IsSuitEquipped() && !( mv->m_nButtons & IN_SPEED ) );
		const bool bIsStandingNotSprinting = !( m_Local.m_bDucked || m_Local.m_bDucking ) && !m_HL2Local.m_bNewSprinting;
		const bool bWantsToChangeSprinting = ( m_HL2Local.m_bNewSprinting != bWantSprint ) && (( nChangedButtons & IN_SPEED ) != 0 || bIsStandingNotSprinting);

		bool bSprinting = m_HL2Local.m_bNewSprinting;
		if ( bWantsToChangeSprinting )
		{
			if ( bWantSprint )
			{
				bSprinting = true;
			}
			else
			{
				bSprinting = false;
			}
		}

		if ( m_HL2Local.m_flSuitPower < 0.01 )
		{
			bSprinting = false;
		}

		bool bWantWalking;

		if ( IsSuitEquipped() )
		{
			bWantWalking = ( mv->m_nButtons & IN_WALK ) && !bSprinting && !( mv->m_nButtons & IN_DUCK );
		}
		else
		{
			bWantWalking = true;
		}

		if ( bWantWalking )
		{
			bSprinting = false;
		}

		m_HL2Local.m_bNewSprinting = bSprinting;

		if ( bSprinting )
		{
			mv->m_flClientMaxSpeed = HL2_SPRINT_SPEED;
		}
		else if ( bWantWalking )
		{
			mv->m_flClientMaxSpeed = HL2_WALK_SPEED;
		}
		else
		{
			mv->m_flClientMaxSpeed = HL2_NORM_SPEED;
		}

		mv->m_flMaxSpeed = sv_maxspeed.GetFloat();
	}
	else
	{
		BaseClass::HandleSpeedChanges(mv);
	}
}

void CRTBMP_Player::ReduceTimers( CMoveData *mv )
{
	bool bSprinting = mv->m_flClientMaxSpeed == HL2_SPRINT_SPEED;

	if (bSprinting && !RTBMPRules()->IsSprintInfinite())
	{
		SuitPower_AddDevice( SuitDeviceSprint );
	}
	else
	{
		SuitPower_RemoveDevice( SuitDeviceSprint );
	}

	SuitPower_Update();
}

void CRTBMP_Player::DisableSpawnProtection( void )
{
	// when the player spawns using lmb/rmb, depending on their starting weapon and/or server model's
	// draw frames, they may be able to instantly attack when spawning - this is evident when using
	// the physcannon. we need a grace period to ensure the player doesn't accidentally disable their
	// own spawn protection.
	float flElapsedSpawnProtectTime = gpGlobals->curtime + RTBMPRules()->FlSpawnProtectionTime() - m_flSpawnProtectTime;
	if ( flElapsedSpawnProtectTime > 0.25f )
	{
		m_flSpawnProtectTime = gpGlobals->curtime - 0.1f;
		m_nRenderFX = kRenderFxNone;
		
		m_Local.m_bStuckSpawn = false;
		SetCollisionGroup( COLLISION_GROUP_PLAYER );
	}
}

void CRTBMP_Player::StartXenHealExpireTimer( void )
{
	m_flXenHealExpireTimer = gpGlobals->curtime + 0.75f;
	m_bBeingXenHealed = true;
}

void CRTBMP_Player::XenHealTimerExpired( void )
{
	m_bBeingXenHealed = false;

	CSingleUserRecipientFilter filter( this );
	UserMessageBegin( filter, "XenHeal" );
	WRITE_BYTE( entindex() );
	WRITE_BOOL( false );
	MessageEnd();
}

unsigned int CRTBMP_Player::PlayerSolidMask( bool brushOnly ) const
{
	int mask = 0;

	if ( HL2MPRules()->IsTeamplay() )
	{
		switch ( GetTeamNumber() )
		{
		case TEAM_REBELS:
			mask = CONTENTS_TEAM1;
			break;

		case TEAM_COMBINE:
			mask = CONTENTS_TEAM2;
			break;
		}
	}

	return ( mask | BaseClass::PlayerSolidMask( brushOnly ) );
}

unsigned int CRTBMP_Player::PhysicsSolidMaskForEntity( void ) const
{
	// Disable all physics collisions if the player's stuck in a prop when spawning.
	// This needs to be tested thoroughly!!
	return m_Local.m_bStuckSpawn ? CONTENTS_EMPTY : BaseClass::PhysicsSolidMaskForEntity();
}

int CRTBMP_Player::GetDesiredPlayerModelTeam( const char *pszModelName )
{
	if ( Q_stristr( pszModelName, "models/human" ) || Q_stristr( pszModelName, "gordon" ) )
	{
		return TEAM_REBELS;
	}

	return TEAM_COMBINE;
}

bool CRTBMP_Player::CanSwitchToModel( const char *pszModelName )
{
	// We can always switch if the game isn't forcing us onto a specific faction.
	if ( RTBMPRules()->GetForcedPlayerModelTeam() == TEAM_UNASSIGNED )
		return true;

	return RTBMPRules()->GetForcedPlayerModelTeam() == GetDesiredPlayerModelTeam( pszModelName );
}

void CRTBMP_Player::SetPlayerModel( void )
{
	const char *szModelName = NULL;
	const char *pszCurrentModelName = modelinfo->GetModelName( GetModel() );
	const char *pszDesiredModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

	if ( CanSwitchToModel( pszDesiredModelName ) )
	{
		szModelName = pszDesiredModelName;
	}
	else if ( ValidatePlayerModel( pszCurrentModelName ) )
	{
		szModelName = pszCurrentModelName;
	}
	else
	{
		// in co-op when the player just spawns the current model doesn't exist,
		// so we need to note the player's preference here. this will get
		// checked later on, so the player won't be able to get past the forced model system.
		szModelName = pszDesiredModelName;
	}

	if ( ValidatePlayerModel( szModelName ) == false )
	{
		char szReturnString[512];

		if ( ValidatePlayerModel( pszCurrentModelName ) == false )
		{
			pszCurrentModelName = "models/Combine_Soldier.mdl";
		}

		if ( !HasForcedModel() )
		{
			Q_snprintf( szReturnString, sizeof( szReturnString ), "cl_playermodel %s\n", pszCurrentModelName );
			engine->ClientCommand( edict(), szReturnString );
		}

		szModelName = pszCurrentModelName;
	}

	if ( GetTeamNumber() == TEAM_COMBINE )
	{
		int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );

		g_iLastCombineModel = (g_iLastCombineModel + 1) % nHeads;
		szModelName = g_ppszRandomCombineModels[g_iLastCombineModel];

		m_iModelType = TEAM_COMBINE;
	}
	else if ( GetTeamNumber() == TEAM_REBELS )
	{
		int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );

		g_iLastCitizenModel = (g_iLastCitizenModel + 1) % nHeads;
		szModelName = g_ppszRandomCitizenModels[g_iLastCitizenModel];

		m_iModelType = TEAM_REBELS;
	}
	else
	{
		if ( Q_strlen( szModelName ) == 0 )
		{
			szModelName = g_ppszRandomCitizenModels[0];
		}

		m_iModelType = GetDesiredPlayerModelTeam( szModelName );
	}

	// Set a forced model if the mapper has set one.
	int iForcedModelTeam = RTBMPRules()->GetForcedPlayerModelTeam();
	if ( RTBMPRules()->IsCoOp() && iForcedModelTeam != TEAM_UNASSIGNED && m_iModelType != iForcedModelTeam )
	{
		if ( iForcedModelTeam == TEAM_REBELS )
		{
			int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );

			g_iLastCitizenModel = (g_iLastCitizenModel + 1) % nHeads;
			szModelName = g_ppszRandomCitizenModels[g_iLastCitizenModel];

			m_iModelType = TEAM_REBELS;
		}
		else
		{
			int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );

			g_iLastCombineModel = (g_iLastCombineModel + 1) % nHeads;
			szModelName = g_ppszRandomCombineModels[g_iLastCombineModel];

			m_iModelType = TEAM_COMBINE;
		}
		m_bHasForcedModel = true;
	}

#ifdef MAPBASE
	if ( szModelName[0] == '#' )
	{
		// Protagonist name. Was already validated above
		SetProtagonist( szModelName + 1 );
	}
	else
	{
		ResetProtagonist();
	}
#endif

	int modelIndex = modelinfo->GetModelIndex( szModelName );

	if ( modelIndex == -1 )
	{
		szModelName = "models/Combine_Soldier.mdl";
		m_iModelType = TEAM_COMBINE;

		char szReturnString[512];

		if ( !HasForcedModel() )
		{
			Q_snprintf( szReturnString, sizeof( szReturnString ), "cl_playermodel %s\n", szModelName );
			engine->ClientCommand( edict(), szReturnString );
		}
	}

#ifdef MAPBASE
	if ( GetProtagonistIndex() == -1 )
	{
		// Find out if we have a protagonist for this model
		const char *pszProtagonistName = g_ProtagonistSystem.FindProtagonistByModel( szModelName );
		if ( pszProtagonistName )
		{
			SetProtagonist( pszProtagonistName );

			// Fall back if not accepted
			if ( GetProtagonistIndex() <= -1 )
				SetModel( szModelName );
		}
		else
		{
			SetModel( szModelName );
		}
	}
#else
	SetModel( szModelName );
#endif

	SetupPlayerSoundsByModel( szModelName );

	m_flNextModelChangeTime = gpGlobals->curtime + MODEL_CHANGE_INTERVAL;
}

void CRTBMP_Player::SetPlayerTeamModel( void )
{
	BaseClass::SetPlayerTeamModel();
}

bool CRTBMP_Player::HandleCommand_JoinTeam( int team )
{
	if ( RTBMPRules() &&
		RTBMPRules()->IsTeamplay() &&
		team > TEAM_SPECTATOR && team <= TEAM_REBELS &&
		RTBMPRules()->WouldChangeUnbalanceTeams( team, GetTeamNumber() ) )
	{
		ClientPrint( this, HUD_PRINTTALK, "Cannot change teams as they would be unbalanced." );
		return false;
	}
		
	return BaseClass::HandleCommand_JoinTeam( team );
}

void CRTBMP_Player::PickupFlag( CCaptureFlag *flag )
{
	if (GetFlag())
		return;

	flag->SetAbsOrigin( GetAbsOrigin() ); // bad, will fix later
	flag->SetOwnerEntity( this );
	flag->SetParent( this );
	flag->OnFlagPickedUp();
	m_hFlag = flag;
}

void CRTBMP_Player::DropFlag( void )
{
	if (!GetFlag())
		return;

	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, MAX_TRACE_LENGTH ), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );
	m_hFlag->SetAbsOrigin( tr.endpos );
	m_hFlag->SetOwnerEntity( NULL );
	m_hFlag->SetParent( NULL );
	m_hFlag->OnFlagDropped();
	m_hFlag = NULL;
}

void CRTBMP_Player::ReturnFlag( void )
{
	if (!GetFlag())
		return;

	m_hFlag = NULL;
}