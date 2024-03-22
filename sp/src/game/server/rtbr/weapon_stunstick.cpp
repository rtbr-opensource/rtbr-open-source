//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Stun Stick- beating stick with a zappy end
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "weapon_stunstick.h"
#include "dlight.h"
#include "r_efx.h"
#include "sprite.h"
#include "IEffects.h"
#include "debugoverlay_shared.h"
#include "in_buttons.h"
#include "basehlcombatweapon.h"
#include "npc_metropolice.h"
#include "te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar metropolice_move_and_melee;


#ifdef MAPBASE
ConVar    sk_plr_dmg_stunstick("sk_plr_dmg_stunstick", "0");
ConVar    sk_npc_dmg_stunstick("sk_npc_dmg_stunstick", "0");
#endif

//-----------------------------------------------------------------------------
// CWeaponStunStick
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponStunStick, DT_WeaponStunStick)

BEGIN_NETWORK_TABLE(CWeaponStunStick, DT_WeaponStunStick)
SendPropInt(SENDINFO(m_bActive), 1, SPROP_UNSIGNED),
SendPropFloat(SENDINFO(m_flChargeLevel), -1, SPROP_PROXY_ALWAYS_YES),
END_NETWORK_TABLE()


BEGIN_PREDICTION_DATA(CWeaponStunStick)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_stunstick, CWeaponStunStick);
PRECACHE_WEAPON_REGISTER(weapon_stunstick);


acttable_t	CWeaponStunStick::m_acttable[] =
{
	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, true },
};

IMPLEMENT_ACTTABLE(CWeaponStunStick);

