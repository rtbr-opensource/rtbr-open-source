//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// This is a skeleton file for use when creating a new 
// NPC. Copy and rename this file for the new
// NPC and add the copy to the build.
//
// Replace occurrences of CNPC_SandBarnacle with the new NPC's
// classname. Don't forget the lower-case occurrence in 
// LINK_ENTITY_TO_CLASS()
//
//
// ASSUMPTIONS MADE:
//
// You're making a character based on CAI_BaseNPC. If this 
// is not true, make sure you replace all occurrences
// of 'CAI_BaseNPC' in this file with the appropriate  
// parent class.
//
// You're making a human-sized NPC that walks.
//
//=============================================================================//
#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_squad.h"
#include "soundent.h"
#include "util.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "activitylist.h"
#include "particle_parse.h"
#include "particle_system.h"
#include "ai_basenpc.h"
#include "bone_setup.h"
#include "engine/IEngineSound.h"
#include "vehicle_base.h"
#include "physics_prop_ragdoll.h"
#include "baseanimating.h"
#include "particle_parse.h"
#include "rtbr_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_sandbarnacle_health( "sk_sandbarnacle_health", "0" );
ConVar sk_sandbarnacle_dmg( "sk_sandbarnacle_dmg", "0" );

ConVar g_debug_sandbarnacle( "g_debug_sandbarnacle", "0", FCVAR_CHEAT );

#define SANDBARNACLE_MAX_GRAB_DISTANCE					125.0f
#define SANDBARNACLE_MAX_AMBUSH_GRAB_DISTANCE			145.0f

// the final value is between either of theese two based on the vehicle speed
#define SANDBARNACLE_MAX_VEHICLE_GRAB_DISTANCE_MIN		215.0f
#define SANDBARNACLE_MAX_VEHICLE_GRAB_DISTANCE_MAX		245.0f

#define SANDBARNACLE_CONSUME_BLOODFX_RED				"blood_impact_red_01"
#define SANDBARNACLE_CONSUME_BLOODFX_GREEN				"blood_impact_green_01"
#define SANDBARNACLE_CONSUME_BLOODFX_YELLOW				"blood_impact_yellow_01"
#define SANDBARNACLE_CONSUME_BLOODFX_ANTLION			"AntlionGib"
#define SANDBARNACLE_CONSUME_BLOODFX_ZOMBIE				"blood_impact_zombie_01"
#define SANDBARNACLE_CONSUME_BLOODFX_ANTLION_WORKER		"antlion_gib_02"

#define IS_EASY		g_pGameRules->IsSkillLevel(SKILL_EASY)
#define IS_MEDIUM	g_pGameRules->IsSkillLevel(SKILL_MEDIUM)
#define IS_HARD		g_pGameRules->IsSkillLevel(SKILL_HARD)

// if the thing should use the vehicle ambush attack even when prey is on foot
// makes the barnacle grab you VERY quickly
#define SANDBARNACLE_SHOULD_AMBUSH_BY_DEFAULT (IS_MEDIUM || IS_HARD)

// there are instances where the player has to platform across a pit with sandbarnacles in it
// sometimes these pits are less than SANDBARNACLE_MAX_GRAB_DISTANCE units tall, which causes the sandbarnacle to grab the player directly from the platforms
// this exists to prevent that
#define SANDBARNACLE_MAX_VERTICAL_GRAB_DISTANCE 30.0f

// Forward declaration
class CSandBarnacleTongueTip;

int AE_SB_GRAB;
int AE_SB_AMBUSH_GRAB;
int AE_SB_KILL;
int AE_SB_PULLDOWN;

//=========================================================
//=========================================================
class CNPC_SandBarnacle : public CAI_BaseNPC
{
	DECLARE_CLASS(CNPC_SandBarnacle, CAI_BaseNPC);
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

public:
	void	Precache(void);
	void	Spawn(void);
	Vector	FindBestSpawnPosition(void);
	void	PrescheduleThink(void);
	virtual void GatherConditions( void );

	void	NPCThink(void);
	virtual void Event_Killed(const CTakeDamageInfo &info);

	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info ); // todo, vehicle grab logic
	int TranslateSchedule(int scheduleType);
	virtual Activity	TranslateActivity(Activity idealActivity, Activity* pIdealWeaponActivity);
	virtual void		OnChangeActivity(Activity eNewActivity);

	virtual void		StartTask( const Task_t *pTask );

	Class_T				Classify(void);
	virtual int			MeleeAttack1Conditions(float flDot, float flDist);
	virtual int			MeleeAttack2Conditions(float flDot, float flDist); // todo, vehicle grab logic
	virtual int			SelectSchedule();

	void HandleAnimEvent(animevent_t* pEvent);
	float MaxYawSpeed(void);
	virtual float GetViewDistance() { return 128.0f; } // TODO: Make with work with vehicles vs on foot.

	virtual Activity	GetDeathActivity( void ) { return ACT_DIESIMPLE; } // We don't ragdoll.
	virtual bool		CanBecomeRagdoll( void ) { return false; }
	virtual bool		BecomeRagdoll(const CTakeDamageInfo& info, const Vector& forceVector) { return false; }

	CRagdollProp* AttachRagdollToTongue(CBaseAnimating* pAnimating);
	virtual bool CanBoogie() OVERRIDE { return false; }

private:
	// -------------
	// Sounds
	// -------------
	void			IdleSound( void );
	void			DeathSound( void );
	void			PainSound( void );

	void			SetSmallHullSize( bool bSmall);
	void			RemoveCorpse();
	bool			m_bIsSmallHull;

	float	m_flTimeSinceCombat;
	EHANDLE m_hAttached;
	EHANDLE m_hBreathParticle;

	// the last position of our prey before we grabbed, used to place them back to where they were if they kill us before we kill them
	Vector m_vAttachedLastPosition;
	
	CHandle<CRagdollProp>				m_hRagdoll;
	CHandle<CSandBarnacleTongueTip>		m_hTongueTip;
	bool	m_bWantsToBeUnderground;
	bool	m_bHasSmallHull;
	bool	m_bShouldPlayDigestionSound;
	bool	m_bIsAmbushing;

	float	m_flNextIdleSoundTime;
	float	m_flNextDigestSoundTime;
	float	m_flNextPainSoundTime;
	float	m_flFinishedDigestionTime;

	// changes based on the vehicle speed
	// used to see if barnacle should attempt a grab, but also to check if grab should succeed
	float	m_flCurrentMaxVehicleGrabDistance;

