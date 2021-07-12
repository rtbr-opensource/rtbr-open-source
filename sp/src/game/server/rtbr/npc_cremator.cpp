//========= Copyright Iridium Incorporated, All rights reserved. ============//
// Made by Iridium77
// Cremator AI
//========================================================================//
#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "activitylist.h"
#include "ai_basenpc.h"
#include "te_particlesystem.h"
#include "particle_parse.h"
#include "engine/IEngineSound.h"
#include "props.h"
#include "decals.h"
#include "npc_cremator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int AE_CREM_MELEE1;

ConVar	sk_cremator_health("sk_cremator_health", "200");
ConVar	sk_cremator_dmg_melee("sk_cremator_dmg_melee", "15");
ConVar	sk_cremator_dmg_melee_force("sk_cremator_melee_force", "450");
ConVar	sk_cremator_dmg_immo("sk_cremator_dmg_immo", "2"); //Anything higher than this is OP
ConVar	sk_cremator_max_range("sk_cremator_max_range", "100");
ConVar	sk_cremator_immolator_color_r("sk_cremator_immolator_color_r", "0");
ConVar	sk_cremator_immolator_color_g("sk_cremator_immolator_color_g", "255");
ConVar	sk_cremator_immolator_color_b("sk_cremator_immolator_color_b", "0");
//ConVar	sk_cremator_immolator_beamsprite("sk_cremator_immolator_beamsprite", "sprites/physbeam.vmt");

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Cremator::Precache()
{
	BaseClass::Precache();

	PrecacheModel("models/cremator_npc_green.mdl");
	PrecacheModel("models/cremator_npc_purple.mdl");
	PrecacheModel("sprites/lgtning.vmt");

	PrecacheParticleSystem("immo_beam_muzzle02");

	PrecacheScriptSound("NPC_Cremator.Breathing");
	PrecacheScriptSound("NPC_Cremator.Pain");
	PrecacheScriptSound("NPC_Cremator.Die");
	PrecacheScriptSound("Weapon_Immolator.Flame_Start");
	PrecacheScriptSound("Weapon_Immolator.Flame_Stop");

	
}

/*void CNPC_Cremator::InitCustomSchedules(void)
{
	INIT_CUSTOM_AI(CNPC_Cremator);

	ADD_CUSTOM_SCHEDULE(CNPC_Cremator, SCHED_CREM_WANDER);
}*/

int CNPC_Cremator::RangeAttack1Conditions(float flDot, float flDist)
{
	if (GetNextAttack() > gpGlobals->curtime)
		return COND_NOT_FACING_ATTACK;

	if (flDot < DOT_30DEGREE)
		return COND_NOT_FACING_ATTACK;

	if (flDist >(InnateRange1MaxRange()))
		return COND_TOO_FAR_TO_ATTACK;

	if (flDist < (InnateRange1MinRange()))
		return COND_TOO_CLOSE_TO_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}

//=========================================================
// MeleeAttack1Conditions
//=========================================================
int CNPC_Cremator::MeleeAttack1Conditions(float flDot, float flDist)
{
	if (flDist > 48)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return 0;
	}
	else if (GetEnemy() == NULL)
	{
		return 0;
	}

	return COND_CAN_MELEE_ATTACK1;
}

void CNPC_Cremator::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();

	if (m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT) {
		IdleSound();
	}
	if (HasCondition(COND_HEAVY_DAMAGE) || HasCondition(COND_LIGHT_DAMAGE)) {
		CPASAttenuationFilter filter4(this);
		EmitSound(filter4, entindex(), "NPC_Cremator.Pain");
	}
	if (m_NPCState == NPC_STATE_ALERT) {
		CPASAttenuationFilter filter4(this);
		EmitSound(filter4, entindex(), "NPC_Cremator.Alert");
	}
}

