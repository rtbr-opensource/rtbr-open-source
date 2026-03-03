//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifndef C_VEHICLE_JEEP_H
#define C_VEHICLE_JEEP_H
#pragma once

#include "cbase.h"
#include "c_prop_vehicle.h"
//#include "movevars_shared.h"
//#include "view.h"
#include "flashlighteffect.h"
//#include "c_baseplayer.h"
//#include "c_te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"

//=============================================================================
//
// Client-side Jeep Class
//
class C_PropJeep : public C_PropVehicleDriveable
{

	DECLARE_CLASS( C_PropJeep, C_PropVehicleDriveable );

public:

	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_PropJeep();
	~C_PropJeep();

public:
	virtual void Precache( void );
	virtual void Spawn( void );

	void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd );
	void DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );

	void OnEnteredVehicle( C_BasePlayer *pPlayer );
	void Simulate( void );

	void DrawGaussBeams( void );

	virtual void OnExitedVehicle( C_BaseCombatCharacter *pPassenger );

private:

	void DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void ComputePDControllerCoefficients( float *pCoefficientsOut, float flFrequency, float flDampening, float flDeltaTime );

private:

	Vector		m_vecLastEyePos;
	Vector		m_vecLastEyeTarget;
	Vector		m_vecEyeSpeed;
	Vector		m_vecTargetSpeed;

	float		m_flViewAngleDeltaTime;

	float		m_flJeepFOV;
	CHeadlightEffect *m_pHeadlight;
	bool		m_bHeadlightIsOn;

	bool		m_bCannonFiring;
	bool		m_bCannonCharging;
	bool		m_bChargeEffectsActive;

	float		m_flCannonChargeAmount;

	Vector m_vGaussBeam1; // Main gauss beam start pos
	Vector m_vGaussBeam2; // Main gauss beam end pos/Reflected gauss beam start pos
	Vector m_vGaussBeam3; // Reflected gauss beam end pos

	// RTBR Gauss Particles
	CNewParticleEffect *m_hCapacitorEffect;
	CNewParticleEffect *m_hCoilEffect;
	CNewParticleEffect *m_hExhaustEffect;

	CNewParticleEffect *m_hGaussBeam1;
	CNewParticleEffect *m_hGaussBeam2;
};

#endif // C_VEHICLE_JEEP_H