private:

	enum
	{
		SCHED_SB_IDLE = LAST_SHARED_SCHEDULE,
		SCHED_SB_UNDERIDLE,
		SCHED_SB_DIGESTIDLE,
		SCHED_SB_FACE_PREY,
		SCHED_SB_PULLDOWN,
		SCHED_SB_BURROW,
		SCHED_SB_POPUP,
		SCHED_SB_MELEE_ATTACK1, // regular grab
		SCHED_SB_MELEE_ATTACK2, // vehicle ambush
		LAST_SB_SCHEDULE,
	};

	enum
	{
		TASK_SB_EMIT_PARTICLES = LAST_SHARED_TASK,
	};

	enum
	{
		COND_SB_PREY_WITHIN_RANGE = LAST_SHARED_CONDITION,
		COND_SB_PREY_ATTACHED,
	};
};

LINK_ENTITY_TO_CLASS(npc_sandbarnacle, CNPC_SandBarnacle);

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_SandBarnacle)
DEFINE_FIELD(m_hAttached, FIELD_EHANDLE),
DEFINE_FIELD(m_hBreathParticle, FIELD_EHANDLE),
DEFINE_FIELD(m_vAttachedLastPosition, FIELD_VECTOR),
DEFINE_FIELD(m_hTongueTip, FIELD_EHANDLE),
DEFINE_FIELD(m_hRagdoll, FIELD_EHANDLE),
DEFINE_FIELD(m_bWantsToBeUnderground, FIELD_BOOLEAN),
DEFINE_FIELD(m_bShouldPlayDigestionSound, FIELD_BOOLEAN),
DEFINE_FIELD(m_bIsSmallHull, FIELD_BOOLEAN),
DEFINE_FIELD(m_flNextIdleSoundTime, FIELD_TIME),
DEFINE_FIELD(m_flNextDigestSoundTime, FIELD_TIME),
DEFINE_FIELD(m_flNextPainSoundTime, FIELD_TIME),
DEFINE_FIELD(m_flFinishedDigestionTime, FIELD_TIME),
DEFINE_FIELD(m_flTimeSinceCombat, FIELD_TIME),
END_DATADESC()

//=========================================================
// Sand Barnacle Tongue Tip
//=========================================================
class CSandBarnacleTongueTip : public CBaseAnimating
{
	DECLARE_CLASS( CSandBarnacleTongueTip, CBaseAnimating );
	DECLARE_DATADESC();

public:
	virtual void Precache( void );
	virtual void Spawn( void );
	void UpdateTongue( void );

	CNPC_SandBarnacle* GetOwnerBarnacle( void ) { return m_hBarnacle.Get(); }

	static CSandBarnacleTongueTip *CreateTongue( class CNPC_SandBarnacle *pBarnacle, Vector vecOrigin, QAngle vecAngles);

private:
	CHandle<CNPC_SandBarnacle>	m_hBarnacle;
};

LINK_ENTITY_TO_CLASS( npc_sandbarnacle_tongue_tip, CSandBarnacleTongueTip );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CSandBarnacleTongueTip)
DEFINE_FIELD(m_hBarnacle, FIELD_EHANDLE),
DEFINE_THINKFUNC(UpdateTongue),
END_DATADESC()

void CSandBarnacleTongueTip::Precache( void )
{
	PrecacheModel( "models/props_junk/rock001a.mdl" );

	BaseClass::Precache();
}

void CSandBarnacleTongueTip::Spawn(void)
{
	Precache();
	SetSolid(SOLID_VPHYSICS);
	AddSolidFlags(FSOLID_NOT_SOLID);
	BaseClass::Spawn();
	SetThink(&CSandBarnacleTongueTip::UpdateTongue);
	SetNextThink(gpGlobals->curtime + 0.01);

	SetModel("models/props_junk/rock001a.mdl");
	AddEffects( EF_NODRAW );
}

void CSandBarnacleTongueTip::UpdateTongue(void)
{
	Vector vecOrigin;
	QAngle vecAngles;

	if (GetOwnerBarnacle())
	{
		CStudioHdr* pStudioHdr = GetOwnerBarnacle()->GetModelPtr();
		int boneIndex = Studio_BoneIndexByName(pStudioHdr, "tongue.01");
		GetOwnerBarnacle()->GetBonePosition(boneIndex, vecOrigin, vecAngles);
		SetAbsOrigin(vecOrigin);
		SetAbsAngles(vecAngles);
	}

	SetNextThink(gpGlobals->curtime + 0.01);
}

CSandBarnacleTongueTip *CSandBarnacleTongueTip::CreateTongue( class CNPC_SandBarnacle *pBarnacle, Vector vecOrigin, QAngle vecAngles)
{
	CSandBarnacleTongueTip *pTip = (CSandBarnacleTongueTip *)CBaseEntity::Create( "npc_sandbarnacle_tongue_tip", vecOrigin, vecAngles );
	if ( !pTip )
		return NULL;

	pTip->SetOwnerEntity(pBarnacle);

	//pTip->VPhysicsInitNormal(pTip->GetSolid(), pTip->GetSolidFlags(), false);
	//IPhysicsObject* pTipPhys = pTip->VPhysicsGetObject();
	//pTipPhys->SetCallbackFlags(pTipPhys->GetCallbackFlags() & (~CALLBACK_DO_FLUID_SIMULATION));

	pTip->AddSolidFlags(FSOLID_NOT_SOLID);
	pTip->VPhysicsInitShadow(false, false);
	pTip->SetMoveType(MOVETYPE_NONE);

	pTip->m_hBarnacle = pBarnacle;

	return pTip;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_SandBarnacle::Precache(void)
{
	PrecacheModel("models/barnacle_sand.mdl");

	// Sounds
	PrecacheScriptSound( "NPC_Sandbarnacle.Idle" );
	PrecacheScriptSound( "outland_10a.DvS_StrdrSkew_RockDirt02" );
	PrecacheScriptSound( "NPC_Sandbarnacle.IdleAngry" );
	PrecacheScriptSound( "outland_10a.DvS_StrdrSkew_RockDirt03" );
	PrecacheScriptSound( "NPC_SandBarnacle_Pain" );
	PrecacheScriptSound( "NPC_Sandbarnacle.MeleeAttack" );
	PrecacheScriptSound( "NPC_Sandbarnacle.Death" );

	PrecacheScriptSound("NPC_Barnacle.FinalBite");
	PrecacheScriptSound("NPC_Barnacle.Digest");
	PrecacheScriptSound("Breakable.Flesh");

	// Particles
	PrecacheParticleSystem( "npc_sandbarnacle_burrow" );
	PrecacheParticleSystem( "npc_sandbarnacle_death" );
	PrecacheParticleSystem( "npc_sandbarnacle_emerge" );
	PrecacheParticleSystem( "npc_sandbarnacle_breath" );
	
	// Kill Particles
	PrecacheParticleSystem( SANDBARNACLE_CONSUME_BLOODFX_RED );
	PrecacheParticleSystem( SANDBARNACLE_CONSUME_BLOODFX_GREEN );
	PrecacheParticleSystem( SANDBARNACLE_CONSUME_BLOODFX_YELLOW );
	PrecacheParticleSystem( SANDBARNACLE_CONSUME_BLOODFX_ANTLION );
	PrecacheParticleSystem( SANDBARNACLE_CONSUME_BLOODFX_ZOMBIE );
	PrecacheParticleSystem( SANDBARNACLE_CONSUME_BLOODFX_ANTLION_WORKER );

	BaseClass::Precache();
}

//=========================================================
// Translate Schedule
//=========================================================
int CNPC_SandBarnacle::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_MELEE_ATTACK1:
		return SCHED_SB_MELEE_ATTACK1;

	case SCHED_MELEE_ATTACK2:
		return SCHED_SB_MELEE_ATTACK2;

	case SCHED_SB_BURROW:
	case SCHED_SB_PULLDOWN:
		return ((m_hAttached == NULL) ? SCHED_SB_BURROW : SCHED_SB_PULLDOWN);
	}

	return BaseClass::TranslateSchedule(scheduleType);
}