void CNPC_Cremator::IdleSound(void)
{
	CPASAttenuationFilter filter4(this);
	EmitSound(filter4, entindex(), "NPC_Cremator.Breathing");
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//            In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_Cremator::BuildScheduleTestBits(void)
{
	BaseClass::BuildScheduleTestBits();

	if (gpGlobals->curtime < m_flNextAttack)
	{
		ClearCustomInterruptCondition(COND_CAN_RANGE_ATTACK1);
		ClearCustomInterruptCondition(COND_CAN_RANGE_ATTACK2);
	}
}
int CNPC_Cremator::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_ESTABLISH_LINE_OF_FIRE:
	{
		return SCHED_CREM_ELOF;
	}
	case SCHED_RANGE_ATTACK1:
	{
		return SCHED_CREM_RANGE_ATTACK1;
	}
	case SCHED_BACK_AWAY_FROM_ENEMY:
	{
		return SCHED_MELEE_ATTACK1;
	}
	default:
		return BaseClass::TranslateSchedule(scheduleType);
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Cremator::Spawn()
{
	Precache();
	if (random->RandomInt(0, 1) == 1) {
		SetModel("models/cremator_npc_green.mdl");
	}
	else {
		SetModel("models/cremator_npc_purple.mdl");
	}
	//m_nSkin = random->RandomInt(0, CREM_SKIN_COUNT - 1);
	//m_nSkin = 1;

	SetRenderColor(255, 255, 255, 255);
	
	SetHullType(HULL_MEDIUM_TALL);
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);
	m_bloodColor = BLOOD_COLOR_BLUE; //NOTE TO DL'ers: You really, really, really want to change this
	ClearEffects();
	m_iHealth = sk_cremator_health.GetFloat();
	m_flFieldOfView = 0.65;
	m_NPCState = NPC_STATE_NONE;


	CapabilitiesClear(); //Still can't open doors on his own for some reason... WTF
	CapabilitiesAdd(bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE | bits_CAP_DOORS_GROUP);
	CapabilitiesAdd(bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_MOVE_SHOOT | bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE);
	CapabilitiesAdd(bits_CAP_SQUAD | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE);
	CapabilitiesAdd(bits_CAP_INNATE_MELEE_ATTACK1);


	NPCInit();

	BaseClass::Spawn();
}

void CNPC_Cremator::Event_Killed(const CTakeDamageInfo &info)
{
	StopParticleEffects(this);
	if (m_nBody < CREM_BODY_GUNGONE)
	{
		// drop the gun!
		Vector vecGunPos;
		QAngle angGunAngles;

		SetBodygroup(1, CREM_BODY_GUNGONE);
		StopSound("NPC_Cremator.Breathing");
		CPASAttenuationFilter filter4(this);
		EmitSound(filter4, entindex(), "NPC_Cremator.Die");
		GetAttachment("0", vecGunPos, angGunAngles);

		angGunAngles.y += 180;
		DropItem("weapon_immolator", vecGunPos, angGunAngles);
	}

	BaseClass::Event_Killed(info);
}
int CNPC_Cremator::SelectSchedule() {
	if (HasSpawnFlags(SF_CREM_WANDER)) {
		switch (m_NPCState)
		{
		case NPC_STATE_IDLE:

		{
			return SCHED_CREM_WANDER;
		}

		if (HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return SCHED_SMALL_FLINCH;
		}

		if (GetEnemy() == NULL)
		{
			{
				return SCHED_CREM_WANDER;
			}
		}

		break;
		}
	}
	return BaseClass::SelectSchedule();
}
//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Cremator::Classify(void)
{
	return	CLASS_COMBINE;
}
//=========================================================
// ImmoBeam - heavy damage directly forward
//=========================================================
void CNPC_Cremator::ImmoBeam(int side)
{
	Vector laserStart;
	QAngle laserAngle;
	Vector forward, right, up;
	GetAttachment(LookupAttachment("1"), laserStart, laserAngle);
	DispatchParticleEffect("immo_beam_muzzle02", PATTACH_POINT_FOLLOW,
		this, "1", true);
	Vector muzzlePosition = laserStart;
	// + Vector(10.0f, 0.0f, 36.0f)
	Vector vecAim = GetShootEnemyDir(muzzlePosition);

	CCrematorPlasmaBall *pPlasmaBall = (CCrematorPlasmaBall *)CBaseEntity::Create("cremator_plasma_ball", muzzlePosition, laserAngle);
	//UTIL_SetOrigin(pPlasmaBall, vecSrc);
	//pPlasmaBall->SetLocalOrigin(GetAbsOrigin() + Vector(0.0f, 0.0f, 36.0f));
	pPlasmaBall->SetAbsAngles(laserAngle);
	pPlasmaBall->Spawn();
	//pPlasmaBall->SetOwnerEntity(pOwner);
	pPlasmaBall->SetBaseVelocity((vecAim * 512) + BaseClass::GetLocalVelocity());
	pPlasmaBall->SetContextThink(&CCrematorPlasmaBall::SUB_Remove, gpGlobals->curtime + sk_cremator_max_range.GetInt() / 100, "KillBoltThink");
	//pPlasmaBall->SetCollisionGroup(COLLISION_GROUP_NONE);
}