BEGIN_DATADESC(CWeaponStunStick) 
DEFINE_FIELD(m_flChargeStartTime, FIELD_FLOAT),
DEFINE_FIELD(m_bIsCharging, FIELD_BOOLEAN),
DEFINE_FIELD(m_hSparkSprite, FIELD_EHANDLE),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Fidget Animation Stuff
//-----------------------------------------------------------------------------
void CWeaponStunStick::ItemPostFrame(void)
{
	if (!m_bIsCharging) {
		m_flChargeStartTime = gpGlobals->curtime;
	}

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	float	ChargeTime = (gpGlobals->curtime - m_flChargeStartTime);
	float chargeLevel = ChargeTime / STUNSTICK_MAX_CHARGE;
	m_flChargeLevel = chargeLevel;

	if (pPlayer->m_nButtons & IN_ATTACK2)
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.25;
		if (!m_bIsCharging && GetActivity() != ACT_VM_FIRSTDRAW && m_flNextSecondaryAttack <= gpGlobals->curtime) {
			m_flChargeStartTime = gpGlobals->curtime;
			m_bIsCharging = true;
			WeaponSound( WPN_DOUBLE );
			SendWeaponAnim(ACT_VM_HAULBACK);
			//DevMsg("ButtonClicked \n");
		}
		
		pPlayer->m_nButtons &= ~IN_ATTACK3;
	}

	if (pPlayer->m_afButtonReleased & IN_ATTACK2 && m_bIsCharging && GetActivity() != ACT_VM_FIRSTDRAW)
	{
		//DevMsg("ButtonReleased \n");
		ChargedAttack();
	}

	if (ChargeTime >= STUNSTICK_MAX_CHARGE && m_bIsCharging && GetActivity() != ACT_VM_FIRSTDRAW)
	{
		//DevMsg("ButtonReleased \n");
		ChargedAttack();
	}

	BaseClass::ItemPostFrame();
}
//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponStunStick::CWeaponStunStick(void)
{
	// HACK:  Don't call SetStunState because this tried to Emit a sound before
	//  any players are connected which is a bug
	m_bActive = false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponStunStick::Spawn()
{
	Precache();

	BaseClass::Spawn();
	AddSolidFlags(FSOLID_NOT_STANDABLE);
}

void CWeaponStunStick::Precache()
{
	BaseClass::Precache();
	PrecacheScriptSound("Weapon_StunStick.Activate");
	PrecacheScriptSound("Weapon_StunStick.Deactivate");
	PrecacheParticleSystem("weapon_stunstick_impact");

	PrecacheModel(STUNSTICK_BEAM_MATERIAL);
	PrecacheModel(STUNSTICK_GLOW_MATERIAL);
	PrecacheModel(STUNSTICK_GLOW_MATERIAL2);
	PrecacheModel(STUNSTICK_EFFECT_MATERIAL);
	PrecacheModel("sprites/light_glow02_add_noz.vmt");
}
//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponStunStick::GetDamageForActivity(Activity hitActivity)
{
#ifdef MAPBASE
	if ((GetOwner() != NULL) && (GetOwner()->IsPlayer()))
		return sk_plr_dmg_stunstick.GetFloat();

	return sk_npc_dmg_stunstick.GetFloat();
#else
	return 40.0f;
#endif
}

//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
//-----------------------------------------------------------------------------
extern ConVar sk_crowbar_lead_time;

void CWeaponStunStick::PrimaryAttack() {
	if ( m_flNextPrimaryAttack <= gpGlobals->curtime ) {
		BaseClass::PrimaryAttack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStunStick::ImpactEffect(trace_t &traceHit)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL) {
		return;
	}

	CEffectData	data;

	data.m_vNormal = traceHit.plane.normal;
	data.m_vOrigin = traceHit.endpos + (data.m_vNormal * 4.0f);

	Vector vecDir;

	AngleVectors(pOwner->EyeAngles(), &vecDir);

	QAngle EyeAngle = pOwner->EyeAngles();
	Vector vecAbsStart = pOwner->EyePosition();
	Vector vecAbsEnd = vecAbsStart + (vecDir * 72);

	trace_t tr;
	UTIL_TraceLine(vecAbsStart, vecAbsEnd, MASK_ALL, pOwner, COLLISION_GROUP_NONE, &tr);

	cplane_t impactPlane = tr.plane;
	Vector impactNormal = impactPlane.normal;
	QAngle impactAngles;
	VectorAngles(impactNormal, impactAngles);

	//DispatchParticleEffect("weapon_stunstick_impact", PATTACH_POINT_FOLLOW, pOwner->GetViewModel(), "1", true);
	if (tr.DidHit()) {
		DispatchParticleEffect("weapon_stunstick_impact", tr.endpos, impactAngles, this);
	}

	UTIL_ImpactTrace(&traceHit, DMG_CLUB);
}
//-----------------------------------------------------------------------------
// Purpose: Calculates stunstick charge damage
//-----------------------------------------------------------------------------

int	CWeaponStunStick::GetChargedDmg(void) {
	float	ChargeTime = (gpGlobals->curtime - m_flChargeStartTime);
	if (ChargeTime < (STUNSTICK_MAX_CHARGE)) {
		return ChargeTime * 15;
	}
	else {
		return 83;
	}
}

void CWeaponStunStick::ChargedAttack() {
	float	ChargeTime = (gpGlobals->curtime - m_flChargeStartTime);
	if (ChargeTime >= 0.5f) {
		BaseClass::PrimaryAttack(GetChargedDmg(), DMG_CLUB | DMG_SHOCK);
		m_flNextSecondaryAttack = gpGlobals->curtime + 1;
	}
	else {
		StopWeaponSound(WPN_DOUBLE);  // stop sound so it doesn't keep playing when we stop early
		SendWeaponAnim(ACT_VM_IDLE);
	}
	m_bIsCharging = false;
}


int CWeaponStunStick::WeaponMeleeAttack1Condition(float flDot, float flDist)
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
	CAI_BaseNPC *pNPC = GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	AngularImpulse angVelocity;
	pEnemy->GetVelocity(&vecVelocity, &angVelocity);

	// Project where the enemy will be in a little while, add some randomness so he doesn't always hit
	float dt = sk_crowbar_lead_time.GetFloat();
	dt += random->RandomFloat(-0.3f, 0.2f);
	if (dt < 0.0f)
		dt = 0.0f;

	Vector vecExtrapolatedPos;
	VectorMA(pEnemy->WorldSpaceCenter(), dt, vecVelocity, vecExtrapolatedPos);

	Vector vecDelta;
	VectorSubtract(vecExtrapolatedPos, pNPC->WorldSpaceCenter(), vecDelta);

	if (fabs(vecDelta.z) > 70)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	Vector vecForward = pNPC->BodyDirection2D();
	vecDelta.z = 0.0f;
	float flExtrapolatedDot = DotProduct2D(vecDelta.AsVector2D(), vecForward.AsVector2D());
	if ((flDot < 0.7) && (flExtrapolatedDot < 0.7))
	{
		return COND_NOT_FACING_ATTACK;
	}

	float flExtrapolatedDist = Vector2DNormalize(vecDelta.AsVector2D());

	if (pEnemy->IsPlayer())
	{
		//Vector vecDir = pEnemy->GetSmoothedVelocity();
		//float flSpeed = VectorNormalize( vecDir );

		// If player will be in front of me in one-half second, clock his arse.
		Vector vecProjectEnemy = pEnemy->GetAbsOrigin() + (pEnemy->GetAbsVelocity() * 0.35);
		Vector vecProjectMe = GetAbsOrigin();

		if ((vecProjectMe - vecProjectEnemy).Length2D() <= 48.0f)
		{
			return COND_CAN_MELEE_ATTACK1;
		}
	}
	/*
	if( metropolice_move_and_melee.GetBool() )
	{
	if( pNPC->IsMoving() )
	{
	flTargetDist *= 1.5f;
	}
	}
	*/
	float flTargetDist = 48.0f;
	if ((flDist > flTargetDist) && (flExtrapolatedDist > flTargetDist))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}


