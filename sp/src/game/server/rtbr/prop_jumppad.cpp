//========= Copyright (c) RTBR Team, 2025 ============//
//
// Purpose: A jump pad similar to that seen in Half-Life.
//
//====================================================//

#include "cbase.h"
#include "props.h"
#include "weapon_physcannon.h"
//#include "saverestore_utlvector.h"
#include "player.h"
#include "movevars_shared.h"
#include "filters.h"

#define JUMP_PAD_MODEL "models/props_xen/jumppad/jumppad.mdl"

class CPropJumppad : public CDynamicProp
{
	DECLARE_CLASS( CPropJumppad, CDynamicProp );

public:
	DECLARE_DATADESC();

	CPropJumppad();
	virtual void	Spawn( void );
	virtual void	Precache( void );

	void			LaunchThink( void );

	virtual void	DrawDebugGeometryOverlays( void );
	virtual int		DrawDebugTextOverlays( void );

	bool			PassesFilters( CBaseEntity *pOther );

	static const char *s_szLaunchThinkContext;

	string_t	m_iFilterName;
	CHandle<class CBaseFilter>	m_hFilter;

protected:
	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );
	void			InputSetSpeed( inputdata_t &in );
	void			InputSetPhysicsSpeed( inputdata_t &in );
	void			InputSetLaunchTarget( inputdata_t &in );

	void			LaunchByTarget( CBaseEntity *pVictim, CBaseEntity *pTarget );
	Vector			CalculateLaunchVector( CBaseEntity *pVictim, CBaseEntity *pTarget );
	Vector			CalculateLaunchVectorPreserve( Vector vecInitialVelocity, CBaseEntity *pVictim, CBaseEntity *pTarget, bool bForcePlayer = false );

	void			LaunchByDirection( CBaseEntity *pVictim );
	void			OnLaunchedVictim( CBaseEntity *pVictim );

	// we might need to use m_flRefireDelay but for now just leave it be. reference tf sdk
	//float m_flRefireDelay;
	float m_flVelocity;
	float m_flPhysicsVelocity;
	QAngle m_vecLaunchAngles;
	string_t m_strLaunchTarget;
	int m_ExactVelocityChoice;
	bool m_bUseExactVelocity;
	float m_flEntryAngleTolerance;
	EHANDLE m_hLaunchTarget;
	bool m_bApplyAngularImpulse;
	bool m_bPlayersPassTriggerFilters;
	// this is useful in the future but maybe not now
	//float m_flAirControlSupressionTime;
	//bool m_bDirectionSuppressAirControl;
	float m_flNextJumpDelta;

	COutputEvent m_OnTrigger;
};

const char *CPropJumppad::s_szLaunchThinkContext = "launch";

ConVar catapult_physics_drag_boost( "catapult_physics_drag_boost", "2.1", FCVAR_REPLICATED );

BEGIN_DATADESC( CPropJumppad )
DEFINE_KEYFIELD( m_flNextJumpDelta, FIELD_FLOAT, "nextjumpdelta" ),
DEFINE_KEYFIELD( m_strLaunchTarget, FIELD_STRING, "target" ),
DEFINE_KEYFIELD( m_flVelocity, FIELD_FLOAT, "speed" ),
DEFINE_KEYFIELD( m_flPhysicsVelocity, FIELD_FLOAT, "physicsSpeed" ),
DEFINE_KEYFIELD( m_vecLaunchAngles, FIELD_VECTOR, "launchDirection" ),
DEFINE_KEYFIELD( m_bUseExactVelocity, FIELD_BOOLEAN, "useExactVelocity" ),
DEFINE_KEYFIELD( m_ExactVelocityChoice, FIELD_INTEGER, "exactVelocityChoiceType" ),
DEFINE_KEYFIELD( m_bApplyAngularImpulse, FIELD_BOOLEAN, "applyAngularImpulse" ),

DEFINE_KEYFIELD( m_flEntryAngleTolerance, FIELD_FLOAT, "EntryAngleTolerance" ),
//DEFINE_KEYFIELD( m_flAirControlSupressionTime, FIELD_FLOAT, "AirCtrlSupressionTime" ),
//DEFINE_KEYFIELD( m_bDirectionSuppressAirControl, FIELD_BOOLEAN, "DirectionSuppressAirControl" ),
DEFINE_KEYFIELD( m_iFilterName, FIELD_STRING, "filtername" ),

