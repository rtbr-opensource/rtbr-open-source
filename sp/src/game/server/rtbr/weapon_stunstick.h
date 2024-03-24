//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is technically a Mapbase addition, but it's just weapon_stunstick's class declaration.
// 			All actual changes are still nested in #ifdef MAPBASE.
//
//=============================================================================//

#ifndef WEAPON_STUNSTICK_H
#define WEAPON_STUNSTICK_H
#ifdef _WIN32
#pragma once
#endif

#include "basebludgeonweapon.h"

#define	STUNSTICK_RANGE				75.0f
#define	STUNSTICK_REFIRE			0.8f
#define	STUNSTICK_MAX_CHARGE		5.5f
#define	STUNSTICK_BEAM_MATERIAL		"sprites/lgtning.vmt"
#define STUNSTICK_GLOW_MATERIAL		"sprites/light_glow02_add.vmt"
#define STUNSTICK_GLOW_MATERIAL2	"effects/blueflare1.vmt"
#define STUNSTICK_GLOW_MATERIAL_NOZ	"sprites/light_glow02_add_noz"
#define STUNSTICK_EFFECT_MATERIAL	"effects/stunstick.vmt"


class CWeaponStunStick : public CBaseHLBludgeonWeapon
{
	DECLARE_CLASS(CWeaponStunStick, CBaseHLBludgeonWeapon);

public:

	CWeaponStunStick();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

	DECLARE_ACTTABLE();


	virtual void Precache();

	void		Spawn();

	float		GetRange(void)		{ return STUNSTICK_RANGE; }
	float		GetFireRate(void)		{ return STUNSTICK_REFIRE; }


	bool		Deploy(void);
	bool		Holster(CBaseCombatWeapon *pSwitchingTo = NULL);

	void ItemPostFrame(void);
	void	OnPickedUp(CBaseCombatCharacter *pNewOwner);
	void		Drop(const Vector &vecVelocity);
	void		ImpactEffect(trace_t &traceHit);
	void		SecondaryAttack(void)	{}
	void		SetStunState(bool state);
	bool		GetStunState(void);
	virtual void PrimaryAttack(void);

	void		Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
	int			WeaponMeleeAttack1Condition(float flDot, float flDist);

	float		GetDamageForActivity(Activity hitActivity);
	bool		ShouldDisplayAltFireHUDHint();

	int		GetChargedDmg(void);
	void	ChargedAttack(void);
	float	m_flChargeStartTime;
	bool	m_bIsCharging;
	EHANDLE	m_hSparkSprite;

	CWeaponStunStick(const CWeaponStunStick &);

private:

	CNetworkVar(bool, m_bActive);
	CNetworkVar(float, m_flChargeLevel);
};

#endif // WEAPON_STUNSTICK_H
