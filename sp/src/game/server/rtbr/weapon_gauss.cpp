//========= Copyright (c) RTBR Team, 20xx ============//
//
// Purpose: The gauss rifle/tau cannon, a weapon which
// makes you need nothing else in your life.
//
//====================================================//

#include "cbase.h"
#include "weapon_gauss.h"
#include "player.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "soundenvelope.h"
#include "explode.h"
#include "rtbr_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Declarations
//-----------------------------------------------------------------------------

IMPLEMENT_SERVERCLASS_ST( CWeaponGauss, DT_WeaponGauss )
	SendPropArray3(SENDINFO_ARRAY3(m_vBeamPoints), SendPropVector(SENDINFO(m_vBeamPoints))),
	SendPropBool(SENDINFO(m_bCharging)),
	SendPropFloat(SENDINFO(m_flChargeAmount)),
	SendPropBool(SENDINFO( m_bJustFired)),
	SendPropBool(SENDINFO(m_bSuitCharging)),
	SendPropEHandle(SENDINFO(m_hCharger)),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_gauss, CWeaponGauss);
PRECACHE_WEAPON_REGISTER(weapon_gauss);

acttable_t	CWeaponGauss::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_AR2, true },
};

IMPLEMENT_ACTTABLE(CWeaponGauss);

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CWeaponGauss)

DEFINE_FIELD(m_hViewModel, FIELD_EHANDLE),
DEFINE_FIELD(m_flNextChargeTime, FIELD_TIME),
DEFINE_FIELD(m_flChargeStartTime, FIELD_TIME),
DEFINE_FIELD(m_bChargeIndicated, FIELD_BOOLEAN),
DEFINE_AUTO_ARRAY(m_vBeamPoints, FIELD_VECTOR),
DEFINE_FIELD(m_bStartedCharging, FIELD_BOOLEAN),
DEFINE_SOUNDPATCH(m_sndCharge),

END_DATADESC()