void CNPC_SandBarnacle::OnChangeActivity(Activity eNewActivity)
{
	// remove corpse when relevent
	switch (eNewActivity)
	{
		case ACT_IDLE_RELAXED:
		{
			RemoveCorpse();
		}
	}

	switch (eNewActivity)
	{
		case ACT_BARNACLE_PULL:
			m_flFinishedDigestionTime = gpGlobals->curtime + RandomFloat( 15.0f, 25.0f );
			// fallthrough here is intentional
		case ACT_CLIMB_DOWN:
		{
			m_bWantsToBeUnderground = true;
			break;
		}
		case ACT_MELEE_ATTACK2:
		case ACT_CLIMB_UP:
		{
			m_bWantsToBeUnderground = false;
			SetSmallHullSize(m_bWantsToBeUnderground);
			break;
		}
	}

	// handle breath particles
	switch (eNewActivity)
	{
		case ACT_MELEE_ATTACK2:
		case ACT_CLIMB_UP:
		case ACT_CLIMB_DOWN:
		case ACT_DIESIMPLE:
		{
			if (m_hBreathParticle)
			{
				UTIL_Remove(m_hBreathParticle);
				m_hBreathParticle = NULL;
			}
			break;
		}
		case ACT_IDLE_RELAXED:	// idle burrowed
		{
			if (!m_hBreathParticle)
			{
				m_hBreathParticle = (CParticleSystem *) CreateEntityByName( "info_particle_system" );

				m_hBreathParticle->KeyValue("start_active", "1");
				m_hBreathParticle->KeyValue("effect_name", "npc_sandbarnacle_breath");
				m_hBreathParticle->SetParent(this);
				m_hBreathParticle->SetLocalOrigin(vec3_origin);
				DispatchSpawn(m_hBreathParticle);
				m_hBreathParticle->Activate();
			}
		}
	}

	float flAnimSpeed = 1.0f;
	bool bTargetInVehicle = false;
	if (GetEnemy())
	{
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter(GetEnemy());
		if (pBCC && pBCC->IsInAVehicle())
			bTargetInVehicle = true;
	}

	// handle anim speed
	switch (eNewActivity)
	{
		case ACT_BARNACLE_PULL:
		{
			// if eating an NPC, grab and pull speed will not be affected by difficulty
			flAnimSpeed = (IS_HARD && GetEnemy()->IsPlayer()) ? 1.37f : 1.0f;
			break;
		}
		case ACT_MELEE_ATTACK2:
		{
			if (bTargetInVehicle)
				flAnimSpeed = 1.35;
			else
				flAnimSpeed = (IS_HARD && GetEnemy()->IsPlayer()) ? 0.94 : 0.72;

			break;
		}
		case ACT_IDLE_RELAXED:	// idle burrowed
			SetSmallHullSize(m_bWantsToBeUnderground);
			// fallthrough here is intentional
		case ACT_IDLE:			// idle unburrowed
			flAnimSpeed = random->RandomFloat(0.8f, 1.2f);
			break;
	}
	BaseClass::OnChangeActivity(eNewActivity);
	SetPlaybackRate(flAnimSpeed);
}

Activity CNPC_SandBarnacle::TranslateActivity(Activity idealActivity, Activity* pIdealWeaponActivity) 
{
	return BaseClass::TranslateActivity(idealActivity, pIdealWeaponActivity);;
}

