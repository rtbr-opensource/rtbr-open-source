//========= Copyright (c) RTBR Team, 20xx ============//
//
// Purpose: The tau cannon, a weapon which makes you need
// nothing else in your life.
//
//====================================================//

#ifndef WEAPON_GAUSS_H
#define WEAPON_GAUSS_H
#ifdef _WIN32
#pragma once
#endif

#include "basehlcombatweapon.h"
#include "effect_dispatch_data.h"

#define	GAUSS_CHARGE_TIME		0.2f
#define	MAX_GAUSS_CHARGE_TIME		3.2f // 16 ammo used
#define	DANGER_GAUSS_CHARGE_TIME	10

#define GAUSS_RAGDOLL_BOOGIE_SPRITE "sprites/lgtning_noz.vmt"

#define NUM_BEAM_POINTS 3 // how many beam start/end points to send to client. there are 4 points in total but point 0 is always attached to the gun so we only send 3

//=============================================================================
// Tau cannon
//=============================================================================

class CWeaponGauss : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS(CWeaponGauss, CBaseHLCombatWeapon);

	CWeaponGauss(void);

	DECLARE_SERVERCLASS();

	void	Spawn(void);
	void	Precache(void);
	void	PrimaryAttack(void);
	void	SecondaryAttack(void);
	void	AddViewKick(void);

	bool	Holster(CBaseCombatWeapon *pSwitchingTo = NULL);

	void	ItemPostFrame(void);

	float	GetFireRate(void) { return 0.2f; }

	virtual const Vector &GetBulletSpread(void)
	{
		static Vector cone = VECTOR_CONE_1DEGREES;
		return cone;
	}

	virtual bool		HandleInteraction( int interactionType, void *data, CBaseCombatCharacter* sourceEnt );

protected:

	void	Fire(void);
	void	ChargedFire(void);
	void	RechargeAmmo(void);
	float	GetFullChargeTime(void);
	void	StartFire(void);
	void	StopChargeSound(void);

	void	ImpactParticles( const trace_t *tr );

	void	IncreaseCharge(void);
	bool	ShouldDrawWaterImpacts(const trace_t &shot_trace);
	void	LoopChargeAnimation( void );

	CNetworkVar( bool, m_bCharging );
	CNetworkVar( float, m_flChargeAmount );
	CNetworkArray( Vector, m_vBeamPoints, NUM_BEAM_POINTS );
	CNetworkVar( bool, m_bJustFired );
	CNetworkVar( bool, m_bSuitCharging );
	CNetworkVar( EHANDLE, m_hCharger );

	bool	m_bStartedCharging;

private:
	EHANDLE			m_hViewModel;
	float			m_flNextChargeTime;
	float			m_flNextSecondaryAttack;
	CSoundPatch		*m_sndCharge;

	float			m_flChargeStartTime;
	bool			m_bChargeIndicated;

	DECLARE_ACTTABLE();
};

#endif // WEAPON_GAUSS_H
