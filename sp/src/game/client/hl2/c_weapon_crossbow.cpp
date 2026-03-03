//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "model_types.h"
#include "clienteffectprecachesystem.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "beamdraw.h"
#include "c_basehlcombatweapon.h"
#include "c_weapon__stubs.h"
#include "particles_new.h"

//
// Crossbow bolt
//

class C_CrossbowBolt : public C_BaseCombatCharacter
{
	DECLARE_CLASS( C_CrossbowBolt, C_BaseCombatCharacter );
	DECLARE_CLIENTCLASS();
public:
	
	C_CrossbowBolt( void );

	virtual RenderGroup_t GetRenderGroup( void )
	{
		// We want to draw translucent bits as well as our main model
		return RENDER_GROUP_TWOPASS;
	}

	virtual void	Precache( void );
	virtual void	ClientThink( void );

	virtual void	OnDataChanged( DataUpdateType_t updateType );

private:

	C_CrossbowBolt( const C_CrossbowBolt & ); // not defined, not accessible

	Vector	m_vecLastOrigin;
	bool	m_bUpdated;
	int		m_iElectric;
	bool	m_bStruckSomething;
	CNewParticleEffect *m_hTrail;
};

IMPLEMENT_CLIENTCLASS_DT( C_CrossbowBolt, DT_CrossbowBolt, CCrossbowBolt )
	RecvPropInt( RECVINFO( m_iElectric ) ),
	RecvPropBool(RECVINFO(m_bStruckSomething)),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_CrossbowBolt::C_CrossbowBolt( void )
{
	m_hTrail = NULL;
}

void C_CrossbowBolt::Precache( void ){
	PrecacheParticleSystem( "weapon_steambow_bolt_trail" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_CrossbowBolt::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_bUpdated = false;
		m_vecLastOrigin = GetAbsOrigin();
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		if ( m_iElectric != -1 ){
			m_hTrail = ParticleProp()->Create( "weapon_steambow_bolt_trail", PATTACH_ABSORIGIN_FOLLOW );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CrossbowBolt::ClientThink( void )
{
	m_bUpdated = false;
	if ( m_bStruckSomething && m_iElectric != -1 ){
		ParticleProp()->StopParticlesNamed( "weapon_steambow_bolt_trail" );
		ParticleProp()->StopEmission( m_hTrail );
		m_hTrail = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CrosshairLoadCallback( const CEffectData &data )
{
	IClientRenderable *pRenderable = data.GetRenderable( );
	if ( !pRenderable )
		return;
	
	Vector	position;
	QAngle	angles;

	// If we found the attachment, emit sparks there
	if ( pRenderable->GetAttachment( data.m_nAttachmentIndex, position, angles ) )
	{
		FX_ElectricSpark( position, 1.0f, 1.0f, NULL );
	}
}

DECLARE_CLIENT_EFFECT( "CrossbowLoad", CrosshairLoadCallback );

class C_WeaponCrossbow : public C_BaseHLCombatWeapon
{
	DECLARE_CLASS( C_CrossbowBolt, C_BaseHLCombatWeapon );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();
public:

	C_WeaponCrossbow( void );
	
	virtual void	Precache( void );
	virtual void	ClientThink( void );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	void EnableScope();
	void DisableScope();

private:

	bool	m_bCharging;
	float	m_flChargeStartTime;
	bool	m_bInZoom;

	CNewParticleEffect *m_hCharge1;
	CNewParticleEffect *m_hCharge2;
	CNewParticleEffect *m_hCharge3;
};

STUB_WEAPON_CLASS_IMPLEMENT( weapon_crossbow, C_WeaponCrossbow );

IMPLEMENT_CLIENTCLASS_DT( C_WeaponCrossbow, DT_WeaponCrossbow, CWeaponCrossbow )
	RecvPropBool( RECVINFO( m_bCharging ) ),
	RecvPropFloat(RECVINFO(m_flChargeStartTime)),
	RecvPropBool(RECVINFO(m_bInZoom)),
END_RECV_TABLE()

BEGIN_DATADESC( C_WeaponCrossbow )
DEFINE_FIELD( m_bCharging, FIELD_BOOLEAN ),
DEFINE_FIELD(m_flChargeStartTime, FIELD_FLOAT),
END_DATADESC()

C_WeaponCrossbow::C_WeaponCrossbow()
{
	m_hCharge1 = NULL;
	m_hCharge2 = NULL;
	m_hCharge3 = NULL;
}

void C_WeaponCrossbow::Precache( void )
{
	PrecacheParticleSystem( "weapon_steambow_charge" );
	BaseClass::Precache();
}

void C_WeaponCrossbow::ClientThink( void )
{
	BaseClass::ClientThink();
}

void C_WeaponCrossbow::OnDataChanged( DataUpdateType_t updateType )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
		return;

	if (pOwner->GetActiveWeapon() != this)
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_hCharge1 );
		m_hCharge1 = NULL;
		ParticleProp()->StopEmissionAndDestroyImmediately( m_hCharge2 );
		m_hCharge2 = NULL;
		ParticleProp()->StopEmissionAndDestroyImmediately( m_hCharge3 );
		m_hCharge3 = NULL;
	}

	if (!m_bCharging) {
		if (m_hCharge1) {
			ParticleProp()->StopEmission( m_hCharge1 );
			m_hCharge1 = NULL;
		}
		if (m_hCharge2) {
			ParticleProp()->StopEmission( m_hCharge2 );
			m_hCharge2 = NULL;
		}
		if (m_hCharge3) {
			ParticleProp()->StopEmission( m_hCharge3 );
			m_hCharge3 = NULL;
		}
	}
	else if (pOwner) {
		float flElapsedTime = gpGlobals->curtime - m_flChargeStartTime;

		if (!m_hCharge1 && m_bCharging && flElapsedTime >= 0.667f){
			m_hCharge1 = ParticleProp()->Create( "weapon_steambow_charge", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( m_hCharge1, 0, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "battery_1" );
		}
		else if (!m_hCharge2 && m_bCharging && flElapsedTime >= 1.333f) {
			m_hCharge2 = ParticleProp()->Create( "weapon_steambow_charge", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( m_hCharge2, 0, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "battery_2" );
		}
		else if (!m_hCharge3 && m_bCharging && flElapsedTime >= 2.0f) {
			m_hCharge3 = ParticleProp()->Create( "weapon_steambow_charge", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( m_hCharge3, 0, pOwner->GetViewModel(), PATTACH_POINT_FOLLOW, "battery_3" );
		}
	}

	if (m_bInZoom) {
		EnableScope();
	}
	else {
		DisableScope();
	}

	BaseClass::OnDataChanged( updateType );
}

void C_WeaponCrossbow::EnableScope( void ){
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	pPlayer->SetFX( SFX_CROSSBOW, true );
}

void C_WeaponCrossbow::DisableScope( void ){
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	pPlayer->SetFX( SFX_CROSSBOW, false );
}