DEFINE_FIELD( m_hFilter, FIELD_EHANDLE ),
DEFINE_FIELD( m_hLaunchTarget, FIELD_EHANDLE ),
//DEFINE_FIELD( m_flRefireDelay, FIELD_TIME ),

DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeed", InputSetSpeed ),
DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPhysicsSpeed", InputSetPhysicsSpeed ),
DEFINE_INPUTFUNC( FIELD_STRING, "SetLaunchTarget", InputSetLaunchTarget ),

DEFINE_OUTPUT( m_OnTrigger, "OnTrigger" ),

DEFINE_THINKFUNC( LaunchThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( prop_jumppad, CPropJumppad );

CPropJumppad::CPropJumppad()
{
	//Defaulting to true;
	m_bApplyAngularImpulse = true;
	//m_flAirControlSupressionTime = -1.0f;
}

void CPropJumppad::Spawn()
{
	SetModelName( AllocPooledString( JUMP_PAD_MODEL ) );
	Precache();
	SetModel( STRING( GetModelName() ) );
	m_iszDefaultAnim = AllocPooledString( "idle" ); // since we know the model we'll know the default animation, we need to set it here

	BaseClass::Spawn();

	// need a separate think context as setting an animation runs a think function
	RegisterThinkContext( s_szLaunchThinkContext );
	SetContextThink( &CPropJumppad::LaunchThink, gpGlobals->curtime + 0.1f, s_szLaunchThinkContext );

	m_hLaunchTarget = gEntList.FindEntityByName( NULL, m_strLaunchTarget );

	// Get a handle to my filter entity if there is one
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName ));
	}
}

void CPropJumppad::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}

#define MAX_ENTS 4 // surely there won't be more than this many entities touching the jump pad right