//=========================================================
// Task Handling
//=========================================================
void CNPC_SandBarnacle::StartTask( const Task_t* pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_SB_EMIT_PARTICLES:
		{
			switch ( (int)pTask->flTaskData )
			{
			default:  // Emerge particles
			case 0:
				EmitSound("outland_10a.DvS_StrdrSkew_RockDirt02"); // this should be done with animevents, but for now this works
				DispatchParticleEffect( "npc_sandbarnacle_emerge", GetAbsOrigin(), GetAbsAngles(), this );
				TaskComplete();
				break;

			case 1:	 // Burrow particles
				StopSound("NPC_Sandbarnacle.IdleAngry");
				EmitSound("outland_10a.DvS_StrdrSkew_RockDirt03"); // this should be done with animevents, but for now this works
				DispatchParticleEffect( "npc_sandbarnacle_burrow", GetAbsOrigin(), GetAbsAngles(), this );
				TaskComplete();
				break;
			
			case 2:	// Death particles
				DispatchParticleEffect( "npc_sandbarnacle_death", GetAbsOrigin(), GetAbsAngles(), this );
				TaskComplete();
				break;
			
			case 3:	// Breath particles
				TaskComplete();
				break;
			}
		}
		break;

		default:
			BaseClass::StartTask( pTask );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_SandBarnacle::Spawn(void)
{
	Precache();

	SetModel("models/barnacle_sand.mdl");
	SetHullType(HULL_SMALL);
	SetHullSizeNormal();

	SetEffects( EF_NOSHADOW );
	SetMoveType(MOVETYPE_NONE);
	SetAbsOrigin(FindBestSpawnPosition());
	SetAbsAngles(QAngle(0.0, 0.0, 0.0));

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetCollisionGroup( RTBRCOLLISION_GROUP_SANDBARNACLE );
	SetBloodColor(BLOOD_COLOR_GREEN);
	m_iHealth = sk_sandbarnacle_health.GetInt();
	m_flFieldOfView = -1; // indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState = NPC_STATE_ALERT;

	m_bWantsToBeUnderground = true;
	SetSmallHullSize(m_bWantsToBeUnderground);
	m_bShouldPlayDigestionSound = false;

	CanBecomeRagdoll();

	CapabilitiesClear();
	CapabilitiesAdd(bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 | bits_CAP_TURN_HEAD);
	CapabilitiesRemove(bits_CAP_MOVE_GROUND);

	NPCInit();

	Vector vecOrigin;
	QAngle vecAngles;
	GetAttachment(LookupAttachment("playerpull"), vecOrigin, vecAngles);
	m_hTongueTip = CSandBarnacleTongueTip::CreateTongue(this, vecOrigin, vecAngles);

	m_vAttachedLastPosition = vec3_invalid;

	// Setup sound times
	m_flNextIdleSoundTime = gpGlobals->curtime;
	m_flNextDigestSoundTime = gpGlobals->curtime;
	m_flNextPainSoundTime = gpGlobals->curtime;
	m_flFinishedDigestionTime = 0.0;
}

// try to find best position to place myself after spawning
Vector CNPC_SandBarnacle::FindBestSpawnPosition(void)
{
	Vector forward, right, up;
	AngleVectors(GetAbsAngles(), &forward, &right, &up);

	float dist = 16.0f;
	float maxHeight = 4096.0f;

	while(dist<maxHeight)
	{
		trace_t tr;
		UTIL_TraceLine(GetAbsOrigin()+(up*dist), GetAbsOrigin()-(up*dist), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_DEBRIS, &tr);
		if (tr.fraction < 1)
			return tr.endpos;

		dist *= 2.0f;
	}

	// cant find anywhere to place, have a winge and turn green
	SetRenderColor(0, 255, 0, 255);
	Error("sandbarnacle '%s' at position x:%.2f y:%.2f z:%.2f could not find valid point to place as the map specified spawn position was too far above the ground for some reason (over %.0f units)\nsandbarnacle will show with a green tint\n", GetDebugName(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, maxHeight);
	return GetAbsOrigin();
}

//=========================================================
// Damage event handler
//=========================================================
int CNPC_SandBarnacle::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newInfo = info;
	if (info.GetDamageType() & DMG_VEHICLE)
		newInfo.ScaleDamage(0.0);

	// regular barnacles also have weakness to clubs
	if (info.GetDamageType() & DMG_CLUB)
		newInfo.ScaleDamage(2.0);
	else {
		if (m_bIsSmallHull)
			newInfo.ScaleDamage(0.75);
	}

	/*
	// TEMP? If we were hit by a vehicle, just absorb the damage and grab the driver.
	CBaseCombatCharacter *pEnemy = ( GetEnemy() ) ? GetEnemy()->MyCombatCharacterPointer() : 0;
	IServerVehicle *pVehicle = NULL;

	if ( pEnemy && pEnemy->IsInAVehicle() )
		pVehicle = pEnemy->GetVehicle();

	if ( pEnemy && pVehicle && !( info.GetDamageType() & DMG_SHOCK ) ) // Don't pull the player out of their vehicle if we were hit by a stray gauss beam
	{
		// Grab the player
		{
			// TODO: Copy-pasta badness
			//CBaseEntity* pHurt = CheckTraceHullAttack(70, -Vector(16, 16, 18), Vector(16, 16, 18), 0, DMG_CLUB);
			if (pEnemy)
			{
				float heightAdj = 64;

				// Grab players out of their vehicle.
				// TODO: Vehicle anticipation?
				if (pEnemy->IsInAVehicle())
				{
					CPropVehicleDriveable *pJeep = dynamic_cast<CPropVehicleDriveable *>( pEnemy->GetVehicleEntity() );

					if ( pJeep && pJeep->IsBoosting() )
					{
						return BaseClass::OnTakeDamage_Alive( info );
					}

					dynamic_cast<CBasePlayer*>(pEnemy)->LeaveVehicle(pEnemy->GetAbsOrigin(), pEnemy->GetAbsAngles());
					SetCollisionGroup( RTBRCOLLISION_GROUP_SANDBARNACLE );
				}

				m_hAttached = pEnemy;
				GetMotor()->SetYawLocked(true);
				pEnemy->SetMoveType(MOVETYPE_NOCLIP);
				//pPlayer->AddEFlags(EFL_NOCLIP_ACTIVE);
				pEnemy->SetEFlags(EFL_IS_BEING_LIFTED_BY_BARNACLE);
				
				// Make the player not solid, so that the vehicle can pass through them without getting stuck.
				// This will get undone when the player is released.
				pEnemy->SetSolidFlags( FSOLID_NOT_SOLID );
				//pEnemy->SetParent(this, LookupAttachment("sb_tongue"));
				Vector attachVec;
				QAngle attachAng;
				GetAttachment(LookupAttachment("playerpull"), attachVec, attachAng);

				CAI_BaseNPC* pNPC = dynamic_cast<CAI_BaseNPC*>(m_hAttached.Get());
				if (pNPC != NULL) {
					heightAdj = pNPC->GetHullHeight();
				}

				attachVec = Vector(attachVec.x, attachVec.y, attachVec.z - (heightAdj / 2));
				pEnemy->SetAbsOrigin(attachVec);
				EmitSound("NPC_Sandbarnacle.MeleeAttack");
			}
		}

		return 0;
	}

	if ( info.GetDamageType() & DMG_VEHICLE )
	{
		// Don't take damage from vehicles.
		return 0;
	}

	if ( gpGlobals->curtime > m_flNextPainSoundTime )
	{
		PainSound();
	}
	*/

	return BaseClass::OnTakeDamage_Alive(newInfo);
}
//=========================================================
// On Death function
//=========================================================
void CNPC_SandBarnacle::Event_Killed(const CTakeDamageInfo& info) {

	m_bShouldPlayDigestionSound = false;
	StopSound("NPC_Sandbarnacle.Idle");
	StopSound("NPC_Sandbarnacle.IdleAngry");
	StopSound("NPC_Barnacle.Digest");
	StopSound("NPC_SandBarnacle_Pain");
	StopSound("NPC_Sandbarnacle.MeleeAttack");

	// this helps avoid the player getting stuck in me
	Vector mins = Vector(-1, -1, 0);
	Vector maxs = Vector(1, 1, 1);
	UTIL_SetSize(this, mins, maxs);

	if (m_hAttached != NULL) 
	{
		if (m_hRagdoll)
		{
			DetachAttachedRagdoll(m_hRagdoll);
			m_hAttached->SetHealth(-98);
			CTakeDamageInfo info = CTakeDamageInfo(this, this, sk_sandbarnacle_dmg.GetFloat(), DMG_CRUSH);
			m_hAttached->TakeDamage(info);
		}
		else {
			m_hAttached->SetMoveType(MOVETYPE_WALK);
		}
		m_hAttached->RemoveEFlags(EFL_IS_BEING_LIFTED_BY_BARNACLE);
		m_hAttached->RemoveSolidFlags( FSOLID_NOT_SOLID );

		Vector attachVec;
		QAngle attachAng;
		float heightAdj = -64;

		CAI_BaseNPC* pNPC = dynamic_cast<CAI_BaseNPC*>(m_hAttached.Get());
		if (pNPC != NULL) {
			heightAdj = pNPC->GetHullHeight() * 2.;
		}

		GetAttachment(LookupAttachment("playerpull"), attachVec, attachAng);
		//GetBonePosition("playerpull", attachVec, attachAng);
		attachVec = Vector(attachVec.x, attachVec.y, attachVec.z + heightAdj);
		Vector vecPreyForward;
		m_hAttached->GetVectors(&vecPreyForward, NULL, NULL);

		// if the player kills me while i am underground, this makes sure they dont get stuck
		if (m_hAttached->IsPlayer())
		{
			Vector mins = Vector(-12, -12, 0);
			Vector maxs = Vector(12, 12, 68);

			trace_t tr;

			Vector preyDepositPosition = attachVec + (-vecPreyForward * 15.0f);
			UTIL_TraceHull(
				preyDepositPosition,
				preyDepositPosition,
				mins,
				maxs,
				MASK_PLAYERSOLID,
				m_hAttached,
				COLLISION_GROUP_PLAYER_MOVEMENT,
				&tr
			);

			if (tr.startsolid || tr.allsolid)
			{
				m_hAttached->SetAbsOrigin(m_vAttachedLastPosition);
			}
			else {
				m_hAttached->SetAbsOrigin(preyDepositPosition);
			}
		}


		m_hAttached->SetAbsVelocity((-vecPreyForward * 175.0f)); // player can sometimes get stuck in me if they kill me while im grabbing them, this dislodges them

		m_hAttached = NULL;
		m_vAttachedLastPosition = vec3_invalid;
		GetMotor()->SetYawLocked(false);
	}
	DeathSound();

	// TODO:
	// For some reason, the sand barnacle refuses to die immediately if it's underground,
	// just explicitly kill it for now, rework this later.
	if ( m_bWantsToBeUnderground && GetHealth() <= 0 )
	{
		//DevMsg( "WE SHOULD BE DEAD!\n" );
		SetState( NPC_STATE_DEAD );
		SetSchedule( SCHED_DIE );
	}
	StopParticleEffects( this );
	DispatchParticleEffect( "npc_sandbarnacle_death", GetAbsOrigin(), GetAbsAngles(), this );

	BaseClass::Event_Killed(info);
}

//=========================================================
// Preschedule Think
//=========================================================
void CNPC_SandBarnacle::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();

	m_flCurrentMaxVehicleGrabDistance = 0.0;
	if (GetEnemy())
	{
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter(GetEnemy());
		if (pBCC && pBCC->IsInAVehicle())
		{
			CPropVehicleDriveable* pVehicle = dynamic_cast<CPropVehicleDriveable*>(pBCC->GetVehicleEntity());
			ASSERT(pVehicle != NULL);
			if (pVehicle)
			{
				Vector vecVehicleForward;
				pVehicle->GetVectors(&vecVehicleForward, NULL, NULL);

				Vector vecVehicleLinear;
				AngularImpulse vecVehicleAngular;
				pVehicle->GetVelocity(&vecVehicleLinear, &vecVehicleAngular);

				float flVehicleSpeed = vecVehicleLinear.Length();

				float flMaxGrabDistance = flVehicleSpeed;
				flMaxGrabDistance /= 500.0; // jeep max speed 
				flMaxGrabDistance = min(flMaxGrabDistance, 1.0);
				flMaxGrabDistance = pow(flMaxGrabDistance, 0.25); // powered by 0.25 so it reaches max grab distance faster
				flMaxGrabDistance *= SANDBARNACLE_MAX_VEHICLE_GRAB_DISTANCE_MAX - SANDBARNACLE_MAX_VEHICLE_GRAB_DISTANCE_MIN;
				flMaxGrabDistance += SANDBARNACLE_MAX_VEHICLE_GRAB_DISTANCE_MIN;

				m_flCurrentMaxVehicleGrabDistance = flMaxGrabDistance;
			}
		}
	}

	// Check idle sounds
	if ( gpGlobals->curtime > m_flNextIdleSoundTime && !m_bShouldPlayDigestionSound )
	{
		IdleSound();
	}
	
	if ( gpGlobals->curtime > m_flNextDigestSoundTime && m_bShouldPlayDigestionSound )
	{
		EmitSound( "NPC_Barnacle.Digest" );
		m_flNextDigestSoundTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 3.5f );
	}

}