//=========================================================
// HandleAnimEvent 
//=========================================================
void CNPC_Cremator::HandleAnimEvent(animevent_t *pEvent)
{
	if (pEvent->type & AE_TYPE_NEWEVENTSYSTEM)
	{
		if (pEvent->event == AE_CREM_MELEE1) {
			CBaseEntity *pHurt = CheckTraceHullAttack(70, -Vector(16, 16, 18), Vector(16, 16, 18), 0, DMG_CLUB);
			CBaseCombatCharacter* pBCC = ToBaseCombatCharacter(pHurt);
			if (pBCC)
			{
				Vector attackDir = (pHurt->WorldSpaceCenter() - GetAbsOrigin());
				VectorNormalize(attackDir);
				Vector offset = RandomVector(-32, 32) + pHurt->WorldSpaceCenter();

				// Generate enough force to make a 75kg guy move away at 700 in/sec
				Vector vecForce = attackDir * sk_cremator_dmg_melee_force.GetFloat();

				pHurt->ApplyAbsVelocityImpulse(vecForce);

				// Deal the damage
				CTakeDamageInfo	info(this, this, vecForce, offset, sk_cremator_dmg_melee.GetFloat(), DMG_CLUB);
				pHurt->TakeDamage(info);
			}
		}
		else {
			BaseClass::HandleAnimEvent(pEvent);
		}
	}
	else {
		switch (pEvent->event)
		{
		case CREM_AE_IMMO_SHOOT:
		{

			if (m_hDead != NULL)
			{
				Vector vecDest = m_hDead->GetAbsOrigin() + Vector(0, 0, 38);
				trace_t trace;
				UTIL_TraceHull(vecDest, vecDest, GetHullMins(), GetHullMaxs(), MASK_SOLID, m_hDead, COLLISION_GROUP_NONE, &trace);
			}

			ClearMultiDamage();

			//ImmoBeam( -1 );
			ImmoBeam(1);

			CPASAttenuationFilter filter4(this);
			 EmitSound(filter4, entindex(), "Weapon_Immolator.Flame_Start");
			ApplyMultiDamage();

			CBroadcastRecipientFilter filter;
			te->DynamicLight(filter, 0.0, &GetAbsOrigin(), sk_cremator_immolator_color_r.GetFloat(), sk_cremator_immolator_color_g.GetFloat(), sk_cremator_immolator_color_b.GetFloat(), 2, 256, 0.2 / m_flPlaybackRate, 0);

			m_flNextAttack = gpGlobals->curtime + random->RandomFloat(0.5, 4.0);
		}
		break;

		case CREM_AE_IMMO_DONE:
		{
		StopParticleEffects(this);
		StopSound("Weapon_Immolator.Flame_Start");
		StopSound("Weapon_Immolator.Flame_stop");
		}
		break;

		default:
			BaseClass::HandleAnimEvent(pEvent);
			break;
		}
	}
}
AI_BEGIN_CUSTOM_NPC(npc_cremator, CNPC_Cremator)
DECLARE_ANIMEVENT(AE_CREM_MELEE1)
DEFINE_SCHEDULE
(
SCHED_CREM_WANDER,
"	Tasks"
"		TASK_STOP_MOVING				0"
"		TASK_WANDER						256"
"		TASK_WALK_PATH					0"
"		TASK_WAIT_FOR_MOVEMENT			0"
"		TASK_STOP_MOVING				0"
"		TASK_FACE_REASONABLE			0"
"		TASK_WAIT						3"
"		TASK_WAIT_RANDOM				3"
"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_CREM_WANDER" // keep doing it
""
"	Interrupts"
"		COND_ENEMY_DEAD"
"		COND_LIGHT_DAMAGE"
"		COND_HEAVY_DAMAGE"
"		COND_HEAR_DANGER"
"		COND_HEAR_MOVE_AWAY"
"		COND_NEW_ENEMY"
"		COND_SEE_ENEMY"
"		COND_CAN_RANGE_ATTACK1"
"		COND_CAN_RANGE_ATTACK2"

)
DEFINE_SCHEDULE
(
SCHED_CREM_ELOF,
"	Tasks"
"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_FAIL_ESTABLISH_LINE_OF_FIRE"
"		TASK_SET_TOLERANCE_DISTANCE		48"
"		TASK_GET_PATH_TO_ENEMY_LKP_LOS	0"
"		TASK_RUN_PATH					0"
"		TASK_WAIT_FOR_MOVEMENT			0"
"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
""
"	Interrupts"
"		COND_NEW_ENEMY"
"		COND_ENEMY_DEAD"
"		COND_CAN_RANGE_ATTACK1"
"		COND_CAN_RANGE_ATTACK2"
"		COND_CAN_MELEE_ATTACK1"
"		COND_CAN_MELEE_ATTACK2"
"		COND_HEAR_DANGER"
"		COND_HEAR_MOVE_AWAY"
"		COND_HEAVY_DAMAGE"
"		COND_SEE_ENEMY"

)
DEFINE_SCHEDULE
(
SCHED_CREM_RANGE_ATTACK1,

"	Tasks"
"		TASK_STOP_MOVING		0"
"		TASK_FACE_ENEMY			0"
"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
"		TASK_RANGE_ATTACK1		0"
""
"	Interrupts"
"		COND_NEW_ENEMY"
"		COND_ENEMY_DEAD"
"		COND_HEAVY_DAMAGE"
"		COND_ENEMY_OCCLUDED"
"		COND_NO_PRIMARY_AMMO"
"		COND_HEAR_DANGER"
"		COND_WEAPON_BLOCKED_BY_FRIEND"
"		COND_WEAPON_SIGHT_OCCLUDED"
);

