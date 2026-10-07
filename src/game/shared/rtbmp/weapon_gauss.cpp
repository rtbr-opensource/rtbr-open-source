//========= Copyright (c) RTBR Team, 20xx ============//
//
// Purpose: The gauss rifle/tau cannon, a weapon which
// makes you need nothing else in your life.
//
//====================================================//

#include "cbase.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "particles_new.h"
	#include "prediction.h"
#else
	#include "hl2mp_player.h"
	#include "explode.h"
	#include "ilagcompensationmanager.h"
	#include "rtbmp_player.h"
#endif

#include "weapon_gauss.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "soundenvelope.h"
#include "effect_dispatch_data.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar rtba_test_gauss_noreflect( "rtba_test_gauss_noreflect", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar rtba_test_gauss_lowreflectdmg( "rtba_test_gauss_lowreflectdmg", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar rtba_test_gauss_armourbuster( "rtba_test_gauss_armourbuster", "1", FCVAR_NOTIFY | FCVAR_REPLICATED );

//-----------------------------------------------------------------------------
// Declarations
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponGauss, DT_WeaponGauss )

BEGIN_NETWORK_TABLE( CWeaponGauss, DT_WeaponGauss)
#ifdef CLIENT_DLL
RecvPropArray3( RECVINFO_ARRAY( m_vBeamPoints ), RecvPropVector( RECVINFO( m_vBeamPoints[0] ) ) ),
RecvPropBool( RECVINFO( m_bCharging ) ),
RecvPropFloat( RECVINFO( m_flChargeAmount ) ),
RecvPropFloat( RECVINFO( m_flLastAttack ) ),
RecvPropBool( RECVINFO( m_bSuitCharging ) ),
RecvPropEHandle( RECVINFO( m_hCharger ) ),
#else
SendPropArray3( SENDINFO_ARRAY3( m_vBeamPoints ), SendPropVector( SENDINFO( m_vBeamPoints ) ) ),
SendPropBool( SENDINFO( m_bCharging ) ),
SendPropFloat( SENDINFO( m_flChargeAmount ) ),
SendPropFloat( SENDINFO( m_flLastAttack ) ),
SendPropBool( SENDINFO( m_bSuitCharging ) ),
SendPropEHandle( SENDINFO( m_hCharger ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponGauss )
DEFINE_PRED_ARRAY(m_vBeamPoints, FIELD_VECTOR, NUM_BEAM_POINTS, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_bCharging, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD( m_flChargeAmount, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_flLastAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_bSuitCharging, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_hCharger, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD_TOL( m_flChargeStartTime, FIELD_FLOAT, 0, TD_MSECTOLERANCE ),
DEFINE_PRED_FIELD_TOL( m_flNextChargeTime, FIELD_FLOAT, 0, TD_MSECTOLERANCE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_gauss, CWeaponGauss );
PRECACHE_WEAPON_REGISTER( weapon_gauss );


acttable_t	CWeaponGauss::m_acttable[] =
{
#if AR2_ACTIVITY_FIX == 1
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_AR2,				true },
	{ ACT_IDLE,						ACT_IDLE_AR2,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_AR2,			false },

	{ ACT_WALK,						ACT_WALK_AR2,					true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_AR2_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_AR2_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_AR2,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_AR2_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_AR2_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_AR2,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_AR2_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_AR2_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_AR2_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_AR2_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_AR2,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_AR2_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_AR2_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_AR2,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_AR2_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_AR2_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_AR2,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_AR2,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_AR2,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_COVER_LOW,				ACT_COVER_AR2_LOW,				true },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_AR2_LOW,		false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_AR2_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_AR2,		true },
//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },
#else
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },		// FIXME: hook to AR2 unique

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },		// FIXME: hook to AR2 unique
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },		// FIXME: hook to AR2 unique
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },
#endif

#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_ARM,						ACT_ARM_RIFLE,					false },
	{ ACT_DISARM,					ACT_DISARM_RIFLE,				false },
#endif

#if EXPANDED_HL2_COVER_ACTIVITIES
	{ ACT_RANGE_AIM_MED,			ACT_RANGE_AIM_AR2_MED,			false },
	{ ACT_RANGE_ATTACK1_MED,		ACT_RANGE_ATTACK_AR2_MED,		false },

	{ ACT_COVER_WALL_R,				ACT_COVER_WALL_R_RIFLE,			false },
	{ ACT_COVER_WALL_L,				ACT_COVER_WALL_L_RIFLE,			false },
	{ ACT_COVER_WALL_LOW_R,			ACT_COVER_WALL_LOW_R_RIFLE,		false },
	{ ACT_COVER_WALL_LOW_L,			ACT_COVER_WALL_LOW_L_RIFLE,		false },