void CPropJumppad::LaunchThink()
{
	Vector vOrigin = GetAbsOrigin();
	
	// 64x64x32 "trigger" box
	Vector vMins = Vector( vOrigin.x - 32, vOrigin.y - 32, vOrigin.z + 12 );
	Vector vMaxs = Vector( vOrigin.x + 32, vOrigin.y + 32, vOrigin.z + 44 );

	CBaseEntity *pTouchingEnts[MAX_ENTS] = {}; // initialise all array elements to null pointers
	
	UTIL_EntitiesInBox( pTouchingEnts, MAX_ENTS, vMins, vMaxs, 0 );

	bool bTriggered = false;
	
	for (int i = 0; i < MAX_ENTS; i++)
	{
		if (!pTouchingEnts[i])
			continue;

		if (!PassesFilters( pTouchingEnts[i] ))
			continue;

		bool bValidObject = false;

		// can be a player or NPC
		if (pTouchingEnts[i]->IsPlayer() || pTouchingEnts[i]->IsNPC())
			bValidObject = true;
		
		// can be a physics object
		if (pTouchingEnts[i]->VPhysicsGetObject() && !(pTouchingEnts[i]->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD))
		{
			bValidObject = true;
		}
		
		if (bValidObject)
		{
			// Get the target
			CBaseEntity *pLaunchTarget = m_hLaunchTarget;

			// See if we're attempting to hit a target
			if (pLaunchTarget)
			{
				LaunchByTarget( pTouchingEnts[i], pLaunchTarget );
			}
			else
			{
				LaunchByDirection( pTouchingEnts[i] );
			}

			bTriggered = true;
		}
	}

	if (bTriggered)
		SetNextThink( gpGlobals->curtime + m_flNextJumpDelta, s_szLaunchThinkContext );
	else
		SetNextThink( gpGlobals->curtime + 0.05f, s_szLaunchThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: calculates the launch vector between the entity that touched the
//			catapult trigger and the catapult target
//-----------------------------------------------------------------------------
Vector CPropJumppad::CalculateLaunchVector( CBaseEntity *pVictim, CBaseEntity *pTarget )
{
	// Find where we're going
	Vector vecSourcePos = pVictim->GetAbsOrigin();
	Vector vecTargetPos = pTarget->GetAbsOrigin();

	// If victim is player, adjust target position so player's center will hit the target
	if (pVictim->IsPlayer())
	{
		vecTargetPos.z -= 32.0f;
	}

	float flSpeed = (pVictim->IsPlayer() || pVictim->IsNPC()) ? (float)m_flVelocity : (float)m_flPhysicsVelocity;	// u/sec
	float flGravity = GetCurrentGravity();

	Vector vecVelocity = (vecTargetPos - vecSourcePos);

	// throw at a constant time
	float time = vecVelocity.Length() / flSpeed;
	vecVelocity = vecVelocity * (1.f / time); // CatapultLaunchVelocityMultiplier

	// adjust upward toss to compensate for gravity loss
	vecVelocity.z += flGravity * time * 0.5;

	return vecVelocity;
}

//-----------------------------------------------------------------------------
// Purpose: calculates the launch vector between the entity that touched the
//			catapult trigger and the catapult target
//-----------------------------------------------------------------------------
Vector CPropJumppad::CalculateLaunchVectorPreserve( Vector vecInitialVelocity, CBaseEntity *pVictim, CBaseEntity *pTarget, bool bForcePlayer )
{
	// Find where we're going
	Vector vecSourcePos = pVictim->GetAbsOrigin();
	Vector vecTargetPos = pTarget->GetAbsOrigin();

	// If victim is player, adjust target position so player's center will hit the target
	if (pVictim->IsPlayer() || bForcePlayer)
	{
		vecTargetPos.z -= 32.0f;
	}

	Vector vecDiff = (vecTargetPos - vecSourcePos);

	float flHeight = vecDiff.z;
	float flDist = vecDiff.Length2D();
	float flVelocity = (pVictim->IsPlayer() || bForcePlayer || pVictim->IsNPC()) ? (float)m_flVelocity : (float)m_flPhysicsVelocity;
	float flGravity = -1.0f * GetCurrentGravity();

	if (flDist == 0.f)
	{
		DevWarning( "Bad location input for catapult!\n" );
		return CalculateLaunchVector( pVictim, pTarget );
	}

	float flRadical = flVelocity * flVelocity * flVelocity * flVelocity - flGravity * (flGravity * flDist * flDist - 2.f * flHeight * flVelocity * flVelocity);

	if (flRadical <= 0.f)
	{
		DevWarning( "Catapult can't hit target! Add more speed!\n" );
		return CalculateLaunchVector( pVictim, pTarget );
	}

	flRadical = (sqrt( flRadical ));

	float flTestAngle1 = flVelocity * flVelocity;
	float flTestAngle2 = flTestAngle1;

	flTestAngle1 = -atan( (flTestAngle1 + flRadical) / (flGravity * flDist) );
	flTestAngle2 = -atan( (flTestAngle2 - flRadical) / (flGravity * flDist) );

	Vector vecTestVelocity1 = vecDiff;
	vecTestVelocity1.z = 0;
	vecTestVelocity1.NormalizeInPlace();

	Vector vecTestVelocity2 = vecTestVelocity1;

	vecTestVelocity1 *= flVelocity * cos( flTestAngle1 );
	vecTestVelocity1.z = flVelocity * sin( flTestAngle1 );

	vecTestVelocity2 *= flVelocity * cos( flTestAngle2 );
	vecTestVelocity2.z = flVelocity * sin( flTestAngle2 );

	vecInitialVelocity.NormalizeInPlace();

	if (m_ExactVelocityChoice == 1)
	{
		return vecTestVelocity1;
	}
	else if (m_ExactVelocityChoice == 2)
	{
		return vecTestVelocity2;
	}

	if (vecInitialVelocity.Dot( vecTestVelocity1 ) > vecInitialVelocity.Dot( vecTestVelocity2 ))
	{
		return vecTestVelocity1;
	}
	return vecTestVelocity2;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJumppad::LaunchByTarget( CBaseEntity *pVictim, CBaseEntity *pTarget )
{
	Vector vecVictim;
	if (pVictim->VPhysicsGetObject())
	{
		pVictim->VPhysicsGetObject()->GetVelocity( &vecVictim, NULL );
	}
	else
	{
		vecVictim = pVictim->GetAbsVelocity();
	}
	// get the launch vector
	Vector vecVelocity = m_bUseExactVelocity ?
		CalculateLaunchVectorPreserve( vecVictim, pVictim, pTarget ) :
		CalculateLaunchVector( pVictim, pTarget );

	// Handle a player/NPC
	if (pVictim->IsPlayer() || pVictim->IsNPC())
	{
		// Send us flying
		if (pVictim->GetFlags() & FL_ONGROUND)
		{
			pVictim->SetGroundEntity( NULL );
			pVictim->SetGroundChangeTime( gpGlobals->curtime + 0.5f );
		}

		//CBasePlayer *pPlayer = ToBasePlayer( pVictim );
		//if (pPlayer)
		//{
		//float flSupressionTimeInSeconds = 0.25f;
		//if (m_flAirControlSupressionTime > 0)
		//{
		//	// If set in the map, use this override time
		//	flSupressionTimeInSeconds = m_flAirControlSupressionTime;
		//}
		//pPlayer->SetAirControlSupressionTime( flSupressionTimeInSeconds * 1000.0f ); // fix units, this method expects milliseconds
		pVictim->Teleport( NULL, NULL, &vecVelocity );
		OnLaunchedVictim( pVictim );
		//}
	}
	else
	{
		if (pVictim->GetMoveType() == MOVETYPE_VPHYSICS)
		{
			// Launch!
			IPhysicsObject *pPhysObject = pVictim->VPhysicsGetObject();
			if (pPhysObject)
			{
				AngularImpulse angImpulse = m_bApplyAngularImpulse ? RandomAngularImpulse( -150.0f, 150.0f ) : vec3_origin;
				pPhysObject->SetVelocityInstantaneous( &vecVelocity, &angImpulse );

				// UNDONE: don't mess with physics properties 

				CPhysicsProp *pProp = dynamic_cast<CPhysicsProp *>(pVictim);
				if (pProp != NULL)
				{
					//HACK!
					pProp->OnPhysGunDrop( UTIL_GetLocalPlayer(), LAUNCHED_BY_CANNON );
				}
			}
		}
		OnLaunchedVictim( pVictim );
	}
	PropSetAnim( "bounce" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJumppad::LaunchByDirection( CBaseEntity *pVictim )
{
	Vector vecForward;
	AngleVectors( m_vecLaunchAngles, &vecForward, NULL, NULL );

	// Handle a player
	if (pVictim->IsPlayer() || pVictim->IsNPC())
	{
		// Simply push us forward
		Vector vecPush = vecForward * m_flVelocity;

		// Hack on top of magic
		if (CloseEnough( vecPush[0], 0.f ) && CloseEnough( vecPush[1], 0.f ))
		{
			vecPush[2] = m_flVelocity * 1.5f;	// FIXME: Magic!
		}

		// Send us flying
		if (pVictim->GetFlags() & FL_ONGROUND)
		{
			pVictim->SetGroundEntity( NULL );
			pVictim->SetGroundChangeTime( gpGlobals->curtime + 0.5f );
		}

		pVictim->SetAbsVelocity( vecPush );
		OnLaunchedVictim( pVictim );

		//// Do air control suppression
		//if (m_bDirectionSuppressAirControl)
		//{
		//	float flSupressionTimeInSeconds = 0.25f;
		//	if (m_flAirControlSupressionTime > 0)
		//	{
		//		// If set in the map, use this override time
		//		flSupressionTimeInSeconds = m_flAirControlSupressionTime;
		//	}

		//	CBasePlayer* pTFPlayer = static_cast<CBasePlayer*>(pVictim);
		//	pTFPlayer->SetAirControlSupressionTime( flSupressionTimeInSeconds * 1000.0f ); // fix units, this method expects milliseconds
		//}
	}
	else
	{
		if (pVictim->GetMoveType() == MOVETYPE_VPHYSICS)
		{
			// Launch!
			IPhysicsObject *pPhysObject = pVictim->VPhysicsGetObject();
			if (pPhysObject)
			{
				Vector vecVelocity = vecForward * m_flPhysicsVelocity;
				vecVelocity[2] = m_flPhysicsVelocity;

				AngularImpulse angImpulse = RandomAngularImpulse( -50.0f, 50.0f );

				pPhysObject->SetVelocityInstantaneous( &vecVelocity, &angImpulse );

				// Force this!
				float flNull = 0.0f;
				pPhysObject->SetDragCoefficient( &flNull, &flNull );
				pPhysObject->SetDamping( &flNull, &flNull );

				CPhysicsProp *pProp = dynamic_cast<CPhysicsProp *>(pVictim);
				if (pProp != NULL)
				{
					//HACK!
					pProp->OnPhysGunDrop( UTIL_GetLocalPlayer(), LAUNCHED_BY_CANNON );
				}
			}
		}
		OnLaunchedVictim( pVictim );
	}
	PropSetAnim( "bounce" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJumppad::OnLaunchedVictim( CBaseEntity *pVictim )
{
	m_OnTrigger.FireOutput( pVictim, this );

	//if (pVictim->IsPlayer())
	//{
	//	CBasePlayer *pPlayer = static_cast<CBasePlayer *>(pVictim);
	//	int nRefireIndex = pPlayer->entindex();
	//	m_flRefireDelay = gpGlobals->curtime + 0.5f; // HACK!
	//}
	//else
	//{
	//	m_flRefireDelay = gpGlobals->curtime + 0.5f; // HACK!
	//}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJumppad::DrawDebugGeometryOverlays( void )
{
	BaseClass::DrawDebugGeometryOverlays();
	CBaseEntity *pLaunchTarget = m_hLaunchTarget;
	if (pLaunchTarget)
	{
		// Help us visualize the target
		Vector vecSourcePos = GetAbsOrigin();
		Vector vecTargetPos = pLaunchTarget->GetAbsOrigin();

		float flSpeed = m_flVelocity;
		float flGravity = sv_gravity.GetFloat();

		Vector vecVelocity = (vecTargetPos - vecSourcePos);

		// This is a hack to get around air resistance with weighted cubes -- this is not intended for all objects!
		// float flDragCoefficient = (pVictim->IsPlayer()) ? 1.0f : ( 1.6f );
		float flDragCoefficient = 0.0f;

		// throw at a constant time
		float time = vecVelocity.Length() / flSpeed;
		vecVelocity = vecVelocity * (1.0 / time) * flDragCoefficient;

		// adjust upward toss to compensate for gravity loss
		vecVelocity.z += flGravity * time * 0.5;

		Vector vecApex = vecSourcePos + (vecTargetPos - vecSourcePos) * 0.5;
		vecApex.z += 0.5 * flGravity * (time * 0.5) * (time * 0.5);

		// Visualize it!
		if (!m_bUseExactVelocity)
		{
			NDebugOverlay::Box( vecSourcePos, -Vector( 2, 2, 2 ), Vector( 2, 2, 2 ), 0, 255, 0, 8.0f, 0.05f );
			NDebugOverlay::Box( vecTargetPos, -Vector( 2, 2, 2 ), Vector( 2, 2, 2 ), 0, 255, 0, 8.0f, 0.05f );
			NDebugOverlay::Box( vecApex, -Vector( 2, 2, 2 ), Vector( 2, 2, 2 ), 0, 255, 0, 8.0f, 0.05f );
			NDebugOverlay::Line( vecSourcePos, vecApex, 0, 255, 0, false, 0.05f );
			NDebugOverlay::Line( vecApex, vecTargetPos, 0, 255, 0, false, 0.05f );
		}
		else
		{
			Vector lastPos = vecSourcePos;
			vecVelocity = (vecTargetPos - vecSourcePos);
			vecVelocity = CalculateLaunchVectorPreserve( vecVelocity, this, pLaunchTarget, true );
			for (int i = 0; i < 20; i++)
			{
				float flTime = 0.2f * (i + 1);

				vecApex = vecSourcePos + vecVelocity * flTime;
				vecApex.z -= 0.5 * flGravity * (flTime) * (flTime);
				NDebugOverlay::Box( vecApex, -Vector( 2, 2, 2 ), Vector( 2, 2, 2 ), 0, 255, 0, 8.0f, 0.05f );
				NDebugOverlay::Line( vecApex, lastPos, 0, 255, 0, false, 0.05f );
				lastPos = vecApex;
			}
		}

		// Physics!
		flSpeed = m_flPhysicsVelocity;
		vecVelocity = (vecTargetPos - vecSourcePos);

		// This is a hack to get around air resistance with weighted cubes -- this is not intended for all objects!
		flDragCoefficient = catapult_physics_drag_boost.GetFloat();

		// throw at a constant time
		time = vecVelocity.Length() / flSpeed;
		vecVelocity = vecVelocity * (1.0 / time) * flDragCoefficient;

		// adjust upward toss to compensate for gravity loss
		vecVelocity.z += flGravity * time * 0.5;

		vecApex = vecSourcePos + (vecTargetPos - vecSourcePos) * 0.5;
		vecApex.z += 0.5 * flGravity * (time * 0.5) * (time * 0.5);

		// Visualize it!
		if (!m_bUseExactVelocity)
		{
			NDebugOverlay::Box( vecApex, -Vector( 2, 2, 2 ), Vector( 2, 2, 2 ), 255, 255, 0, 8.0f, 0.05f );
			NDebugOverlay::Line( vecSourcePos, vecApex, 255, 255, 0, false, 0.05f );
			NDebugOverlay::Line( vecApex, vecTargetPos, 255, 255, 0, false, 0.05f );
		}
		else
		{
			Vector lastPos = vecSourcePos;
			vecVelocity = (vecTargetPos - vecSourcePos);
			vecVelocity = CalculateLaunchVectorPreserve( vecVelocity, this, pLaunchTarget );
			for (int i = 0; i < 20; i++)
			{
				float flTime = 0.2f * (i + 1);

				vecApex = vecSourcePos + vecVelocity * flTime;
				vecApex.z -= 0.5 * flGravity * (flTime) * (flTime);
				NDebugOverlay::Box( vecApex, -Vector( 2, 2, 2 ), Vector( 2, 2, 2 ), 255, 255, 0, 8.0f, 0.05f );
				NDebugOverlay::Line( vecApex, lastPos, 255, 255, 0, false, 0.05f );
				lastPos = vecApex;
			}
		}
	}
}
//---------------------------------------------------------
//---------------------------------------------------------
int CPropJumppad::DrawDebugTextOverlays( void )
{
	int text_offset = BaseClass::DrawDebugTextOverlays();
	if (m_debugOverlays & OVERLAY_TEXT_BIT)
	{
		char tempstr[512];
		Q_snprintf( tempstr, sizeof( tempstr ), "Launch target: %s", m_strLaunchTarget.ToCStr() );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		Q_snprintf( tempstr, sizeof( tempstr ), "Player velocity: %.2f", m_flVelocity );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		Q_snprintf( tempstr, sizeof( tempstr ), "Physics velocity: %.2f", m_flPhysicsVelocity );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		// Get the target
		CBaseEntity *pLaunchTarget = m_hLaunchTarget;

		// See if we're attempting to hit a target
		if (pLaunchTarget)
		{
			Vector vecSourcePos = GetAbsOrigin();
			float flGravity = sv_gravity.GetFloat();

			{
				Vector vecTargetPos = pLaunchTarget->GetAbsOrigin();
				vecTargetPos.z -= 32.0f;

				Vector vecVelocity = (vecTargetPos - vecSourcePos);

				// throw at a constant time
				float time = vecVelocity.Length() / m_flVelocity;
				vecVelocity = vecVelocity * (1.0 / time);

				// adjust upward toss to compensate for gravity loss
				vecVelocity.z += flGravity * time * 0.5;

				Q_snprintf( tempstr, sizeof( tempstr ), "Adjusted Player velocity: %.2f", vecVelocity.Length() );
				EntityText( text_offset, tempstr, 0 );
				text_offset++;
			}

			{
				Vector vecTargetPos = pLaunchTarget->GetAbsOrigin();
				Vector vecVelocity = (vecTargetPos - vecSourcePos);

				// throw at a constant time
				float time = vecVelocity.Length() / m_flPhysicsVelocity;
				vecVelocity = vecVelocity * (1.0 / time);

				// adjust upward toss to compensate for gravity loss
				vecVelocity.z += flGravity * time * 0.5;

				Q_snprintf( tempstr, sizeof( tempstr ), "Adjusted Physics velocity: %.2f", vecVelocity.Length() );
				EntityText( text_offset, tempstr, 0 );
				text_offset++;
			}
		}
	}
	return text_offset;
}

void CPropJumppad::InputEnable( inputdata_t &in )
{
	SetContextThink( &CPropJumppad::LaunchThink, gpGlobals->curtime, s_szLaunchThinkContext );
}

void CPropJumppad::InputDisable( inputdata_t &in )
{
	SetContextThink( NULL, gpGlobals->curtime, s_szLaunchThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJumppad::InputSetSpeed( inputdata_t &in )
{
	m_flVelocity = in.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJumppad::InputSetPhysicsSpeed( inputdata_t &in )
{
	m_flPhysicsVelocity = in.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJumppad::InputSetLaunchTarget( inputdata_t &in )
{
	m_strLaunchTarget = in.value.StringID();
	m_hLaunchTarget = gEntList.FindEntityByName( NULL, m_strLaunchTarget );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this entity passes the filter criteria, false if not.
// Input  : pOther - The entity to be filtered.
//-----------------------------------------------------------------------------
bool CPropJumppad::PassesFilters( CBaseEntity *pOther )
{
	CBaseFilter *pFilter = m_hFilter.Get();
	return (!pFilter) ? true : pFilter->PassesFilter( this, pOther );
}