AI_END_CUSTOM_NPC()

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CCrematorPlasmaBall::CreateVPhysics(void)
{
	// Create the object in the physics system
	VPhysicsInitNormal(SOLID_BBOX, FSOLID_NOT_STANDABLE, false);

	return true;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
unsigned int CCrematorPlasmaBall::PhysicsSolidMaskForEntity() const
{
	return (BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX) & ~CONTENTS_GRATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrematorPlasmaBall::Spawn(void)
{
	Precache();
	PrecacheParticleSystem("burning_character_immo");
	SetModel("models/immolator_bolt.mdl");
	SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM);
	UTIL_SetSize(this, -Vector(0.3, 0.3f, 0.3f), Vector(0.3f, 0.3f, 0.3f));
	SetSolid(SOLID_VPHYSICS);
	SetGravity(0.05f);

	DispatchParticleEffect("immo_beam_fire02", PATTACH_ABSORIGIN_FOLLOW, this);
	SetTouch(&CCrematorPlasmaBall::BoltTouch);
}


void CCrematorPlasmaBall::Precache(void)
{
	PrecacheParticleSystem("immo_beam_fire02");
	PrecacheModel("models/immolator_bolt.mdl");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCrematorPlasmaBall::BoltTouch(CBaseEntity *pOther)
{
	Assert(pOther);
	if (!pOther->IsSolid())
		return;

	//If the flare hit a person or NPC, do damage here.
	if (pOther && pOther->m_takedamage)
	{
		IgniteOtherIfAllowed(pOther);
		SUB_Remove();
		return;
	}

	// hit the world, check the material type here, see if the flare should stick.
	trace_t tr;
	Vector traceDir = GetAbsVelocity();
	//tr = BaseClass::GetTouchTrace();
	UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + traceDir * 64, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	//Scorch decal
	if (GetAbsVelocity().LengthSqr() > (250 * 250))
	{
		/*int index = decalsystem->GetDecalIndexForName("FadingScorch");
		if (index >= 0)
		{
		CBroadcastRecipientFilter filter;
		te->Decal(filter, 0.0, &tr.endpos, &tr.startpos, ENTINDEX(tr.m_pEnt), tr.hitbox, index);

		}*/
		//DecalTrace(&tr, "FadingScorch");
		UTIL_DecalTrace(&tr, "FadingScorch");
	}

	StopParticleEffects(this);
	SUB_Remove();
}

void CCrematorPlasmaBall::IgniteOtherIfAllowed(CBaseEntity * pOther)
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	// Don't burn the player
	if (pOther->IsPlayer())
		pPlayer->IgniteLifetimeGreen(3);

	CAI_BaseNPC *pNPC;
	pNPC = dynamic_cast<CAI_BaseNPC*>(pOther);
	if (pNPC) {

		// Don't burn boss enemies
		if (FStrEq(STRING(pNPC->m_iClassname), "npc_combinegunship")
			|| FStrEq(STRING(pNPC->m_iClassname), "npc_combinedropship")
			|| FStrEq(STRING(pNPC->m_iClassname), "npc_strider")
			|| FStrEq(STRING(pNPC->m_iClassname), "npc_helicopter")
			|| FStrEq(STRING(pNPC->m_iClassname), "npc_cremator")
			)
			return;

		// Burn this NPC
		pNPC->IgniteLifetimeGreen(3);
	}

	// If this is a breakable prop, ignite it!
	CBreakableProp *pBreakable;
	pBreakable = dynamic_cast<CBreakableProp*>(pOther);
	if (pBreakable)
	{
		pBreakable->IgniteLifetimeGreen(3);
		// Don't do damage to props that are on fire
		if (pBreakable->IsOnFire())
			return;
	}

	// Do damage
	pOther->TakeDamage(CTakeDamageInfo(this, this, sk_cremator_dmg_immo.GetInt(), (DMG_BULLET | DMG_BURN)));

}