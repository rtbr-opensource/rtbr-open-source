//========= Copyright (c) RTBR Team, 2023 ============//
//
// Purpose: Gauss Rifle/Tau Cannon
//
//====================================================//

#include "cbase.h"
#include "c_basehlcombatweapon.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "c_weapon__stubs.h"

#define NUM_BEAM_POINTS 3 // how many beam start/end points to receive from server

class C_WeaponGauss : public C_BaseHLCombatWeapon
{
	DECLARE_CLASS(C_WeaponGauss, C_BaseHLCombatWeapon);

public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	virtual void OnDataChanged(DataUpdateType_t updateType);
	virtual void Precache( void );
	void DrawBeams( void );
	void ClearEffects( void );

	Vector m_vBeamPoints[NUM_BEAM_POINTS];
	bool m_bCharging;
	bool m_bChargeEffectsActive = false;
	float m_flChargeAmount = 0.0f;
	bool m_bJustFired = false;
	bool m_bSuitCharging;
	EHANDLE m_hCharger;

	CNewParticleEffect *m_hCapacitorEffect;
	CNewParticleEffect *m_hCoilEffect;
	CNewParticleEffect *m_hExhaustEffect;
	CNewParticleEffect *m_hGaussBeam1;
	CNewParticleEffect *m_hGaussBeam2;
	CNewParticleEffect *m_hEnergySuck;
};

STUB_WEAPON_CLASS_IMPLEMENT(weapon_gauss, C_WeaponGauss);

IMPLEMENT_CLIENTCLASS_DT(C_WeaponGauss, DT_WeaponGauss, CWeaponGauss)
	RecvPropArray3(RECVINFO_ARRAY(m_vBeamPoints), RecvPropVector(RECVINFO(m_vBeamPoints))),
	RecvPropBool(RECVINFO(m_bCharging)),
	RecvPropFloat(RECVINFO(m_flChargeAmount)),
	RecvPropBool(RECVINFO(m_bJustFired)),
	RecvPropBool(RECVINFO(m_bSuitCharging)),
	RecvPropEHandle(RECVINFO(m_hCharger)),
END_RECV_TABLE()

void C_WeaponGauss::Precache( void ){
	PrecacheParticleSystem( "weapon_gauss_beam" );
	PrecacheParticleSystem( "weapon_gauss_beam_reflect" );
	PrecacheParticleSystem( "weapon_gauss_vm_capacitor_charge" );
	PrecacheParticleSystem( "weapon_gauss_vm_coil_charge" );
	PrecacheParticleSystem( "weapon_gauss_vm_exhaust_charge" );
	PrecacheParticleSystem( "weapon_gauss_energysuck" );
	BaseClass::Precache();
}

void C_WeaponGauss::OnDataChanged(DataUpdateType_t updateType)
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner ) 
	{
		ClearEffects();
		return; 
	}

	if (pOwner->GetActiveWeapon() != this) 
	{
		// player switched off weapon
		ClearEffects();
		return;
	}

	if ( m_bCharging && !m_bChargeEffectsActive )
	{
		m_hCapacitorEffect = ParticleProp()->Create( "weapon_gauss_vm_capacitor_charge", PATTACH_ABSORIGIN_FOLLOW);
		ParticleProp()->AddControlPoint( m_hCapacitorEffect, 0, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "capacitor" );

		m_hCoilEffect = ParticleProp()->Create( "weapon_gauss_vm_coil_charge", PATTACH_ABSORIGIN_FOLLOW);
		ParticleProp()->AddControlPoint( m_hCoilEffect, 0, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "muzzle" );
		
		m_hExhaustEffect = ParticleProp()->Create( "weapon_gauss_vm_exhaust_charge", PATTACH_ABSORIGIN_FOLLOW );
		ParticleProp()->AddControlPoint( m_hExhaustEffect, 0, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "exhaust" );
		m_bChargeEffectsActive = true;
	}
	else if ( !m_bCharging && m_bChargeEffectsActive ) 
	{
		// player released charge fire
		ParticleProp()->StopEmission( m_hCapacitorEffect );
		ParticleProp()->StopEmission( m_hCoilEffect );
		ParticleProp()->StopEmission( m_hExhaustEffect );
		m_bChargeEffectsActive = false;
	}

	DrawBeams();

	// suitcharger effects
	if (m_bSuitCharging && !m_hEnergySuck && m_hCharger)
	{
		m_hEnergySuck = ParticleProp()->Create( "weapon_gauss_energysuck", PATTACH_ABSORIGIN_FOLLOW );
		ParticleProp()->AddControlPoint( m_hEnergySuck, 0, m_hCharger, PATTACH_POINT_FOLLOW, "port" );
		ParticleProp()->AddControlPoint( m_hEnergySuck, 1, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "muzzle" );
	}
	else if ((!m_bSuitCharging || !m_hCharger) && m_hEnergySuck)
	{
		ParticleProp()->StopEmission( m_hEnergySuck );
		m_hEnergySuck = NULL;
	}

	BaseClass::OnDataChanged(updateType);
}

void C_WeaponGauss::DrawBeams( void ){
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner ) { return; }
	if ( !m_bJustFired ) { return; }

	// create the first beam
	m_hGaussBeam1 = ParticleProp()->Create( "weapon_gauss_beam", PATTACH_ABSORIGIN_FOLLOW );
	ParticleProp()->AddControlPoint( m_hGaussBeam1, 0, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "muzzle" );
	ParticleProp()->AddControlPoint( m_hGaussBeam1, 1, NULL, PATTACH_WORLDORIGIN, 0, m_vBeamPoints[0] );
	m_hGaussBeam1->SetControlPoint( 2, Vector( m_flChargeAmount, 0, 0 ) );

	if ( m_vBeamPoints[1] == vec3_invalid ) { return; }

	// create the second beam - either a reflected or penetrated shot
	m_hGaussBeam2 = ParticleProp()->Create( "weapon_gauss_beam_reflect", PATTACH_ABSORIGIN_FOLLOW );
	ParticleProp()->AddControlPoint( m_hGaussBeam2, 0, NULL, PATTACH_WORLDORIGIN, 0, m_vBeamPoints[1] );
	ParticleProp()->AddControlPoint( m_hGaussBeam2, 1, NULL, PATTACH_WORLDORIGIN, 0, m_vBeamPoints[2] );
	m_hGaussBeam2->SetControlPoint( 2, Vector( m_flChargeAmount, 0, 0 ) );
}

void C_WeaponGauss::ClearEffects( void )
{
	ParticleProp()->StopEmissionAndDestroyImmediately( m_hCapacitorEffect );
	ParticleProp()->StopEmissionAndDestroyImmediately( m_hCoilEffect );
	ParticleProp()->StopEmissionAndDestroyImmediately( m_hExhaustEffect );
	m_bChargeEffectsActive = false;
}