//========= Copyright (c) RTBR Team, 20xx ============//
//
// Purpose: Immolator
//
//====================================================//

#include "cbase.h"
#include "c_basehlcombatweapon.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "c_weapon__stubs.h"

class C_WeaponImmolator : public C_BaseHLCombatWeapon
{
	DECLARE_CLASS( C_WeaponImmolator, C_BaseHLCombatWeapon );

public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void Precache( void );

	bool m_bImmolating;

	CNewParticleEffect *m_hImmolatorFlame;
	Vector m_vPlayerMuzzleVector;
	Vector m_vAiming;
};

STUB_WEAPON_CLASS_IMPLEMENT( weapon_immolator, C_WeaponImmolator );

IMPLEMENT_CLIENTCLASS_DT( C_WeaponImmolator, DT_WeaponImmolator, CWeaponImmolator )
	RecvPropBool(RECVINFO(m_bImmolating)),
	RecvPropVector(RECVINFO(m_vPlayerMuzzleVector)),
	RecvPropVector( RECVINFO( m_vAiming ) ),
END_RECV_TABLE()

void C_WeaponImmolator::Precache( void ){
	PrecacheParticleSystem( "weapon_immolator_flame" );
	BaseClass::Precache();
}

void C_WeaponImmolator::OnDataChanged( DataUpdateType_t updateType )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		ParticleProp()->StopEmission( m_hImmolatorFlame );
		m_hImmolatorFlame = NULL;
		return;
	}

	if ( m_bImmolating && !m_hImmolatorFlame){
		m_hImmolatorFlame = ParticleProp()->Create( "weapon_immolator_flame", PATTACH_CUSTOMORIGIN, 0, m_vPlayerMuzzleVector );
	}
	else if (!m_bImmolating && m_hImmolatorFlame){
		ParticleProp()->StopEmission( m_hImmolatorFlame );
		m_hImmolatorFlame = NULL;
	}

	// set position & angles of the flame stream as a workaround for the weird attachment we have on the viewmodel; this also ensures consistency with the flame entities' paths
	if ( m_hImmolatorFlame ){
		m_hImmolatorFlame->SetControlPoint( 0, m_vPlayerMuzzleVector );
		QAngle angAiming;
		VectorAngles( m_vAiming, angAiming );
		Quaternion qAiming;
		AngleQuaternion( angAiming, qAiming );
		m_hImmolatorFlame->SetControlPointOrientation( 0, qAiming );
	}

	BaseClass::OnDataChanged( updateType );
}