//=========================================================
// Gather Conditions
//=========================================================
void CNPC_SandBarnacle::GatherConditions( void )
{
	// This will call GatherEnemyConditions() for us
	BaseClass::GatherConditions();
	
	if (m_hAttached != NULL)
		SetCondition(COND_SB_PREY_ATTACHED);
	else
		ClearCondition(COND_SB_PREY_ATTACHED);

	if ( HasCondition( COND_SEE_PLAYER ) && m_bWantsToBeUnderground )
		SetState( NPC_STATE_COMBAT );

	//if ( GetEnemy() )
	//{
	//	// Check if our enemy is in a vehicle.
	//	CBaseCombatCharacter *pBCC = ToBaseCombatCharacter( GetEnemy() );
	//	if ( pBCC && pBCC->IsInAVehicle() )
	//	{
	//		CBaseEntity *pVehicle = pBCC->GetVehicleEntity();
	//		Vector vecVehicleForward;
	//		pVehicle->GetVectors( &vecVehicleForward, NULL, NULL );

	//		Vector2D vecVehicleVel = pVehicle->GetSmoothedVelocity().AsVector2D();
	//		Vector2D vecVehicleToBarnacle = pVehicle->GetAbsOrigin().AsVector2D() - GetAbsOrigin().AsVector2D();

	//		float flDot = DotProduct2D( vecVehicleForward.AsVector2D(), vecVehicleToBarnacle );

	//		if ( flDot < 0.0f && ( vecVehicleVel.x < 0.0f && vecVehicleVel.y < 0.0f ) && vecVehicleVel.LengthSqr() > Square( 400 ) ) // Set the condition if the vehicle is coming at us (at a decent speed)
	//		{
	//			m_bVehicleIsComing = true;

	//			// Save off the vehicle details for later use.
	//			m_vecLastPosition = pVehicle->GetAbsOrigin();
	//			m_vecLastVehicleVel = Vector( vecVehicleVel.x, vecVehicleVel.y, 0.0f );  // NOTE: For now we're just ignoring the velocity's Z-component.
	//		}
	//	}
	//}
}

//=========================================================
// Select Schedule 
//=========================================================
int CNPC_SandBarnacle::SelectSchedule()
{
	if (m_flFinishedDigestionTime > gpGlobals->curtime)
		return SCHED_SB_DIGESTIDLE;

	m_bShouldPlayDigestionSound = false;
	StopSound("NPC_Barnacle.Digest");

	if (HasCondition(COND_CAN_MELEE_ATTACK1) && m_bWantsToBeUnderground)
		return SCHED_SB_POPUP;

	if (m_bWantsToBeUnderground)
	{
		if (HasCondition(COND_CAN_MELEE_ATTACK2))
			return SCHED_MELEE_ATTACK2;

		return SCHED_SB_UNDERIDLE;
	}
	else
	{
		if (HasCondition(COND_CAN_MELEE_ATTACK1))
			return SCHED_MELEE_ATTACK1;

		if (HasCondition(COND_SB_PREY_WITHIN_RANGE))
			return SCHED_SB_FACE_PREY;

		return SCHED_SB_BURROW;
	}
}