extern ConVar sk_plr_dmg_gauss;
extern ConVar sk_plr_max_dmg_gauss;
extern int g_interactionSuitChargerEmpty;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponGauss::CWeaponGauss(void)
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
	m_hCharger = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::Precache(void)
{
	PrecacheScriptSound( "Weapon_Gauss.Charge" );
	PrecacheParticleSystem( "weapon_gauss_impact" );
	PrecacheScriptSound( "Weapon_Gauss.SuitCharger_ChargeStart" );
	PrecacheScriptSound( "Weapon_Gauss.SuitCharger_ChargeEnd" );
	PrecacheModel(GAUSS_RAGDOLL_BOOGIE_SPRITE);
	BaseClass::Precache();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::Spawn(void)
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::Fire(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (!pOwner){ return; }

	m_bCharging = false;

	if (m_hViewModel == NULL)
	{
		CBaseViewModel *vm = pOwner->GetViewModel();

		if (vm)
		{
			m_hViewModel.Set(vm);
		}
	}

	Vector	startPos = pOwner->Weapon_ShootPosition();
	Vector	aimDir = pOwner->GetAutoaimVector(AUTOAIM_5DEGREES);

	Vector vecUp, vecRight;
	VectorVectors(aimDir, vecRight, vecUp);

	float x, y, z;

	// cone
	do {
		x = random->RandomFloat(-0.5, 0.5) + random->RandomFloat(-0.5, 0.5);
		y = random->RandomFloat(-0.5, 0.5) + random->RandomFloat(-0.5, 0.5);
		z = x*x + y*y;
	} while (z > 1);

	aimDir = aimDir + x * GetBulletSpread().x * vecRight + y * GetBulletSpread().y * vecUp;

	Vector	endPos = startPos + (aimDir * MAX_TRACE_LENGTH);

	// dist
	trace_t	tr;
	UTIL_TraceLine(startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_INTERACTIVE, &tr);

	ClearMultiDamage();

	CBaseEntity *pHit = tr.m_pEnt;

	CTakeDamageInfo dmgInfo(this, pOwner, sk_plr_dmg_gauss.GetFloat(), DMG_SHOCK);

	if (pHit != NULL)
	{
		CalculateBulletDamageForce(&dmgInfo, m_iPrimaryAmmoType, aimDir, tr.endpos);
		if (!pHit->IsWorld())
			dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
		pHit->DispatchTraceAttack(dmgInfo, aimDir, &tr);
	}

	ShouldDrawWaterImpacts(tr);

	m_vBeamPoints.Set( 0, tr.endpos );

	if ( tr.DidHitWorld() && !( tr.surface.flags & SURF_SKY ) )
	{
		float hitAngle = -DotProduct(tr.plane.normal, aimDir);

		if (hitAngle < 0.5f)
		{
			Vector vReflection;

			vReflection = 2.0 * tr.plane.normal * hitAngle + aimDir;

			startPos = tr.endpos;
			endPos = startPos + (vReflection * MAX_TRACE_LENGTH);

			CPVSFilter filter(tr.endpos);

			UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");

			ImpactParticles(&tr);

			//Find new reflection end position
			UTIL_TraceLine(startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

			if (tr.m_pEnt != NULL)
			{
				dmgInfo.SetDamageForce(GetAmmoDef()->DamageForce(m_iPrimaryAmmoType) * vReflection);
				dmgInfo.SetDamagePosition(tr.endpos);
				dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
				tr.m_pEnt->DispatchTraceAttack(dmgInfo, vReflection, &tr);
			}

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
	ApplyMultiDamage();

	UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");
	ImpactParticles( &tr );

	CPVSFilter filter(tr.endpos);

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3f;

	AddViewKick();

	m_flChargeAmount = 0.0f;
	m_bJustFired = true;

	pOwner->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
}

//-----------------------------------------------------------------------------
// charge
//-----------------------------------------------------------------------------
void CWeaponGauss::ChargedFire(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (!pOwner){ return; }

	bool penetrated = false;

	WeaponSound(WPN_DOUBLE);

	SendWeaponAnim(ACT_VM_SECONDARYATTACK);
	StopChargeSound();

	m_bCharging = false;
	m_bChargeIndicated = false;

	m_flNextPrimaryAttack = gpGlobals->curtime + 1.5f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.5f;

	Vector	startPos = pOwner->Weapon_ShootPosition();
	Vector	aimDir = pOwner->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector	endPos = startPos + (aimDir * MAX_TRACE_LENGTH);

	trace_t	tr;
	UTIL_TraceLine(startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);
	ShouldDrawWaterImpacts(tr);
	ClearMultiDamage();

	m_vBeamPoints.Set( 0, tr.endpos );

	float flChargeAmount = (gpGlobals->curtime - m_flChargeStartTime) / MAX_GAUSS_CHARGE_TIME;


	if (flChargeAmount > 1.0f){ flChargeAmount = 1.0f; }


	float flDamage = sk_plr_dmg_gauss.GetFloat() + ((sk_plr_max_dmg_gauss.GetFloat()/* - sk_plr_dmg_gauss.GetFloat()*/) * flChargeAmount);

	CBaseEntity *pHit = tr.m_pEnt;

	bool bSkyboxHit = ((tr.surface.flags & SURF_SKY) > 0);

	if ( tr.DidHit() )
	{
		// wall
		UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");
		cplane_t impactPlane = tr.plane;
		Vector impactNormal = impactPlane.normal;
		QAngle impactAngles;
		VectorAngles(impactNormal, impactAngles);
		ImpactParticles( &tr );
		CPVSFilter filter(tr.endpos);
		Vector  vStore = tr.endpos;
		Vector	testPos = tr.endpos + (aimDir * 48.0f);

		UTIL_TraceLine(testPos, tr.endpos, MASK_SOLID_BRUSHONLY, pOwner, COLLISION_GROUP_NONE, &tr);

		if (!tr.allsolid && !bSkyboxHit) // at specific angles and positions the charged beam can penetrate skybox brushes, creating erroneous impact effects so we need an extra check
		{
			cplane_t impactPlane = tr.plane;
			Vector impactNormal = impactPlane.normal;
			QAngle impactAngles;
			VectorAngles(impactNormal, impactAngles);
			ImpactParticles( &tr );
			penetrated = true;

			trace_t backward_tr;
			UTIL_TraceLine( tr.endpos, vStore, MASK_SOLID_BRUSHONLY, pOwner, COLLISION_GROUP_NONE, &backward_tr );
			if (backward_tr.DidHit()){
				UTIL_ImpactTrace(&backward_tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");
			}

			
		}
	}
	if (pHit != NULL)
	{
		CTakeDamageInfo dmgInfo(this, pOwner, flDamage, DMG_CHARGEDGAUSS);
		CalculateBulletDamageForce(&dmgInfo, m_iPrimaryAmmoType, aimDir, tr.endpos);
		dmgInfo.ScaleDamageForce( flChargeAmount * 8 );
		dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
		pHit->DispatchTraceAttack(dmgInfo, aimDir, &tr);
	}

	ApplyMultiDamage();

	if ( !bSkyboxHit )
		UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat(-4.0f, -8.0f);
	viewPunch.y = random->RandomFloat(-0.25f, 0.25f);
	viewPunch.z = 0;

	pOwner->ViewPunch(viewPunch);

	Vector	recoilForce = pOwner->GetAbsVelocity() - pOwner->GetAutoaimVector(0) * (flDamage * 5.0f);
	recoilForce[2] += 12.80f;
	if ( recoilForce[2] > 350.0f ) recoilForce[2] = 350.0f; // goodbye gaussjumping, you will be missed o7
	pOwner->SetAbsVelocity(recoilForce);

	CPVSFilter filter(tr.endpos);
	if (penetrated)
	{
		//RadiusDamage(CTakeDamageInfo(this, this, flDamage, DMG_SHOCK), tr.endpos, 200.0f, CLASS_NONE, NULL);

		UTIL_TraceLine(tr.endpos, endPos, MASK_SHOT, pHit, COLLISION_GROUP_NONE, &tr);

		m_vBeamPoints.Set( 1, tr.startpos );
		m_vBeamPoints.Set( 2, tr.endpos );

		if ( !bSkyboxHit )
			UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");
		cplane_t impactPlane = tr.plane;
		Vector impactNormal = impactPlane.normal;
		QAngle impactAngles;
		VectorAngles(impactNormal, impactAngles);
		ImpactParticles( &tr );

		//RadiusDamage(CTakeDamageInfo(this, this, flDamage, DMG_SHOCK), tr.endpos, 200.0f, CLASS_NONE, NULL);
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
	}
	else{
		m_vBeamPoints.Set( 1, vec3_invalid );
		m_vBeamPoints.Set( 2, vec3_invalid );
	}
	m_flChargeAmount = flChargeAmount;
	m_bJustFired = true;

	pOwner->DoMuzzleFlash();

	pOwner->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
}

//-----------------------------------------------------------------------------
// handle dealing with ammo gain via suitcharger
//-----------------------------------------------------------------------------
void CWeaponGauss::RechargeAmmo(void){
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	Vector	vStartPos = pOwner->Weapon_ShootPosition();
	Vector	vAimDir = pOwner->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector	vEndPos = vStartPos + (vAimDir * PLAYER_USE_RADIUS);
	trace_t	tr;
	UTIL_TraceLine(vStartPos, vEndPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);
	
	CBaseEntity *pHit = tr.m_pEnt;
	if (pHit != NULL)
	{
		if (pHit->m_bSecondaryUse)
		{
			// fixme: this will inevitably break when some other entity gets a secondary use
			m_bSuitCharging = true;
			m_hCharger = pHit;
			pHit->Use(pOwner, this, USE_AMMO, 0);
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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::PrimaryAttack(void)
{
	//if( weapon_fire_mode.GetBool() == 0 )
	//{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (!pOwner){ return; }

	WeaponSound(SINGLE);

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pOwner->DoMuzzleFlash();

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);

	Fire();
	//}
	//else
	//{
	//	ChargeAttack();
	//}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::IncreaseCharge(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (m_flNextChargeTime > gpGlobals->curtime || !pOwner){ return; }


	if ((gpGlobals->curtime - m_flChargeStartTime) > MAX_GAUSS_CHARGE_TIME)
	{
		if (m_bChargeIndicated == false)
		{
			WeaponSound(SPECIAL1);
			m_bChargeIndicated = true;
		}

		if ((gpGlobals->curtime - m_flChargeStartTime) > DANGER_GAUSS_CHARGE_TIME)
		{

			WeaponSound(SPECIAL1);


			pOwner->TakeDamage(CTakeDamageInfo(this, this, 25, DMG_SHOCK | DMG_CRUSH));

			color32 gaussDamage = { 255, 128, 0, 128 };
			UTIL_ScreenFade(pOwner, gaussDamage, 0.2f, 0.2f, FFADE_IN);

			m_flNextChargeTime = gpGlobals->curtime + random->RandomFloat(0.5f, 2.5f);
		}
		return;
	}


	pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);

	int pitch = (gpGlobals->curtime - m_flChargeStartTime) * (150 / GetFullChargeTime()) + 100;
	if (pitch > 250){ pitch = 250; }
	if (m_sndCharge != NULL)
	{
		(CSoundEnvelopeController::GetController()).SoundChangePitch(m_sndCharge, pitch, 0);
	}


	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		ChargedFire();
		return;
	}

	m_flNextChargeTime = gpGlobals->curtime + GAUSS_CHARGE_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::SecondaryAttack(void)
{
	if (m_flNextSecondaryAttack > gpGlobals->curtime) {
		return;
	}
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (!pOwner || pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{ 
		WeaponSound( EMPTY );
		return; 
	}

	if (pOwner->GetWaterLevel() == 3)
	{
		EmitSound("Weapon_Gauss.Zap");
		SendWeaponAnim(ACT_VM_IDLE);
		m_flNextSecondaryAttack = gpGlobals->curtime + 3.0;

		pOwner->TakeDamage(CTakeDamageInfo(this, this, 25, DMG_SHOCK | DMG_CRUSH));
		return;
	}
	if (!m_bCharging)
	{

		SendWeaponAnim(ACT_VM_PULLBACK);


		if (!m_sndCharge)
		{
			CPASAttenuationFilter filter(this);
			m_sndCharge = (CSoundEnvelopeController::GetController()).SoundCreate(filter, entindex(), CHAN_STATIC, "Weapon_Gauss.Charge", ATTN_NORM);
		}

		if (m_sndCharge != NULL)
		{
			(CSoundEnvelopeController::GetController()).Play(m_sndCharge, 1.0f, 50);
			(CSoundEnvelopeController::GetController()).SoundChangePitch(m_sndCharge, 250, 3.0f);
		}

		m_flChargeStartTime = gpGlobals->curtime;
		m_bCharging = true;
		m_bChargeIndicated = false;
	}

	IncreaseCharge();
}

//-----------------------------------------------------------------------------
// Purpose:  view punch
//-----------------------------------------------------------------------------
void CWeaponGauss::AddViewKick(void)
{
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat(-0.5f, -0.2f);
	viewPunch.y = random->RandomFloat(-0.5f, 0.5f);
	viewPunch.z = 0;

	pPlayer->ViewPunch(viewPunch);
}

//-----------------------------------------------------------------------------
// Purpose: frames
//-----------------------------------------------------------------------------
void CWeaponGauss::ItemPostFrame(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if ( !pPlayer )
		return;
	
	bool bForceStopCharging = false;
	m_bJustFired = false;

	if (m_bSuitCharging && pPlayer->m_nButtons & IN_ATTACK)
	{
		pPlayer->m_nButtons &= ~IN_ATTACK; // make sure player can't interrupt a charge by holding attack1
	}
	if (pPlayer->m_afButtonReleased & IN_ATTACK2 || pPlayer->m_nButtons & IN_ZOOM)
	{
		if (m_bCharging)
			ChargedFire();
	}
	else if ( pPlayer->m_nButtons & IN_ATTACK2){
		pPlayer->m_nButtons &= ~(IN_ATTACK | IN_RELOAD); // don't let player interrupt charge with attack1, or use suitcharger
	}
	else if (pPlayer->m_nButtons & (IN_ATTACK | IN_USE)){
		pPlayer->m_nButtons &= ~IN_RELOAD; // suppress attack1/use and charge so weird stuff doesn't happen with suitcharger
		pPlayer->m_afButtonReleased &= IN_RELOAD;
		if (m_bSuitCharging)
			bForceStopCharging = true;
	}
	else if (pPlayer->m_nButtons & (IN_RELOAD) && gpGlobals->curtime >= NextPrimaryAttack()){
		RechargeAmmo();
	}
	if ((m_bSuitCharging && pPlayer->m_afButtonReleased & IN_RELOAD) || bForceStopCharging){
		SetNextPrimaryAttack(gpGlobals->curtime + GetFireRate()); // short cooldown
		m_bSuitCharging = false;
		m_bStartedCharging = false;
	}

	// todo: this is not the optimal way to do things i can feel it, but too tired to fix this now
	if (!g_pGameRules->CanHaveAmmo( pPlayer, GetPrimaryAmmoType() ))
	{
		// without disturbing any normal gauss/suitcharger function, allow the suitcharger to play its sound
		// while allowing an outro for full ammo
		if (pPlayer->m_afButtonReleased & IN_RELOAD)
			SetNextPrimaryAttack( gpGlobals->curtime + GetFireRate() );
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

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: charge sound
//-----------------------------------------------------------------------------
void CWeaponGauss::StopChargeSound(void)
{
	if (m_sndCharge != NULL)
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut(m_sndCharge, 0.1f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: holster
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGauss::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	StopChargeSound();
	m_bCharging = false;
	m_bChargeIndicated = false;
	m_bJustFired = false;

	return BaseClass::Holster(pSwitchingTo);
}
//-----------------------------------------------
// Purpose: charge time
//-----------------------------------------------
float CWeaponGauss::GetFullChargeTime(void)
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
//----------------------------------------------------------------------------------
// Purpose: splash
//----------------------------------------------------------------------------------
#define FSetBit(iBitVector, bits)	((iBitVector) |= (bits)) // LOLMEN : Set some bits to bit vec
//#define FBitSet(iBitVector, bit)	((iBitVector) & (bit))	// LOLMEN : Do that bit setted in bit vec?
#define TraceContents( vec ) ( enginetrace->GetPointContents( vec ) ) // LOLMEN : Do some test?
#define WaterContents( vec ) ( FBitSet( TraceContents( vec ), CONTENTS_WATER|CONTENTS_SLIME ) ) // Lolmen : For water

bool CWeaponGauss::ShouldDrawWaterImpacts(const trace_t &shot_trace)
{
	//FIXME: This doesn't handle the case of trying to splash while being underwater, but that's not going to look good
	//		 right now anyway...

	// We must start outside the water
	if (WaterContents(shot_trace.startpos))
		return false;

	// We must end inside of water
	if (!WaterContents(shot_trace.endpos))
		return false;

	trace_t	waterTrace;

	UTIL_TraceLine(shot_trace.startpos, shot_trace.endpos, (CONTENTS_WATER | CONTENTS_SLIME), UTIL_GetLocalPlayer(), COLLISION_GROUP_NONE, &waterTrace);


	if (waterTrace.fraction < 1.0f)
	{
		CEffectData	data;

		data.m_fFlags = 0;
		data.m_vOrigin = waterTrace.endpos;
		data.m_vNormal = waterTrace.plane.normal;
		data.m_flScale = random->RandomFloat(2.0, 4.0f); // Lolmen : ����������� ������ ������/����� ���

		// See if we hit slime
		if (FBitSet(waterTrace.contents, CONTENTS_SLIME))
		{
			FSetBit(data.m_fFlags, FX_WATER_IN_SLIME);
		}

		CPASFilter filter(data.m_vOrigin);
		te->DispatchEffect(filter, 0.0, data.m_vOrigin, "watersplash", data);
	}
	return true;
}

void CWeaponGauss::ImpactParticles( const trace_t *tr )
{
	QAngle impactAngles;
	VectorAngles( tr->plane.normal, impactAngles );
	if ( !(tr->surface.flags & SURF_SKY) ){
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