void CWeaponStunStick::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_MELEE_HIT:
	{
		// Trace up or down based on where the enemy is...
		// But only if we're basically facing that direction
		Vector vecDirection;
		AngleVectors(GetAbsAngles(), &vecDirection);

		CBaseEntity *pEnemy = pOperator->MyNPCPointer() ? pOperator->MyNPCPointer()->GetEnemy() : NULL;
		if (pEnemy)
		{
			Vector vecDelta;
			VectorSubtract(pEnemy->WorldSpaceCenter(), pOperator->Weapon_ShootPosition(), vecDelta);
			VectorNormalize(vecDelta);

			Vector2D vecDelta2D = vecDelta.AsVector2D();
			Vector2DNormalize(vecDelta2D);
			if (DotProduct2D(vecDelta2D, vecDirection.AsVector2D()) > 0.8f)
			{
				vecDirection = vecDelta;
			}
		}

		Vector vecEnd;
		VectorMA(pOperator->Weapon_ShootPosition(), 32, vecDirection, vecEnd);
		// Stretch the swing box down to catch low level physics objects
		CBaseEntity *pHurt = pOperator->CheckTraceHullAttack(pOperator->Weapon_ShootPosition(), vecEnd,
			Vector(-16, -16, -40), Vector(16, 16, 16), GetDamageForActivity(GetActivity()), DMG_CLUB, 0.5f, false);

		// did I hit someone?
		if (pHurt)
		{
			// play sound
			WeaponSound(MELEE_HIT);

			CBasePlayer *pPlayer = ToBasePlayer(pHurt);

			bool bFlashed = false;

#ifdef MAPBASE
			CNPC_MetroPolice *pCop = dynamic_cast<CNPC_MetroPolice *>(pOperator);

			if (pCop != NULL && pPlayer != NULL)
			{
				// See if we need to knock out this target
				if (pCop->ShouldKnockOutTarget(pHurt))
				{
					float yawKick = random->RandomFloat(-48, -24);

					//Kick the player angles
					pPlayer->ViewPunch(QAngle(-16, yawKick, 2));

					color32 white = { 255, 255, 255, 255 };
					UTIL_ScreenFade(pPlayer, white, 0.2f, 1.0f, FFADE_OUT | FFADE_PURGE | FFADE_STAYOUT);
					bFlashed = true;

					pCop->KnockOutTarget(pHurt);

					break;
				}
				else
				{
					// Notify that we've stunned a target
					pCop->StunnedTarget(pHurt);
				}
			}
#endif

			// Punch angles
			if (pPlayer != NULL && !(pPlayer->GetFlags() & FL_GODMODE))
			{
				float yawKick = random->RandomFloat(-48, -24);

				//Kick the player angles
				pPlayer->ViewPunch(QAngle(-16, yawKick, 2));

				Vector	dir = pHurt->GetAbsOrigin() - GetAbsOrigin();

				// If the player's on my head, don't knock him up
				if (pPlayer->GetGroundEntity() == pOperator)
				{
					dir = vecDirection;
					dir.z = 0;
				}

				VectorNormalize(dir);

				dir *= 500.0f;

				//If not on ground, then don't make them fly!
				if (!(pPlayer->GetFlags() & FL_ONGROUND))
					dir.z = 0.0f;

				//Push the target back
				pHurt->ApplyAbsVelocityImpulse(dir);

				if (!bFlashed)
				{
					color32 red = { 128, 0, 0, 128 };
					UTIL_ScreenFade(pPlayer, red, 0.5f, 0.1f, FFADE_IN);
				}

				// Force the player to drop anyting they were holding
				pPlayer->ForceDropOfCarriedPhysObjects();
			}

			// do effect?
		}
		else
		{
			WeaponSound(MELEE_MISS);
		}
	}
	break;
	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}

void CWeaponStunStick::OnPickedUp(CBaseCombatCharacter *pNewOwner)
{
	BaseClass::OnPickedUp(pNewOwner);
	//SendWeaponAnim(ACT_VM_FIRSTDRAW);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the state of the stun stick
//-----------------------------------------------------------------------------
void CWeaponStunStick::SetStunState(bool state)
{
	m_bActive = state;
	CBaseCombatCharacter *pOwner = GetOwner();
	if (m_bActive)
	{
		//FIXME: START - Move to client-side
		//if (pOwner->IsNPC()) {
		Vector vecAttachment;
		QAngle vecAttachmentAngles;
		GetAttachment("1", vecAttachment, vecAttachmentAngles);
		if (pOwner->IsNPC()) {
			g_pEffects->Sparks(vecAttachment);
		}

		//}
		//FIXME: END - Move to client-side

		EmitSound("Weapon_StunStick.Activate");
	}
	else
	{
		EmitSound("Weapon_StunStick.Deactivate");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponStunStick::Deploy(void)
{
	SetStunState(true);
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponStunStick::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	SendWeaponAnim(ACT_VM_HOLSTER);

	m_bIsCharging = false;

	if (BaseClass::Holster(pSwitchingTo) == false)
		return false;

	SetStunState(false);
	//SetWeaponVisible( false );

	return true;
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecVelocity - 
//-----------------------------------------------------------------------------
void CWeaponStunStick::Drop(const Vector &vecVelocity)
{
	SetStunState(false);

#ifdef MAPBASE
	BaseClass::Drop(vecVelocity);
#else
	UTIL_Remove(this);
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponStunStick::GetStunState(void)
{
	return m_bActive;
}

bool CWeaponStunStick::ShouldDisplayAltFireHUDHint(){
	if ( GetHudHintCount() >= WEAPON_ALTFIRE_HUD_HINT_COUNT )
		return false;
	return true;
}