void CNPC_SandBarnacle::NPCThink(void)
{
	BaseClass::NPCThink();

	if (m_hAttached != NULL)
	{
		Vector attachVec;
		QAngle attachAng;
		float heightAdj = -64;

		CAI_BaseNPC* pNPC = dynamic_cast<CAI_BaseNPC*>(m_hAttached.Get());
		if (pNPC != NULL) {
			heightAdj = pNPC->GetHullHeight() * 2.;
		}

		GetAttachment(LookupAttachment("playerpull"), attachVec, attachAng);
		//GetBonePosition("playerpull", attachVec, attachAng);
		attachVec = Vector(attachVec.x, attachVec.y, attachVec.z + heightAdj);
		m_hAttached->SetAbsOrigin(attachVec);
		
		SetNextThink(gpGlobals->curtime + .001);
	}
}

//=========================================================
// MeleeAttack1Conditions
//=========================================================
int CNPC_SandBarnacle::MeleeAttack1Conditions(float flDot, float flDist)
{
	if (!GetEnemy())
		return COND_NONE;

	// always ambush NPCs reguardless of difficulty
	if ((SANDBARNACLE_SHOULD_AMBUSH_BY_DEFAULT || !GetEnemy()->IsPlayer()) && m_bWantsToBeUnderground)
		return COND_NONE;

	bool bTargetInVehicle = false;
	if (GetEnemy())
	{
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter(GetEnemy());
		if (pBCC && pBCC->IsInAVehicle())
			bTargetInVehicle = true;
	}

	if (m_hAttached == NULL && !bTargetInVehicle)
	{
		if (flDist < SANDBARNACLE_MAX_GRAB_DISTANCE)
		{
			// If the player is too high above us, dont try to grab them.
			if ((GetEnemy()->GetAbsOrigin().z - GetAbsOrigin().z) > SANDBARNACLE_MAX_VERTICAL_GRAB_DISTANCE)
				return COND_TOO_FAR_TO_ATTACK;

			SetCondition(COND_SB_PREY_WITHIN_RANGE);
			if (flDot > 0.8)
				return COND_CAN_MELEE_ATTACK1;
			else
				return COND_NOT_FACING_ATTACK;
		}
	}
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// MeleeAttack2Conditions
//=========================================================
int CNPC_SandBarnacle::MeleeAttack2Conditions(float flDot, float flDist)
{
	bool bTargetInVehicle = false;
	if (GetEnemy())
	{
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter(GetEnemy());
		if (pBCC && pBCC->IsInAVehicle())
			bTargetInVehicle = true;
	}

	// always ambush NPCs reguardless of difficulty
	if (m_hAttached == NULL && (bTargetInVehicle || (SANDBARNACLE_SHOULD_AMBUSH_BY_DEFAULT || !GetEnemy()->IsPlayer())))
	{
		float flGrabDistance = (
			bTargetInVehicle
			? m_flCurrentMaxVehicleGrabDistance
			: SANDBARNACLE_MAX_AMBUSH_GRAB_DISTANCE
			);

		// If the player is too high above us, and they are not in the vehicle, dont try to grab them.
		if (!bTargetInVehicle)
			if ((GetEnemy()->GetAbsOrigin().z - GetAbsOrigin().z) > SANDBARNACLE_MAX_VERTICAL_GRAB_DISTANCE)
				return COND_TOO_FAR_TO_ATTACK;

		if (flDist < flGrabDistance)
		{
			SetCondition(COND_SB_PREY_WITHIN_RANGE);
			if (flDot > 0.8)
			{
				m_bIsAmbushing = true;
				return COND_CAN_MELEE_ATTACK2;
			}
			else
				return COND_NOT_FACING_ATTACK;
		}
	}
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// HandleAnimEvent 
//=========================================================
void CNPC_SandBarnacle::HandleAnimEvent(animevent_t* pEvent)
{
	if (pEvent->type & AE_TYPE_NEWEVENTSYSTEM)
	{
		if (pEvent->event == AE_SB_GRAB || pEvent->event == AE_SB_AMBUSH_GRAB) {

			bool bTargetInVehicle = false;
			if (GetEnemy())
			{
				CBaseCombatCharacter* pBCC = ToBaseCombatCharacter(GetEnemy());
				if (pBCC && pBCC->IsInAVehicle())
					bTargetInVehicle = true;
			}

			CBaseEntity* pHurt;

			// this should changed based on the specific animevent, but as of yet AE_SB_AMBUSH_GRAB hasnt been implemented anywhere
			bool bIsAmbushing = m_bIsAmbushing;
			m_bIsAmbushing = false;
			
			float flGrabDistance =
				(bIsAmbushing)
				? (
					bTargetInVehicle
					? m_flCurrentMaxVehicleGrabDistance
					: SANDBARNACLE_MAX_AMBUSH_GRAB_DISTANCE
					)
				: SANDBARNACLE_MAX_GRAB_DISTANCE;


			pHurt = CheckTraceHullAttack(flGrabDistance, -Vector(16, 16, 32), Vector(16, 16, 32), 0, DMG_CLUB);

			if (pHurt)
			{
				// dont grab enemies which are already being grabbed by a barnacle
				if (pHurt->GetEFlags() & EFL_IS_BEING_LIFTED_BY_BARNACLE)
					return;

				if (CPropVehicleDriveable* pVehicle = dynamic_cast<CPropVehicleDriveable*>(pHurt))
					if (pVehicle->GetDriver())
						pHurt = pVehicle->GetDriver(); // if we hit a vehicle, get the driver instead
			}

			CBaseCombatCharacter* pBCC = ToBaseCombatCharacter(pHurt);

			if (pBCC)
			{
				float heightAdj = 64;

				// Grab players out of their vehicle.
				// TODO: Vehicle anticipation?
				if ( pBCC->IsInAVehicle() )
				{
					Vector vecVelocity;

					CBaseEntity *pVehicle = pBCC->GetVehicleEntity();
					pVehicle->GetVelocity( &vecVelocity );
					DevMsg( "%f, %f, %f\n", vecVelocity.x, vecVelocity.y, vecVelocity.z );
					dynamic_cast<CBasePlayer *>( pBCC )->LeaveVehicle( pBCC->GetAbsOrigin(), pBCC->GetAbsAngles() );
				}

				m_vAttachedLastPosition = pBCC->GetAbsOrigin();
				m_hAttached = pBCC;

				GetMotor()->SetYawLocked(true);
				pBCC->SetMoveType(MOVETYPE_NOCLIP);
				//pPlayer->AddEFlags(EFL_NOCLIP_ACTIVE);
				pBCC->SetEFlags(EFL_IS_BEING_LIFTED_BY_BARNACLE);
				//pBCC->SetParent(this, LookupAttachment("sb_tongue"));
				Vector attachVec;
				QAngle attachAng;
				GetAttachment(LookupAttachment("playerpull"), attachVec, attachAng);

				attachVec = Vector(attachVec.x, attachVec.y, attachVec.z - (heightAdj / 2));
				pBCC->SetAbsOrigin(attachVec);
				EmitSound( "NPC_Sandbarnacle.MeleeAttack" );

				CAI_BaseNPC* pNPC = dynamic_cast<CAI_BaseNPC*>(m_hAttached.Get());
				if (pNPC != NULL) {
					heightAdj = pNPC->GetHullHeight();
					GetAttachment( "npcpull", attachVec, attachAng );

					CBaseAnimating* pAnimating = dynamic_cast<CBaseAnimating*>(pNPC);
					pAnimating->InvalidateBoneCache();

					// Make a ragdoll for the guy, and hide him.
					pNPC->AddSolidFlags(FSOLID_NOT_SOLID);

					m_hRagdoll = AttachRagdollToTongue(pAnimating);
					m_hRagdoll->SetDamageEntity(pAnimating);

					Vector vecVelocity = Vector(0.0, 0.0, 0.0);
					ragdoll_t* pRagdoll = m_hRagdoll->GetRagdoll();

					for (int i = 0; i < pRagdoll->listCount; i++)
					{
						pRagdoll->list[i].pObject->SetVelocity(&vecVelocity, NULL);
					}

					m_hTongueTip->UpdateTongue();

					// Now hide the actual enemy
					pNPC->AddEffects(EF_NODRAW);

					pNPC->SetNextThink(NULL);
				}

				return;
			}

			// We couldn't get an enemy to grab, just boot out.
			return;
		}

		if (pEvent->event == AE_SB_PULLDOWN) {
			return;
		}

		if (pEvent->event == AE_SB_KILL) {
			if (m_hAttached != NULL) {

				// need to get this before we remove the ragdoll
				CAI_BaseNPC* pNPC = dynamic_cast<CAI_BaseNPC*>(m_hAttached.Get());

				if (pNPC)
				{
					char* BloodParticle = SANDBARNACLE_CONSUME_BLOODFX_RED;
					int PreyBloodColor = pNPC->BloodColor();

					switch (PreyBloodColor)
					{
					case BLOOD_COLOR_RED:
						BloodParticle = SANDBARNACLE_CONSUME_BLOODFX_RED;
						break;
					case BLOOD_COLOR_GREEN:
						BloodParticle = SANDBARNACLE_CONSUME_BLOODFX_GREEN;
						break;
					case BLOOD_COLOR_YELLOW:
						BloodParticle = SANDBARNACLE_CONSUME_BLOODFX_YELLOW;
						break;
					case BLOOD_COLOR_ANTLION:
						BloodParticle = SANDBARNACLE_CONSUME_BLOODFX_ANTLION;
						break;
					case BLOOD_COLOR_ZOMBIE:
						BloodParticle = SANDBARNACLE_CONSUME_BLOODFX_ZOMBIE;
						break;
					case BLOOD_COLOR_ANTLION_WORKER:
						BloodParticle = SANDBARNACLE_CONSUME_BLOODFX_ANTLION_WORKER;
						break;
					}

					DispatchParticleEffect(BloodParticle, GetAbsOrigin(), GetAbsAngles(), this);
					UTIL_BloodSpray(GetAbsOrigin(), Vector(0.0, 0.0, 1.0), PreyBloodColor, 4, FX_BLOODSPRAY_ALL);
				}

				m_bShouldPlayDigestionSound = true;

				m_hAttached->SetHealth(-98);
				CTakeDamageInfo info = CTakeDamageInfo(this, this, sk_sandbarnacle_dmg.GetFloat() , DMG_CRUSH);
				m_hAttached->TakeDamage(info);

				CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>(m_hAttached.Get());
				if (pPlayer != NULL) {
					UTIL_ScreenFade(pPlayer, { 0, 0, 0, 255 }, 0.1, 0, FFADE_OUT | FFADE_STAYOUT);
				}
			}

			RemoveCorpse();

			EmitSound("NPC_Barnacle.FinalBite");
			// Set this immediately so we don't pop back up.
			m_bWantsToBeUnderground = true;
			SetSmallHullSize(m_bWantsToBeUnderground);
			m_hAttached = NULL;
			m_vAttachedLastPosition = vec3_invalid;
			GetMotor()->SetYawLocked(false);
			return;
		}

		else {
			BaseClass::HandleAnimEvent(pEvent);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Attach a serverside ragdoll prop for the specified entity to our tongue
//-----------------------------------------------------------------------------
CRagdollProp* CNPC_SandBarnacle::AttachRagdollToTongue(CBaseAnimating* pAnimating)
{
	// Find his head bone
	int iGrabbedBoneIndex = -1;

	CStudioHdr* pHdr = pAnimating->GetModelPtr();
	if (pHdr)
	{
		int set = pAnimating->GetHitboxSet();
		for (int i = 0; i < pHdr->iHitboxCount(set); i++)
		{
			mstudiobbox_t* pBox = pHdr->pHitbox(i, set);
			if (!pBox)
				continue;

			if (pBox->group == HITGROUP_HEAD)
			{
				iGrabbedBoneIndex = pBox->bone;
				break;
			}
		}
	}

	if (iGrabbedBoneIndex == -1)
	{
		// Just use the first bone
		iGrabbedBoneIndex = 0;
	}

	// Move the tip to the bone
	Vector vecBonePos;
	QAngle vecBoneAngles;
	pAnimating->GetBonePosition(iGrabbedBoneIndex, vecBonePos, vecBoneAngles);

	if (m_hTongueTip)
	{
		m_hTongueTip->Teleport(&vecBonePos, NULL, NULL);
	}

	//NDebugOverlay::Box( vecBonePos, -Vector(5,5,5), Vector(5,5,5), 255,255,255, 0, 10.0 );

	Vector attachVec;
	QAngle attachAng;

	CStudioHdr* pStudioHdr = GetModelPtr();
	int boneIndex = Studio_BoneIndexByName(pStudioHdr, "tongue.01");
	GetBonePosition(boneIndex, attachVec, attachAng);

	// Create the ragdoll attached to tongue
	IPhysicsObject* pTonguePhysObject = m_hTongueTip->VPhysicsGetObject();
	CRagdollProp* pRagdoll = CreateServerRagdollAttached(pAnimating, vec3_origin, -1, COLLISION_GROUP_NONE, pTonguePhysObject, m_hTongueTip, 0, vecBonePos, iGrabbedBoneIndex, vec3_origin);
	if (pRagdoll)
	{
		//pRagdoll->SetAbsOrigin(attachVec);
		//pRagdoll->SetAbsAngles(attachAng);

		PhysEnableEntityCollisions(this, pAnimating);
		PhysDisableEntityCollisions(this, pRagdoll);

		// shadows can behave funny when the caster is underground
		pRagdoll->AddEffects(EF_NOSHADOW);

		pRagdoll->DisableAutoFade();
		pRagdoll->SetThink(NULL);
	}

	return pRagdoll;
}

//=========================================================
// Idle Sound 
//=========================================================
void CNPC_SandBarnacle::IdleSound( void )
{
	if (m_bWantsToBeUnderground)
	{
		EmitSound( "NPC_Sandbarnacle.Idle" );
		m_flNextIdleSoundTime = gpGlobals->curtime + random->RandomFloat( 7.5f, 8.5f );
	}
	else
	{
		EmitSound( "NPC_Sandbarnacle.IdleAngry" );
		m_flNextIdleSoundTime = gpGlobals->curtime + random->RandomFloat( 2.5f, 4.0f );
	}
}

//=========================================================
// Pain Sound 
//=========================================================
void CNPC_SandBarnacle::PainSound( void )
{
	EmitSound( "NPC_SandBarnacle.Pain" );
	m_flNextPainSoundTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 1.0f );
}

void CNPC_SandBarnacle::SetSmallHullSize( bool bSmall)
{
	Vector mins = ( bSmall ? Vector(-4, -4, 0) : Vector(-18, -18, 0) );
	Vector maxs = ( bSmall ? Vector(4, 4, 26) : Vector(18, 18, 100) );

	m_bIsSmallHull = bSmall;
	UTIL_SetSize(this, mins, maxs);
}

void CNPC_SandBarnacle::RemoveCorpse()
{
	CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>(m_hAttached.Get());
	if (!pPlayer)
	{
		if (m_hRagdoll != NULL) {
			DetachAttachedRagdoll(m_hRagdoll);
			UTIL_Remove(m_hRagdoll);
		}
		if (m_hAttached.Get() != NULL) {
			UTIL_Remove(m_hAttached.Get());
		}
	}
}

//=========================================================
// Death Sound 
//=========================================================
void CNPC_SandBarnacle::DeathSound( void )
{
	EmitSound( "NPC_Sandbarnacle.Death" );
	EmitSound( "Breakable.Flesh" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_SandBarnacle::Classify(void)
{
	return CLASS_SAND_BARNACLE;
}

float CNPC_SandBarnacle::MaxYawSpeed()
{
	if (GetActivity() == ACT_MELEE_ATTACK1 || GetActivity() == ACT_MELEE_ATTACK2)
		return 12; // turn slower while grabbing

	return m_bWantsToBeUnderground ? 32: 16;
}

AI_BEGIN_CUSTOM_NPC(npc_sandbarnacle, CNPC_SandBarnacle)

DECLARE_ANIMEVENT(AE_SB_GRAB)
DECLARE_ANIMEVENT(AE_SB_AMBUSH_GRAB)
DECLARE_ANIMEVENT(AE_SB_KILL)
DECLARE_ANIMEVENT(AE_SB_PULLDOWN)

DECLARE_CONDITION(COND_SB_PREY_WITHIN_RANGE)
DECLARE_CONDITION(COND_SB_PREY_ATTACHED)

DECLARE_TASK( TASK_SB_EMIT_PARTICLES )

DEFINE_SCHEDULE
(
	SCHED_SB_IDLE,
	"	Tasks"
	"		TASK_STOP_MOVING		1"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
	"		TASK_WAIT				5"
	"		TASK_WAIT_PVS			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SB_PREY_WITHIN_RANGE"
)

DEFINE_SCHEDULE
(
	SCHED_SB_UNDERIDLE,

	"	Tasks"
	"		TASK_STOP_MOVING		1"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE_RELAXED"
	"		TASK_WAIT_FACE_ENEMY	5"
	"		TASK_WAIT_PVS			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SB_PREY_WITHIN_RANGE"
)

DEFINE_SCHEDULE
(
	SCHED_SB_DIGESTIDLE,

	"	Tasks"
	"		TASK_STOP_MOVING		1"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE_STEALTH"
	"		TASK_WAIT				1"
	""
)

DEFINE_SCHEDULE
(
	SCHED_SB_FACE_PREY,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	""
	"	Interrupts"
	"		COND_SB_PREY_ATTACHED"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
);

DEFINE_SCHEDULE
(
	SCHED_SB_POPUP,
	"	Tasks"
	//"		TASK_FACE_ENEMY			0"
	"		TASK_STOP_MOVING		1"
	"		TASK_SB_EMIT_PARTICLES	0"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_CLIMB_UP"
	"		TASK_WAIT				1"
	"		TASK_WAIT_PVS			0"
	//"       TASK_SET_SCHEDULE       SCHEDULE:SCHED_IDLE_STAND"
	""
	"	Interrupts"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_OCCLUDED"
)

DEFINE_SCHEDULE
(
	SCHED_SB_BURROW,
	"	Tasks"
	"		TASK_STOP_MOVING		1"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_CLIMB_DOWN"
	"		TASK_WAIT				1"
	"		TASK_SB_EMIT_PARTICLES	1"
	"		TASK_WAIT				0.5"
	"       TASK_SET_SCHEDULE       SCHEDULE:SCHED_SB_UNDERIDLE"
	""
	"	Interrupts"
)

DEFINE_SCHEDULE
(
	SCHED_SB_PULLDOWN,
	"	Tasks"
	"		TASK_STOP_MOVING		1"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_BARNACLE_PULL"
	"		TASK_WAIT				2.85"
	"		TASK_SB_EMIT_PARTICLES	1"
	"		TASK_WAIT				1"
	"       TASK_SET_SCHEDULE       SCHEDULE:SCHED_SB_DIGESTIDLE"
	""
	"	Interrupts"
)

DEFINE_SCHEDULE
(
	SCHED_SB_MELEE_ATTACK1,
	"	Tasks"
	"		TASK_STOP_MOVING		0"
	//"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = grab attack
	"		TASK_MELEE_ATTACK1		0"
	""
)

DEFINE_SCHEDULE
(
	SCHED_SB_MELEE_ATTACK2,
	"	Tasks"
	"		TASK_SB_EMIT_PARTICLES	0"
	"		TASK_ANNOUNCE_ATTACK	2"	// 2 = ambush attack
	"		TASK_MELEE_ATTACK2		0"
	""
)


AI_END_CUSTOM_NPC()