#endif

#ifdef MAPBASE
	// HL2:DM activities (for third-person animations in SP)
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_AR2,                    false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_AR2,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_AR2,            false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_AR2,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_AR2,        false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_AR2,                    false },
#if EXPANDED_HL2DM_ACTIVITIES
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_AR2,						false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_AR2,	false },
#endif
#endif
};

IMPLEMENT_ACTTABLE( CWeaponGauss );

#ifndef CLIENT_DLL
//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CWeaponGauss )

DEFINE_FIELD( m_hViewModel, FIELD_EHANDLE ),
DEFINE_FIELD( m_flNextChargeTime, FIELD_TIME ),
DEFINE_FIELD( m_flChargeStartTime, FIELD_TIME ),
DEFINE_FIELD( m_bChargeIndicated, FIELD_BOOLEAN ),
DEFINE_FIELD( m_bStartedCharging, FIELD_BOOLEAN ),
DEFINE_SOUNDPATCH( m_sndCharge ),

END_DATADESC()
#endif

#ifndef RTBMP
extern ConVar sk_plr_dmg_gauss;
extern ConVar sk_plr_max_dmg_gauss;
#endif
#ifndef CLIENT_DLL
extern int g_interactionSuitChargerEmpty;
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponGauss::CWeaponGauss( void )
{
	m_hViewModel = NULL;
	m_flNextChargeTime = 0;
	m_flChargeStartTime = 0;
	m_sndCharge = NULL;
	m_bCharging = false;
	m_bChargeIndicated = false;
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;
	m_bSuitCharging = false;
	m_bStartedCharging = false;
	m_hCharger = EHANDLE(NULL);
	m_flLastAttack = -1.0f;
#ifdef CLIENT_DLL
	m_flOldLastAttack = -1.0f;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::Precache( void )
{
#ifndef CLIENT_DLL
	PrecacheScriptSound( "Weapon_Gauss.Charge" );
	PrecacheParticleSystem( "weapon_gauss_impact" );
	PrecacheScriptSound( "Weapon_Gauss.SuitCharger_ChargeStart" );
	PrecacheScriptSound( "Weapon_Gauss.SuitCharger_ChargeEnd" );
	PrecacheModel( GAUSS_RAGDOLL_BOOGIE_SPRITE );
#else
	PrecacheParticleSystem( "weapon_gauss_beam" );
	PrecacheParticleSystem( "weapon_gauss_beam_reflect" );
	PrecacheParticleSystem( "weapon_gauss_vm_capacitor_charge" );
	PrecacheParticleSystem( "weapon_gauss_vm_coil_charge" );
	PrecacheParticleSystem( "weapon_gauss_vm_exhaust_charge" );
	PrecacheParticleSystem( "weapon_gauss_energysuck" );
#endif
	BaseClass::Precache();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::Fire( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (!pOwner){ return; }

#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	m_bCharging = false;

	if (m_hViewModel == NULL)
	{
		CBaseViewModel *vm = pOwner->GetViewModel();

		if (vm)
		{
			m_hViewModel.Set( vm );
		}
	}

	Vector	startPos = pOwner->Weapon_ShootPosition();
	Vector	aimDir = pOwner->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecUp, vecRight;
	VectorVectors( aimDir, vecRight, vecUp );

	float x, y, z;

	// cone
	do {
		x = random->RandomFloat( -0.5, 0.5 ) + random->RandomFloat( -0.5, 0.5 );
		y = random->RandomFloat( -0.5, 0.5 ) + random->RandomFloat( -0.5, 0.5 );
		z = x*x + y*y;
	} while (z > 1);

	aimDir = aimDir + x * GetBulletSpread().x * vecRight + y * GetBulletSpread().y * vecUp;

	Vector	endPos = startPos + (aimDir * MAX_TRACE_LENGTH);

	// dist
	trace_t	tr;
	UTIL_TraceLine( startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_INTERACTIVE, &tr );

#ifndef CLIENT_DLL
	ClearMultiDamage();

	CBaseEntity *pHit = tr.m_pEnt;

#ifndef RTBMP
	CTakeDamageInfo dmgInfo( this, pOwner, sk_plr_dmg_gauss.GetFloat(), DMG_SHOCK );
#else
	CTakeDamageInfo dmgInfo( this, pOwner, GetHL2MPWpnData().m_iPlayerDamage, DMG_SHOCK );
#endif
	if (pHit != NULL)
	{
		CalculateBulletDamageForce( &dmgInfo, m_iPrimaryAmmoType, aimDir, tr.endpos );
		if (!pHit->IsWorld())
			dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();

		if ( pHit->IsPlayer() && rtba_test_gauss_armourbuster.GetBool() )
		{
			// deal more damage to armour
			CRTBMP_Player *pPlayer = ToRTBMPPlayer( pHit );
			dmgInfo.SetDamage( dmgInfo.GetDamage() + MIN( pPlayer->ArmorValue(), GetHL2MPWpnData().m_iPlayerDamage * 0.5f ) );
		}

		pHit->DispatchTraceAttack( dmgInfo, aimDir, &tr );
	}
#endif
#ifndef CLIENT_DLL
	ShouldDrawWaterImpacts( tr );
#endif
	m_vBeamPoints.Set( 0, tr.endpos );

	if (tr.DidHitWorld() && !(tr.surface.flags & SURF_SKY) && !rtba_test_gauss_noreflect.GetBool())
	{
		float hitAngle = -DotProduct( tr.plane.normal, aimDir );

		if (hitAngle < 0.5f)
		{
			Vector vReflection;

			vReflection = 2.0 * tr.plane.normal * hitAngle + aimDir;

			startPos = tr.endpos;
			endPos = startPos + (vReflection * MAX_TRACE_LENGTH);

			CPVSFilter filter( tr.endpos );

			UTIL_ImpactTrace( &tr, GetAmmoDef()->DamageType( m_iPrimaryAmmoType ), "ImpactGauss" );

			ImpactParticles( &tr );

			//Find new reflection end position
			UTIL_TraceLine( startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );

#ifndef CLIENT_DLL
			if (tr.m_pEnt != NULL)
			{
				if (rtba_test_gauss_lowreflectdmg.GetBool())
				{
					dmgInfo.SetDamage( 10 ); // hardcoded number but this is only for testing!!
				}

				dmgInfo.SetDamageForce( GetAmmoDef()->DamageForce( m_iPrimaryAmmoType ) * vReflection );
				dmgInfo.SetDamagePosition( tr.endpos );
				dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
				tr.m_pEnt->DispatchTraceAttack( dmgInfo, vReflection, &tr );
			}
#endif
			//Connect reflection point to end
			m_vBeamPoints.Set( 1, tr.startpos );
			m_vBeamPoints.Set( 2, tr.endpos );
		}
		else
		{
			m_vBeamPoints.Set( 1, vec3_invalid );
			m_vBeamPoints.Set( 2, vec3_invalid );
		}
	}
	else
	{
		m_vBeamPoints.Set( 1, vec3_invalid );
		m_vBeamPoints.Set( 2, vec3_invalid );
	}
#ifndef CLIENT_DLL
	ApplyMultiDamage();
#endif

	UTIL_ImpactTrace( &tr, GetAmmoDef()->DamageType( m_iPrimaryAmmoType ), "ImpactGauss" );
	ImpactParticles( &tr );

	CPVSFilter filter( tr.endpos );

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3f;

	AddViewKick();

	m_flChargeAmount = 0.0f;
	m_flLastAttack = gpGlobals->curtime;

	// player "shoot" animation
	pOwner->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
	pOwner->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

//-----------------------------------------------------------------------------
// charge
//-----------------------------------------------------------------------------
void CWeaponGauss::ChargedFire( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (!pOwner){ return; }

#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	pPlayer->NoteWeaponFired();
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	bool penetrated = false;

	WeaponSound( WPN_DOUBLE );

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	StopChargeSound();

	m_bCharging = false;
	m_bChargeIndicated = false;

	m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;

	Vector	startPos = pOwner->Weapon_ShootPosition();
	Vector	aimDir = pOwner->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector	endPos = startPos + (aimDir * MAX_TRACE_LENGTH);

	trace_t	tr;
	UTIL_TraceLine( startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );
#ifndef CLIENT_DLL
	ShouldDrawWaterImpacts( tr );
	ClearMultiDamage();
#endif

	m_vBeamPoints.Set( 0, tr.endpos );

	float flChargeAmount = (gpGlobals->curtime - m_flChargeStartTime) / MAX_GAUSS_CHARGE_TIME;


	if (flChargeAmount > 1.0f){ flChargeAmount = 1.0f; }

	const float flMaxDamage = 200 - GetHL2MPWpnData().m_iPlayerDamage; // a fully charged shot should be able to kill anyone at max health/armour
	float flDamage = GetHL2MPWpnData().m_iPlayerDamage + (flMaxDamage * flChargeAmount);

	CBaseEntity *pHit = tr.m_pEnt;

	bool bSkyboxHit = ((tr.surface.flags & SURF_SKY) > 0);

	if (tr.DidHit())
	{
		// wall
		UTIL_ImpactTrace( &tr, GetAmmoDef()->DamageType( m_iPrimaryAmmoType ), "ImpactGauss" );
		cplane_t impactPlane = tr.plane;
		Vector impactNormal = impactPlane.normal;
		QAngle impactAngles;
		VectorAngles( impactNormal, impactAngles );
		ImpactParticles( &tr );
		CPVSFilter filter( tr.endpos );
		Vector  vStore = tr.endpos;
		Vector	testPos = tr.endpos + (aimDir * 48.0f);

		UTIL_TraceLine( testPos, tr.endpos, MASK_SOLID_BRUSHONLY, pOwner, COLLISION_GROUP_NONE, &tr );

		if (!tr.allsolid && !bSkyboxHit) // at specific angles and positions the charged beam can penetrate skybox brushes, creating erroneous impact effects so we need an extra check
		{
			cplane_t impactPlane = tr.plane;
			Vector impactNormal = impactPlane.normal;
			QAngle impactAngles;
			VectorAngles( impactNormal, impactAngles );
			ImpactParticles( &tr );
			penetrated = true;

			trace_t backward_tr;
			UTIL_TraceLine( tr.endpos, vStore, MASK_SOLID_BRUSHONLY, pOwner, COLLISION_GROUP_NONE, &backward_tr );
			if (backward_tr.DidHit()){
				UTIL_ImpactTrace( &backward_tr, GetAmmoDef()->DamageType( m_iPrimaryAmmoType ), "ImpactGauss" );
			}


		}
	}
#ifndef CLIENT_DLL
	if (pHit != NULL)
	{
		CTakeDamageInfo dmgInfo( this, pOwner, flDamage, DMG_SHOCK | DMG_ENERGYBEAM ); // Differentiate between normal and charged attacks by damage type
		CalculateBulletDamageForce( &dmgInfo, m_iPrimaryAmmoType, aimDir, tr.endpos );
		dmgInfo.ScaleDamageForce( flChargeAmount * 16 );
		dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
		pHit->DispatchTraceAttack( dmgInfo, aimDir, &tr );
	}

	ApplyMultiDamage();
#endif
	if (!bSkyboxHit)
		UTIL_ImpactTrace( &tr, GetAmmoDef()->DamageType( m_iPrimaryAmmoType ), "ImpactGauss" );

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( -4.0f, -8.0f );
	viewPunch.y = random->RandomFloat( -0.25f, 0.25f );
	viewPunch.z = 0;

	pOwner->ViewPunch( viewPunch );

	Vector	recoilForce = pOwner->GetAbsVelocity() - pOwner->GetAutoaimVector( 0 ) * (flDamage * 5.0f);
	recoilForce[2] += 12.80f;
	pOwner->SetAbsVelocity( recoilForce );

	CPVSFilter filter( tr.endpos );
	if (penetrated)
	{
		//RadiusDamage(CTakeDamageInfo(this, this, flDamage, DMG_SHOCK), tr.endpos, 200.0f, CLASS_NONE, NULL);

		UTIL_TraceLine( tr.endpos, endPos, MASK_SHOT, pHit, COLLISION_GROUP_NONE, &tr );

		m_vBeamPoints.Set( 1, tr.startpos );
		m_vBeamPoints.Set( 2, tr.endpos );

		if (!bSkyboxHit)
			UTIL_ImpactTrace( &tr, GetAmmoDef()->DamageType( m_iPrimaryAmmoType ), "ImpactGauss" );
		cplane_t impactPlane = tr.plane;
		Vector impactNormal = impactPlane.normal;
		QAngle impactAngles;
		VectorAngles( impactNormal, impactAngles );
		ImpactParticles( &tr );

		//RadiusDamage(CTakeDamageInfo(this, this, flDamage, DMG_SHOCK), tr.endpos, 200.0f, CLASS_NONE, NULL);
#ifndef CLIENT_DLL
		CBaseEntity *pHit = tr.m_pEnt;
		if (pHit != NULL)
		{
			CTakeDamageInfo dmgInfo( this, pOwner, flDamage, DMG_SHOCK );
			CalculateBulletDamageForce( &dmgInfo, m_iPrimaryAmmoType, aimDir, tr.endpos );
			dmgInfo.ScaleDamageForce( flChargeAmount * 8 );
			dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
			pHit->DispatchTraceAttack( dmgInfo, aimDir, &tr );
		}
		ApplyMultiDamage();
#endif
	}
	else{
		m_vBeamPoints.Set( 1, vec3_invalid );
		m_vBeamPoints.Set( 2, vec3_invalid );
	}
	m_flChargeAmount = flChargeAmount;
	m_flLastAttack = gpGlobals->curtime;

	// player "shoot" animation
	pOwner->SetAnimation( PLAYER_ATTACK1 );

	pOwner->DoMuzzleFlash();
#ifndef CLIENT_DLL
	pOwner->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

//-----------------------------------------------------------------------------
// handle dealing with ammo gain via suitcharger
//-----------------------------------------------------------------------------
void CWeaponGauss::RechargeAmmo( void ){
#ifndef CLIENT_DLL
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	Vector	vStartPos = pOwner->Weapon_ShootPosition();
	Vector	vAimDir = pOwner->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector	vEndPos = vStartPos + (vAimDir * PLAYER_USE_RADIUS);
	trace_t	tr;
	UTIL_TraceLine( vStartPos, vEndPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );

	CBaseEntity *pHit = tr.m_pEnt;
	if (pHit != NULL)
	{
		if (pHit->HasSecondaryUse())
		{
			// fixme: this will inevitably break when some other entity gets a secondary use
			m_bSuitCharging = true;
			m_hCharger = EHANDLE(pHit);
			pHit->Use( pOwner, this, USE_AMMO, 0 );
		}
		else
		{
			m_bSuitCharging = false;
			m_bStartedCharging = false;
		}
		
	}
	else
	{
		m_bSuitCharging = false;
		m_bStartedCharging = false;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::PrimaryAttack( void )
{
	//if( weapon_fire_mode.GetBool() == 0 )
	//{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (!pOwner){ return; }

	WeaponSound( SINGLE );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pOwner->DoMuzzleFlash();

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

	Fire();

#ifndef CLIENT_DLL
	CRTBMP_Player *pMPPlayer = ToRTBMPPlayer( pOwner );
	if (pMPPlayer)
		pMPPlayer->DisableSpawnProtection();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::IncreaseCharge( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (m_flNextChargeTime > gpGlobals->curtime || !pOwner){ return; }


	if ((gpGlobals->curtime - m_flChargeStartTime) > MAX_GAUSS_CHARGE_TIME)
	{
		if (m_bChargeIndicated == false)
		{
			WeaponSound( SPECIAL1 );
			m_bChargeIndicated = true;
		}
#ifndef CLIENT_DLL
		if ((gpGlobals->curtime - m_flChargeStartTime) > DANGER_GAUSS_CHARGE_TIME)
		{

			WeaponSound( SPECIAL1 );


			pOwner->TakeDamage( CTakeDamageInfo( this, this, 25, DMG_SHOCK | DMG_CRUSH ) );

			color32 gaussDamage = { 255, 128, 0, 128 };
			UTIL_ScreenFade( pOwner, gaussDamage, 0.2f, 0.2f, FFADE_IN );

			m_flNextChargeTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 2.5f );
		}
#endif
		return;
	}


	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

	int pitch = (gpGlobals->curtime - m_flChargeStartTime) * (150 / GetFullChargeTime()) + 100;
	if (pitch > 250){ pitch = 250; }
	if (m_sndCharge != NULL)
	{
		(CSoundEnvelopeController::GetController()).SoundChangePitch( m_sndCharge, pitch, 0 );
	}


	if (pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
	{
		ChargedFire();
		return;
	}

	m_flNextChargeTime = gpGlobals->curtime + GAUSS_CHARGE_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::SecondaryAttack( void )
{
	if (m_flNextSecondaryAttack > gpGlobals->curtime) {
		return;
	}
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (!pOwner || pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
	{
		WeaponSound( EMPTY );
		return;
	}
#ifndef CLIENT_DLL
	if (pOwner->GetWaterLevel() == 3)
	{
		EmitSound( "Weapon_Gauss.Zap" );
		SendWeaponAnim( ACT_VM_IDLE );
		m_flNextSecondaryAttack = gpGlobals->curtime + 3.0;

		pOwner->TakeDamage( CTakeDamageInfo( this, this, 25, DMG_SHOCK | DMG_CRUSH ) );
		return;
	}
#endif
	if (!m_bCharging)
	{

		SendWeaponAnim( ACT_VM_PULLBACK );


		if (!m_sndCharge)
		{
			CPASAttenuationFilter filter( this );
			m_sndCharge = (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "Weapon_Gauss.Charge", ATTN_NORM );
		}

		if (m_sndCharge != NULL)
		{
			(CSoundEnvelopeController::GetController()).Play( m_sndCharge, 1.0f, 50 );
			(CSoundEnvelopeController::GetController()).SoundChangePitch( m_sndCharge, 250, 3.0f );
		}

		m_flChargeStartTime = gpGlobals->curtime;
		m_bCharging = true;
		m_bChargeIndicated = false;
#ifndef CLIENT_DLL
		CRTBMP_Player *pMPPlayer = ToRTBMPPlayer( pOwner );
		if (pMPPlayer)
			pMPPlayer->DisableSpawnProtection();
#endif
	}

	IncreaseCharge();
}

//-----------------------------------------------------------------------------
// Purpose:  view punch
//-----------------------------------------------------------------------------
void CWeaponGauss::AddViewKick( void )
{
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (pPlayer == NULL)
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( -0.5f, -0.2f );
	viewPunch.y = random->RandomFloat( -0.5f, 0.5f );
	viewPunch.z = 0;

	pPlayer->ViewPunch( viewPunch );
}

//-----------------------------------------------------------------------------
// Purpose: frames
//-----------------------------------------------------------------------------
void CWeaponGauss::ItemPostFrame( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	if (m_bSuitCharging && pPlayer->m_nButtons & IN_ATTACK)
	{
		pPlayer->m_nButtons &= ~IN_ATTACK; // make sure player can't interrupt a charge by holding attack1
	}
	if (pPlayer->m_afButtonReleased & IN_ATTACK2 || pPlayer->m_nButtons & IN_ZOOM)
	{
		if (m_bCharging)
		{
			ChargedFire();
		}
	}
	else if (pPlayer->m_nButtons & IN_ATTACK2){
		pPlayer->m_nButtons &= ~(IN_ATTACK | IN_RELOAD); // don't let player interrupt charge with attack1, or use suitcharger
	}
	else if (pPlayer->m_nButtons & (IN_ATTACK | IN_USE)){
		pPlayer->m_nButtons &= ~IN_RELOAD; // suppress attack1/use and charge so weird stuff doesn't happen with suitcharger
		pPlayer->m_afButtonReleased &= IN_RELOAD;
	}
	else if (pPlayer->m_nButtons & (IN_RELOAD) && gpGlobals->curtime >= m_flNextPrimaryAttack){
		RechargeAmmo();
	}
	else if (m_bSuitCharging && pPlayer->m_afButtonReleased & IN_RELOAD){
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate(); // short cooldown
		m_bSuitCharging = false;
		m_bStartedCharging = false;
	}

	// todo: this is not the optimal way to do things i can feel it, but too tired to fix this now
	if (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) == GetAmmoDef()->MaxCarry( m_iPrimaryAmmoType ))
	{
		// without disturbing any normal gauss/suitcharger function, allow the suitcharger to play its sound
		// while allowing an outro for full ammo
		if (pPlayer->m_afButtonReleased & IN_RELOAD)
			m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
		m_bSuitCharging = false;
		m_bStartedCharging = false;
	}
	
	if (m_bSuitCharging && !m_bStartedCharging)
	{
		// start suitcharging animation
		SendWeaponAnim( ACT_VM_CHARGE_INTRO );
		SetContextThink( &CWeaponGauss::LoopChargeAnimation, gpGlobals->curtime + SequenceDuration(), "LoopContext" );
		m_bStartedCharging = true;
	}
	else if (!m_bSuitCharging && GetActivity() == ACT_VM_CHARGE_LOOP)
	{
		// stop suitcharging animation
		SendWeaponAnim( ACT_VM_CHARGE_OUTRO );
		StopWeaponSound( SPECIAL2 );
	}
	else if (!m_bSuitCharging && GetActivity() == ACT_VM_CHARGE_INTRO)
	{
		SendWeaponAnim( ACT_VM_IDLE );
	}
	else if (!m_bSuitCharging && !m_bStartedCharging)
	{
		StopWeaponSound( SPECIAL2 );
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: charge sound
//-----------------------------------------------------------------------------
void CWeaponGauss::StopChargeSound( void )
{
	if (m_sndCharge != NULL)
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut( m_sndCharge, 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: holster
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGauss::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	StopChargeSound();
	m_bCharging = false;
	m_bChargeIndicated = false;

	return BaseClass::Holster( pSwitchingTo );
}
//-----------------------------------------------
// Purpose: charge time
//-----------------------------------------------
float CWeaponGauss::GetFullChargeTime( void )
{
	if (g_pGameRules->IsMultiplayer())
	{
		return 1.5;
	}
	else
	{
		return 4;
	}
}
#ifndef CLIENT_DLL
//----------------------------------------------------------------------------------
// Purpose: splash
//----------------------------------------------------------------------------------
#define FSetBit(iBitVector, bits)	((iBitVector) |= (bits)) // LOLMEN : Set some bits to bit vec
//#define FBitSet(iBitVector, bit)	((iBitVector) & (bit))	// LOLMEN : Do that bit setted in bit vec?
#define TraceContents( vec ) ( enginetrace->GetPointContents( vec ) ) // LOLMEN : Do some test?
#define WaterContents( vec ) ( FBitSet( TraceContents( vec ), CONTENTS_WATER|CONTENTS_SLIME ) ) // Lolmen : For water

bool CWeaponGauss::ShouldDrawWaterImpacts( const trace_t &shot_trace )
{
	//FIXME: This doesn't handle the case of trying to splash while being underwater, but that's not going to look good
	//		 right now anyway...

	// We must start outside the water
	if (WaterContents( shot_trace.startpos ))
		return false;

	// We must end inside of water
	if (!WaterContents( shot_trace.endpos ))
		return false;

	trace_t	waterTrace;

	UTIL_TraceLine( shot_trace.startpos, shot_trace.endpos, (CONTENTS_WATER | CONTENTS_SLIME), UTIL_GetLocalPlayer(), COLLISION_GROUP_NONE, &waterTrace );


	if (waterTrace.fraction < 1.0f)
	{
		CEffectData	data;

		data.m_fFlags = 0;
		data.m_vOrigin = waterTrace.endpos;
		data.m_vNormal = waterTrace.plane.normal;
		data.m_flScale = random->RandomFloat( 2.0, 4.0f ); // Lolmen : ����������� ������ ������/����� ���

		// See if we hit slime
		if (FBitSet( waterTrace.contents, CONTENTS_SLIME ))
		{
			FSetBit( data.m_fFlags, FX_WATER_IN_SLIME );
		}

		CPASFilter filter( data.m_vOrigin );
		te->DispatchEffect( filter, 0.0, data.m_vOrigin, "watersplash", data );
	}
	return true;
}
#endif
void CWeaponGauss::ImpactParticles( const trace_t *tr )
{
	QAngle impactAngles;
	VectorAngles( tr->plane.normal, impactAngles );
	if (!(tr->surface.flags & SURF_SKY)){
		// non-skybox impact
		DispatchParticleEffect( "weapon_gauss_impact", tr->endpos, impactAngles, this );
	}
}

void CWeaponGauss::LoopChargeAnimation()
{
	if (m_bSuitCharging)
	{
		SendWeaponAnim( ACT_VM_CHARGE_LOOP );
		WeaponSound( SPECIAL2 );
	}
}

//--------------------------------------------------------------
// Purpose: Override to give half ammo for gauss on first pickup
//--------------------------------------------------------------
void CWeaponGauss::Equip( CBaseCombatCharacter *pOwner )
{
#ifndef CLIENT_DLL
	if (pOwner->Weapon_OwnsThisType("weapon_gauss"))
	{
		pOwner->GiveAmmo( GetAmmoDef()->MaxCarry( m_iPrimaryAmmoType ) / 2 - GetDefaultClip1(), m_iPrimaryAmmoType );
	}

	BaseClass::Equip( pOwner );
#endif
}

//--------------------------------------------------------
// Purpose: Override to give max ammo for gauss on pickup
//--------------------------------------------------------
void CWeaponGauss::OnPickedUp( CBaseCombatCharacter *pNewOwner )
{
#ifndef CLIENT_DLL
	if (HasSpawnFlags( SF_GAUSS_FULL_AMMO_ON_PICKUP ) && HL2MPRules()->WeaponShouldRespawn(this) == GR_WEAPON_RESPAWN_YES)
	{
		pNewOwner->GiveAmmo(GetAmmoDef()->MaxCarry(m_iPrimaryAmmoType) / 2, m_iPrimaryAmmoType);
	}

	BaseClass::OnPickedUp( pNewOwner );
#endif
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Set the 'Give full ammo on pickup' spawnflag
//-----------------------------------------------------------------------------
CBaseEntity *CWeaponGauss::Respawn( void )
{
	CBaseEntity *pGauss = BaseClass::Respawn();
	if (HasSpawnFlags( SF_GAUSS_FULL_AMMO_ON_PICKUP ) && HL2MPRules()->WeaponShouldRespawn( this ) == GR_WEAPON_RESPAWN_YES)
	{
		// The entity created will always be a gauss so we can set this flag.
		pGauss->AddSpawnFlags( SF_GAUSS_FULL_AMMO_ON_PICKUP );
	}

	return pGauss;
}

bool CWeaponGauss::HandleInteraction( int interactionType, void *data, CBaseCombatCharacter* sourceEnt )
{
	if (interactionType == g_interactionSuitChargerEmpty)
	{
		m_bSuitCharging = false;
		m_bStartedCharging = false;
		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}
#endif

#ifdef CLIENT_DLL
void CWeaponGauss::OnDataChanged( DataUpdateType_t updateType )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
	{
		ClearEffects();
		return;
	}

	if (pOwner->GetActiveWeapon() != this)
	{
		// player switched off weapon
		ClearEffects();
	}
	else
	{
		if ( m_bCharging && !m_bChargeEffectsActive && ShouldDrawUsingViewModel() )
		{
			m_hCapacitorEffect = ParticleProp()->Create( "weapon_gauss_vm_capacitor_charge", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( m_hCapacitorEffect, 0, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "capacitor" );

			m_hCoilEffect = ParticleProp()->Create( "weapon_gauss_vm_coil_charge", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( m_hCoilEffect, 0, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "muzzle" );

			m_hExhaustEffect = ParticleProp()->Create( "weapon_gauss_vm_exhaust_charge", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( m_hExhaustEffect, 0, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "exhaust" );
			m_bChargeEffectsActive = true;
		}
		else if ( !m_bCharging && m_bChargeEffectsActive && ShouldDrawUsingViewModel() )
		{
			// player released charge fire
			ParticleProp()->StopEmission( m_hCapacitorEffect );
			ParticleProp()->StopEmission( m_hCoilEffect );
			ParticleProp()->StopEmission( m_hExhaustEffect );
			m_bChargeEffectsActive = false;
		}

		if ( m_flLastAttack != m_flOldLastAttack )
		{
			DrawBeams();
			m_flOldLastAttack = m_flLastAttack;
		}

		// suitcharger effects
		if ( m_bSuitCharging && !m_hEnergySuck && m_hCharger.Get() && ShouldDrawUsingViewModel() ) // TODO: we probably need a thirdperosn gauss effect for charger suck
		{
			m_hEnergySuck = ParticleProp()->Create( "weapon_gauss_energysuck", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( m_hEnergySuck, 0, m_hCharger.Get(), PATTACH_POINT_FOLLOW, "port" );
			ParticleProp()->AddControlPoint( m_hEnergySuck, 1, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "muzzle" );
		}
		else if ( (!m_bSuitCharging || !m_hCharger.Get()) && m_hEnergySuck )
		{
			ParticleProp()->StopEmission( m_hEnergySuck );
			m_hEnergySuck = NULL;
		}
	}

	BaseClass::OnDataChanged( updateType );
}

void CWeaponGauss::DrawBeams( void ){
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner) { return; }

	// create the first beam
	m_hGaussBeam1 = ParticleProp()->Create( "weapon_gauss_beam", PATTACH_ABSORIGIN_FOLLOW );
	ParticleProp()->AddControlPoint( m_hGaussBeam1, 0, pOwner->GetRenderedWeaponModel(), PATTACH_POINT_FOLLOW, "muzzle" );
	ParticleProp()->AddControlPoint( m_hGaussBeam1, 1, NULL, PATTACH_WORLDORIGIN, 0, m_vBeamPoints[0] );
	m_hGaussBeam1->SetControlPoint( 2, Vector( m_flChargeAmount, 0, 0 ) );

	if (m_vBeamPoints[1] == vec3_invalid) { return; }

	// create the second beam - either a reflected or penetrated shot
	m_hGaussBeam2 = ParticleProp()->Create( "weapon_gauss_beam_reflect", PATTACH_ABSORIGIN_FOLLOW );
	ParticleProp()->AddControlPoint( m_hGaussBeam2, 0, NULL, PATTACH_WORLDORIGIN, 0, m_vBeamPoints[1] );
	ParticleProp()->AddControlPoint( m_hGaussBeam2, 1, NULL, PATTACH_WORLDORIGIN, 0, m_vBeamPoints[2] );
	m_hGaussBeam2->SetControlPoint( 2, Vector( m_flChargeAmount, 0, 0 ) );
}

void CWeaponGauss::ClearEffects( void )
{
	ParticleProp()->StopEmissionAndDestroyImmediately( m_hCapacitorEffect );
	ParticleProp()->StopEmissionAndDestroyImmediately( m_hCoilEffect );
	ParticleProp()->StopEmissionAndDestroyImmediately( m_hExhaustEffect );
	m_bChargeEffectsActive = false;

	if ( ShouldDrawUsingViewModel() )
	{
		if ( m_hGaussBeam1 )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_hGaussBeam1 );
			m_hGaussBeam1 = NULL;
		}
		if ( m_hGaussBeam2 )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_hGaussBeam2 );
			m_hGaussBeam2 = NULL;
		}
	}
}
#endif