//========= Copyright (c) RTBR Team, 20xx ============//
//
// Purpose: The tau cannon, a weapon which makes you need
// nothing else in your life.
//
//====================================================//

#ifndef WEAPON_GAUSS_H
#define WEAPON_GAUSS_H

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#define GAUSS_BEAM_SPRITE		"sprites/laserbeam.vmt"
#define PHYSGUN_BEAM_SPRITE		"sprites/laserbeam.vmt"

#define	GAUSS_CHARGE_TIME		0.1f
#define	MAX_GAUSS_CHARGE_TIME		1.6f // 16 ammo used
#define	DANGER_GAUSS_CHARGE_TIME	10

#define GAUSS_RAGDOLL_BOOGIE_SPRITE "sprites/lgtning_noz.vmt"

#define NUM_BEAM_POINTS 3 // how many beam start/end points to send to client. there are 4 points in total but point 0 is always attached to the gun so we only send 3

#define SF_GAUSS_FULL_AMMO_ON_PICKUP	0x000080

#ifdef CLIENT_DLL
#define CWeaponGauss C_WeaponGauss
#endif

#ifdef MAPBASE
extern acttable_t *GetAR2Acttable();
extern int GetAR2ActtableCount();
#endif

//=============================================================================
// Tau cannon
//=============================================================================

class CWeaponGauss : public CBaseHL2MPCombatWeapon
{
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif
public:
	DECLARE_CLASS( CWeaponGauss, CBaseHL2MPCombatWeapon );

	CWeaponGauss( void );

	void	Spawn( void );
	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	AddViewKick( void );

	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	void	ItemPostFrame( void );

	float	GetFireRate( void ) { return 0.2f; }

	virtual void	Equip( CBaseCombatCharacter *pOwner );
	virtual void	OnPickedUp( CBaseCombatCharacter *pNewOwner );

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_1DEGREES;
		return cone;
	}

	// used by bots to determine how long to charge an attack
	virtual const float	MaxChargeTime() const { return MAX_GAUSS_CHARGE_TIME; }
	virtual const float	MaxChargeDamage() const { return 200.0f; }

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL
	CBaseEntity*	Respawn( void );
	virtual bool	HandleInteraction( int interactionType, void *data, CBaseCombatCharacter* sourceEnt );
#endif

#ifdef CLIENT_DLL
	virtual void OnDataChanged( DataUpdateType_t updateType );
	void DrawBeams( void );
	void ClearEffects( void );

	bool m_bChargeEffectsActive = false;
	float m_flOldLastAttack;

	CNewParticleEffect *m_hCapacitorEffect;
	CNewParticleEffect *m_hCoilEffect;
	CNewParticleEffect *m_hExhaustEffect;
	CNewParticleEffect *m_hGaussBeam1;
	CNewParticleEffect *m_hGaussBeam2;
	CNewParticleEffect *m_hEnergySuck;
#endif
protected:

	void	Fire( void );
	void	ChargedFire( void );
	void	RechargeAmmo( void );
	float	GetFullChargeTime( void );
	void	StartFire( void );
	void	StopChargeSound( void );

	void	ImpactParticles( const trace_t *tr );

	void	IncreaseCharge( void );
#ifndef CLIENT_DLL
	bool	ShouldDrawWaterImpacts( const trace_t &shot_trace );
#endif
	void	LoopChargeAnimation( void );

	CNetworkVar( bool, m_bCharging );
	CNetworkVar( float, m_flChargeAmount );
	CNetworkArray( Vector, m_vBeamPoints, NUM_BEAM_POINTS );
	CNetworkVar( float, m_flLastAttack );
	CNetworkVar( bool, m_bSuitCharging );
	CNetworkVar( EHANDLE, m_hCharger );

	bool	m_bStartedCharging;

private:
	EHANDLE			m_hViewModel;
	float			m_flNextChargeTime;
	CSoundPatch		*m_sndCharge;

	float			m_flChargeStartTime;
	bool			m_bChargeIndicated;

#ifdef MAPBASE
	virtual acttable_t		*GetBackupActivityList() { return GetAR2Acttable(); }
	virtual int				GetBackupActivityListCount() { return GetAR2ActtableCount(); }
#endif

	DECLARE_ACTTABLE();

	CWeaponGauss( const CWeaponGauss & );
};

#endif // WEAPON_GAUSS_H
