//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon_shared.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "IEffects.h"
#include "te_effect_dispatch.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "beam_shared.h"
#include "rumble_shared.h"
#include "gamestats.h"
#include "decals.h"
#include "props.h"

#ifdef PORTAL
#include "portal_util_shared.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define BOLT_MODEL			"models/crossbow_bolt.mdl"
#define BOLT_MODEL	"models/weapons/w_missile_closed.mdl"

#define BOLT_AIR_VELOCITY	2500
#define BOLT_WATER_VELOCITY	1500
#define BOLT_ELEC_VEL_SCALE 0.70f

extern ConVar sk_plr_dmg_crossbow;
extern ConVar sk_npc_dmg_crossbow;
ConVar sk_crossbow_elec_radius( "sk_crossbow_elec_radius", "0" );
ConVar sk_crossbow_elec_direct_dmg_scale( "sk_crossbow_elec_direct_dmg_scale", "0" );	// scale alt-fire direct impacts by this amount
ConVar sk_crossbow_elec_radius_dmg_scale( "sk_crossbow_elec_radius_dmg_scale", "0" );	// fraction of m_flDamage that the aoe attack does as a base
ConVar sk_crossbow_elec_radius_duration( "sk_crossbow_elec_radius_duration", "0" );		// how long the electric attack lingers
ConVar sk_crossbow_elec_radius_tick( "sk_crossbow_elec_radius_tick", "0" );				// how often, in seconds, to perform damage. note that there is a pulse at impact so the
																						// actual number of pulses is duration / tick + 1. for the default values this means 6 ticks

#ifdef MAPBASE
ConVar weapon_crossbow_new_hit_locations( "weapon_crossbow_new_hit_locations", "1", FCVAR_NONE, "Toggles new crossbow knockback that properly pushes back the correct limbs." );
#endif

void TE_StickyBolt( IRecipientFilter& filter, float delay, Vector vecDirection, const Vector *origin );

#define	BOLT_SKIN_NORMAL	0
#define BOLT_SKIN_GLOW		1

//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------
class CCrossbowBolt : public CBaseCombatCharacter
{
	DECLARE_CLASS( CCrossbowBolt, CBaseCombatCharacter );

public:
#ifdef MAPBASE
	CCrossbowBolt();
#else
	CCrossbowBolt() { };
#endif

	Class_T Classify( void ) { return CLASS_NONE; }

public:
	void Spawn( void );
	void Precache( void );
	void BubbleThink( void );
	void ElectricPulseThink( void );
	void BoltTouch( CBaseEntity *pOther );
	bool CreateVPhysics( void );
	unsigned int PhysicsSolidMaskForEntity() const;
#ifdef MAPBASE
	static CCrossbowBolt *BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, CBaseCombatCharacter *pentOwner = NULL, int iElec = -1 );

	void InputSetDamage( inputdata_t &inputdata );
	float m_flDamage;

	virtual void SetDamage( float flDamage ) { m_flDamage = flDamage; }
#else
	static CCrossbowBolt *BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, CBasePlayer *pentOwner = NULL );
#endif
	void SetElectric( int iElec ) { m_iElectric = iElec; }
	CNetworkVar( int, m_iElectric );
	float m_flElecStart;
	CNetworkVar( bool, m_bStruckSomething );
	bool m_bMovedDown;

	CBaseEntity *m_hAttachedEntity;
	Vector		m_vAttachedEntityOrigin;

protected:
	//CHandle<CSpriteTrail>	m_pGlowTrail;

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};
LINK_ENTITY_TO_CLASS( crossbow_bolt, CCrossbowBolt );

BEGIN_DATADESC( CCrossbowBolt )
// Function Pointers
DEFINE_FUNCTION( BubbleThink ),
DEFINE_FUNCTION(ElectricPulseThink),
DEFINE_FUNCTION( BoltTouch ),

// These are recreated on reload, they don't need storage
//DEFINE_FIELD( m_pGlowTrail, FIELD_EHANDLE ),
DEFINE_FIELD( m_iElectric, FIELD_INTEGER ),
DEFINE_FIELD( m_flElecStart, FIELD_FLOAT ),
DEFINE_FIELD( m_hAttachedEntity, FIELD_CLASSPTR ),
DEFINE_FIELD( m_vAttachedEntityOrigin, FIELD_VECTOR),
DEFINE_FIELD(m_bStruckSomething, FIELD_BOOLEAN),

#ifdef MAPBASE
DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "Damage" ),

// Inputs
DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDamage", InputSetDamage ),
#endif

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CCrossbowBolt, DT_CrossbowBolt )
	SendPropInt(SENDINFO(m_iElectric)),
	SendPropBool(SENDINFO(m_bStruckSomething)),
END_SEND_TABLE()

#ifdef MAPBASE
CCrossbowBolt *CCrossbowBolt::BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, CBaseCombatCharacter *pentOwner, int iElec )
#else
CCrossbowBolt *CCrossbowBolt::BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, CBasePlayer *pentOwner )
#endif
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = (CCrossbowBolt *)CreateEntityByName( "crossbow_bolt" );
	UTIL_SetOrigin( pBolt, vecOrigin );
	pBolt->SetElectric( iElec );
	pBolt->SetAbsAngles( angAngles );
	pBolt->Spawn();
	pBolt->SetOwnerEntity( pentOwner );
#ifdef MAPBASE
	if ( pentOwner && pentOwner->IsNPC() )
		pBolt->m_flDamage = sk_npc_dmg_crossbow.GetFloat();
	//else
	//	pBolt->m_flDamage = sk_plr_dmg_crossbow.GetFloat();
#endif

	return pBolt;
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCrossbowBolt::CCrossbowBolt( void )
{
	// Independent bolts without m_flDamage set need damage
	m_flDamage = sk_plr_dmg_crossbow.GetFloat();
	m_hAttachedEntity = NULL;
	m_vAttachedEntityOrigin = vec3_invalid;
	m_bStruckSomething = false;
	m_bMovedDown = false;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CCrossbowBolt::CreateVPhysics( void )
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CCrossbowBolt::PhysicsSolidMaskForEntity() const
{
	return ( BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX ) & ~CONTENTS_GRATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrossbowBolt::Spawn( void )
{
	Precache();

	SetModel( "models/crossbow_bolt.mdl" );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	UTIL_SetSize( this, -Vector( 0.3f, 0.3f, 0.3f ), Vector( 0.3f, 0.3f, 0.3f ) );
	SetSolid( SOLID_BBOX );
	if ( m_iElectric != -1 )
		SetGravity( 0.50f );
	else
		SetGravity( 0.05f );



	// Make sure we're updated if we're underwater
	UpdateWaterState();

	SetTouch( &CCrossbowBolt::BoltTouch );

	SetThink( &CCrossbowBolt::BubbleThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	// Make us glow until we've hit the wall
	m_nSkin = BOLT_SKIN_GLOW;
}


void CCrossbowBolt::Precache( void )
{
	PrecacheModel( BOLT_MODEL );

	// This is used by C_TEStickyBolt, despte being different from above!!!
	PrecacheModel( "models/crossbow_bolt.mdl" );

	PrecacheModel( "sprites/light_glow02_noz.vmt" );

	PrecacheParticleSystem( "weapon_steambow_bolt_trail" );
	PrecacheParticleSystem( "weapon_steambow_bolt_impact1" );
	PrecacheParticleSystem( "weapon_steambow_bolt_impact2" );
	PrecacheParticleSystem( "weapon_steambow_bolt_impact3" );
	PrecacheParticleSystem( "weapon_steambow_bolt_aoe1" );
	PrecacheParticleSystem( "weapon_steambow_bolt_aoe2" );
	PrecacheParticleSystem( "weapon_steambow_bolt_aoe3" );

	PrecacheScriptSound( "Crossbow_Bolt.ElectricPulse" );
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrossbowBolt::InputSetDamage( inputdata_t &inputdata )
{
	m_flDamage = inputdata.value.Float();
}
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS | FSOLID_TRIGGER ) )
	{
		// Some NPCs are triggers that can take damage (like antlion grubs). We should hit them.
#ifdef MAPBASE
		// But some physics objects that are also triggers (like weapons) shouldn't go through this check.
		// 
		// Note: rpg_missile has the same code, except it properly accounts for weapons in a different way.
		// This was discovered after I implemented this and both work fine, but if this ever causes problems,
		// use rpg_missile's implementation:
		// 
		// if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER|FSOLID_VOLUME_CONTENTS) && pOther->GetCollisionGroup() != COLLISION_GROUP_WEAPON )
		// 
		if ( pOther->GetMoveType() == MOVETYPE_NONE && ( ( pOther->m_takedamage == DAMAGE_NO ) || ( pOther->m_takedamage == DAMAGE_EVENTS_ONLY ) ) )
#else
		if ( ( pOther->m_takedamage == DAMAGE_NO ) || ( pOther->m_takedamage == DAMAGE_EVENTS_ONLY ) )
#endif
			return;
	}

	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		trace_t	tr, tr2;
		tr = BaseClass::GetTouchTrace();
		Vector	vecNormalizedVel = GetAbsVelocity();

		ClearMultiDamage();
		VectorNormalize( vecNormalizedVel );

#if defined(HL2_EPISODIC)
		//!!!HACKHACK - specific hack for ep2_outland_10 to allow crossbow bolts to pass through her bounding box when she's crouched in front of the player
		// (the player thinks they have clear line of sight because Alyx is crouching, but her BBOx is still full-height and blocks crossbow bolts.
		if ( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() && pOther->Classify() == CLASS_PLAYER_ALLY_VITAL && FStrEq( STRING( gpGlobals->mapname ), "ep2_outland_10" ) )
		{
			// Change the owner to stop further collisions with Alyx. We do this by making her the owner.
			// The player won't get credit for this kill but at least the bolt won't magically disappear!
			SetOwnerEntity( pOther );
			return;
		}
#endif//HL2_EPISODIC

#ifdef MAPBASE
		if ( weapon_crossbow_new_hit_locations.GetInt() > 0 )
		{
			// A very experimental and weird way of getting a crossbow bolt to deal accurate knockback.
			CBaseAnimating *pOtherAnimating = pOther->GetBaseAnimating();
			if ( pOtherAnimating && pOtherAnimating->GetModelPtr() && pOtherAnimating->GetModelPtr()->numbones() > 1 )
			{
				int iClosestBone = -1;
				float flCurDistSqr = Square( 128.0f );
				matrix3x4_t bonetoworld;
				Vector vecBonePos;
				for ( int i = 0; i < pOtherAnimating->GetModelPtr()->numbones(); i++ )
				{
					pOtherAnimating->GetBoneTransform( i, bonetoworld );
					MatrixPosition( bonetoworld, vecBonePos );

					float flDist = vecBonePos.DistToSqr( GetLocalOrigin() );
					if ( flDist < flCurDistSqr )
					{
						iClosestBone = i;
						flCurDistSqr = flDist;
					}
				}

				if ( iClosestBone != -1 )
				{
					tr.physicsbone = pOtherAnimating->GetPhysicsBone( iClosestBone );
				}
			}
		}
#endif
		float flDamage = m_flDamage;
		if ( m_iElectric != -1 )
			flDamage *= sk_crossbow_elec_direct_dmg_scale.GetFloat();

		if ( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() && pOther->IsNPC() )
		{
#ifdef MAPBASE
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), flDamage, DMG_NEVERGIB );
#else
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), sk_plr_dmg_crossbow.GetFloat(), DMG_NEVERGIB );
#endif
			dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );

			CBasePlayer *pPlayer = ToBasePlayer( GetOwnerEntity() );
			if ( pPlayer )
			{
				gamestats->Event_WeaponHit( pPlayer, true, "weapon_crossbow", dmgInfo );
			}

		}
		else
		{
#ifdef MAPBASE
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), flDamage, DMG_BULLET | DMG_NEVERGIB );
#else
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), sk_plr_dmg_crossbow.GetFloat(), DMG_BULLET | DMG_NEVERGIB );
#endif
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
		}

		ApplyMultiDamage();

		//Adrian: keep going through the glass.
		if ( pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS )
			return;

		if ( !pOther->IsAlive() )
		{
			// We killed it! 
			const surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );
			if ( pdata->game.material == CHAR_TEX_GLASS )
			{
				return;
			}
		}

		SetAbsVelocity( Vector( 0, 0, 0 ) );

		// play body "thwack" sound
		EmitSound( "Weapon_Crossbow.BoltHitBody" );

		Vector vForward;

		AngleVectors( GetAbsAngles(), &vForward );
		VectorNormalize( vForward );

		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + vForward * 128, MASK_BLOCKLOS, pOther, COLLISION_GROUP_NONE, &tr2 );

		if ( tr2.fraction != 1.0f )
		{
			//			NDebugOverlay::Box( tr2.endpos, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 255, 0, 0, 10 );
			//			NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 0, 255, 0, 10 );

			if ( tr2.m_pEnt == NULL || ( tr2.m_pEnt && tr2.m_pEnt->GetMoveType() == MOVETYPE_NONE ) )
			{
				CEffectData	data;

				data.m_vOrigin = tr2.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = tr2.fraction != 1.0f;

				DispatchEffect( "BoltImpact", data );
			}
		}

		SetTouch( NULL );
		SetThink( NULL );

		if ( !g_pGameRules->IsMultiplayer() )
		{
			if (m_iElectric == -1)
				UTIL_Remove( this );
		}
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if ( pOther->GetMoveType() == MOVETYPE_NONE && !( tr.surface.flags & SURF_SKY ) )
		{
			if (FClassnameIs(pOther, "crossbow_bolt"))
			{
				// remove this crossbow bolt now, refresh the electricity of the struck bolt; this stops a crash but disappears a bolt
				CCrossbowBolt *pBolt = static_cast<CCrossbowBolt *>(pOther);
				pBolt->m_flElecStart = gpGlobals->curtime;
				SetThink( &CCrossbowBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime );
				return;
			}

			EmitSound( "Weapon_Crossbow.BoltHitWorld" );

			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
			float speed = VectorNormalize( vecDir );

			// See if we should reflect off this surface
			float hitDot = DotProduct( tr.plane.normal, -vecDir );

			if ( ( hitDot < 0.5f ) && ( speed > 100 ) && m_iElectric == -1 )
			{
				Vector vReflection = 2.0f * tr.plane.normal * hitDot + vecDir;

				QAngle reflectAngles;

				VectorAngles( vReflection, reflectAngles );

				SetLocalAngles( reflectAngles );

				SetAbsVelocity( vReflection * speed * 0.75f );

				// Start to sink faster
				SetGravity( 1.0f );
			}
			else
			{
				SetThink( &CCrossbowBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );

				//FIXME: We actually want to stick (with hierarchy) to what we've hit
				SetMoveType( MOVETYPE_NONE );

				Vector vForward;

				AngleVectors( GetAbsAngles(), &vForward );
				VectorNormalize( vForward );

				CEffectData	data;

				data.m_vOrigin = tr.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = 0;

				DispatchEffect( "BoltImpact", data );

				UTIL_ImpactTrace( &tr, DMG_BULLET );

				AddEffects( EF_NODRAW );
				SetTouch( NULL );
				SetThink( &CCrossbowBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );
			}
		}
		else
		{
			// Put a mark unless we've hit the sky
			if ( ( tr.surface.flags & SURF_SKY ) == false )
			{
				UTIL_ImpactTrace( &tr, DMG_BULLET );
			}
			if (m_iElectric == -1)
				UTIL_Remove( this );
		}
	}

	if ( m_iElectric != -1 ){
		SetMoveType( MOVETYPE_NONE );
		SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		SetAbsVelocity( Vector( 0, 0, 0 ) );
		AddEffects( EF_NODRAW );
		m_bStruckSomething = true;

		// start aoe damage
		m_flElecStart = gpGlobals->curtime;
		CFmtStr impact;
		impact.sprintf( "weapon_steambow_bolt_impact%d", m_iElectric );
		if ( !pOther->IsWorld() ){
			// Only do this if the NPC allows for it.
			if ( pOther->IsNPC() && pOther->MyNPCPointer()->CanBeStunnedBySteambow() )
			{
				if ( pOther->GetMaxHealth() > 0 && pOther->GetHealth() <= 0 ) {
					// if it's already dead, no point in following it
					m_hAttachedEntity = NULL;
				}
				else {
					// stick it into our attached entity
					m_hAttachedEntity = pOther;
				}
				m_vAttachedEntityOrigin = pOther->WorldSpaceCenter(); // in either case, follow our entity, which may or may not exist anymore
				DispatchParticleEffect( impact.Access(), m_vAttachedEntityOrigin, pOther->GetAbsAngles(), pOther );
			}
		}
		else {
			m_hAttachedEntity = NULL;
			m_vAttachedEntityOrigin = vec3_invalid;
			DispatchParticleEffect( impact.Access(), GetAbsOrigin(), QAngle( 0, 0, 0 ) );
		}

		SetThink( &CCrossbowBolt::ElectricPulseThink );
		SetNextThink( gpGlobals->curtime );
	}
	// todo: this is a really cool feature but there's a lot of problems we need to fix
	/*
	if (pOther->IsNPC())
	{
		// stick the bolt into the NPC
		CAI_BaseNPC *pNPC = static_cast<CAI_BaseNPC*>(pOther);
		SetMoveType( MOVETYPE_NONE );
		SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		SetAbsVelocity( Vector( 0, 0, 0 ) );

		// experimental: find the closest hitbox and stick our bolt into that, like the huntsman from tf2
		int iClosestBone = -1;
		Vector vecClosestBonePos = vec3_invalid;
		float flCurDistSqr = FLT_MAX;
		matrix3x4_t bonetoworld;
		Vector vecBonePos;

		for (int i = 0; i < pNPC->GetModelPtr()->numbones(); i++)
		{
			pNPC->GetBoneTransform( i, bonetoworld );
			MatrixPosition( bonetoworld, vecBonePos );


			float flDist = vecBonePos.DistToSqr( GetAbsOrigin() );
			if (flDist < flCurDistSqr)
			{
				iClosestBone = i;
				flCurDistSqr = flDist;
				vecClosestBonePos = vecBonePos;
			}
		}

		Vector vecAngles;
		AngleVectors( GetAbsAngles(), &vecAngles );
		VectorNormalize( vecAngles );
		SetAbsOrigin( vecClosestBonePos - vecAngles * 8 ); // move the crossbow bolt backwards about 12 units, so it looks to be piercing the enemy

		if (m_pGlowSprite != NULL)
		{
			m_pGlowSprite->TurnOff();
			m_pGlowSprite->SetScale( 0.01f );
			UTIL_Remove( m_pGlowSprite );
		}
	}
	*/
	//if ( g_pGameRules->IsMultiplayer() )
	//{
	//			SetThink( &CCrossbowBolt::ExplodeThink );
	//			SetNextThink( gpGlobals->curtime + 0.1f );
	//}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrossbowBolt::BubbleThink( void )
{
	QAngle angNewAngles;

	VectorAngles( GetAbsVelocity(), angNewAngles );
	SetAbsAngles( angNewAngles );

	SetNextThink( gpGlobals->curtime + 0.1f );

	// Make danger sounds out in front of me, to scare snipers back into their hole
	CSoundEnt::InsertSound( SOUND_DANGER_SNIPERONLY, GetAbsOrigin() + GetAbsVelocity() * 0.2, 120.0f, 0.5f, this, SOUNDENT_CHANNEL_REPEATED_DANGER );

	if ( GetWaterLevel() == 0 )
		return;

	UTIL_BubbleTrail( GetAbsOrigin() - GetAbsVelocity() * 0.1f, GetAbsOrigin(), 5 );
}

//-----------------------------------------------------------------------------
// Purpose: Zap
//-----------------------------------------------------------------------------
void CCrossbowBolt::ElectricPulseThink( void ){
	if ( gpGlobals->curtime >= m_flElecStart + sk_crossbow_elec_radius_duration.GetFloat() ){
		SetThink( NULL );
		UTIL_Remove( this );
		return;
	}

	float flDmg = m_flDamage * sk_crossbow_elec_radius_dmg_scale.GetFloat() * (m_iElectric);	// x1 damage for level 1, x2 for level 2, x3 for level 3
																								// on stock xbow this is 5, 10, 15 for each level respectively
	float flRadius = sk_crossbow_elec_radius.GetFloat() * ( 1 + 0.5f * (m_iElectric - 1) );		// x1 radius for level 1, x1.5 for level 2, x2 for level 3
																								// on stock xbow this is 128, 192, 256 for each level respectively
	CTakeDamageInfo info( this, GetOwnerEntity(), flDmg, DMG_SHOCK );
	CFmtStr aoe;
	aoe.sprintf( "weapon_steambow_bolt_aoe%d", m_iElectric );
	
	EmitSound( "Crossbow_Bolt.ElectricPulse" );

	// perform radius damage and dispatch the appropriate effect
	if ( m_hAttachedEntity != NULL ) {
		// pulse from the centre of the entity
		Vector vOrigin = m_vAttachedEntityOrigin = m_hAttachedEntity->WorldSpaceCenter();
		RadiusDamage( info, vOrigin, flRadius, CLASS_NONE, UTIL_GetLocalPlayer(), RD_NOFALLOFF | RD_STUNNPC );
		DispatchParticleEffect( aoe.Access(), vOrigin, m_hAttachedEntity->GetAbsAngles(), m_hAttachedEntity );

		if ( m_hAttachedEntity->GetMaxHealth() > 0 && m_hAttachedEntity->GetHealth() <= 0 ) {
			// stop following our attached entity
			m_hAttachedEntity = NULL;
		}
	}
	else if ( m_vAttachedEntityOrigin != vec3_invalid ){
		// if the entity somehow died or got removed prematurely, use the stored origin and keep pulsing from there.
		// FIXME: why does the effect not appear when a physics prop breaks?
		if ( !m_bMovedDown ){
			// trace a line straight downwards to find the nearest ground
			trace_t tr;
			Vector vEnd = m_vAttachedEntityOrigin;
			vEnd.z -= 32768;
			UTIL_TraceLine( m_vAttachedEntityOrigin, vEnd, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr ); 
			m_vAttachedEntityOrigin = tr.endpos; // we should have hit something, move there
			m_bMovedDown = true;
		}
		RadiusDamage( info, m_vAttachedEntityOrigin, flRadius, CLASS_NONE, UTIL_GetLocalPlayer(), RD_NOFALLOFF | RD_STUNNPC );
		DispatchParticleEffect( aoe.Access(), m_vAttachedEntityOrigin, QAngle(0, 0, 0), m_hAttachedEntity );
	}
	else {
		// pulse from wherever we landed
		RadiusDamage( info, GetAbsOrigin(), flRadius, CLASS_NONE, UTIL_GetLocalPlayer(), RD_NOFALLOFF | RD_STUNNPC );
		DispatchParticleEffect( aoe.Access(), GetAbsOrigin(), QAngle( 0, 0, 0 ) );
	}

	SetNextThink( gpGlobals->curtime + sk_crossbow_elec_radius_tick.GetFloat() );
}

//-----------------------------------------------------------------------------
// CWeaponCrossbow
//-----------------------------------------------------------------------------

class CWeaponCrossbow : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponCrossbow, CBaseHLCombatWeapon );
public:

	CWeaponCrossbow( void );

	virtual void	Precache( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	void			ChargedFire( void );
	virtual bool	Deploy( void );
	virtual void	Drop( const Vector &vecVelocity );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool	Reload( void );
#ifdef MAPBASE
	virtual void	Reload_NPC( bool bPlaySound = true );
#endif
	virtual void	ItemPostFrame( void );
	virtual void	ItemBusyFrame( void );
	virtual void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#ifdef MAPBASE
	virtual void	Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary );
#endif
	virtual bool	SendWeaponAnim( int iActivity );
	virtual bool	IsWeaponZoomed() { return m_bInZoom; }

	bool	ShouldDisplayHUDHint() { return true; }

#ifdef MAPBASE
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual int	GetMinBurst() { return 1; }
	virtual int	GetMaxBurst() { return 1; }

	virtual float	GetMinRestTime( void ) { return 3.0f; } // 1.5f
	virtual float	GetMaxRestTime( void ) { return 3.0f; } // 2.0f

	virtual float GetFireRate( void )  { return 5.0f; }

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_15DEGREES;
		if ( !GetOwner() || !GetOwner()->IsNPC() )
			return cone;

		static Vector NPCCone = VECTOR_CONE_5DEGREES;

		return NPCCone;
	}

	bool ShouldWeaponFidget( void );
	void LoopChargeAnimation( void );

#endif

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
#ifdef MAPBASE
	DECLARE_ACTTABLE();
#endif

private:

	void	StopEffects( void );
	void	SetSkin( int skinNum );
	void	CheckZoomToggle( void );
	void	FireBolt( int iElec = -1 );
#ifdef MAPBASE
	void	SetBolt( int iSetting );
	void	FireNPCBolt( CAI_BaseNPC *pOwner, Vector &vecShootOrigin, Vector &vecShootDir );
#endif
	void	ToggleZoom( void );

	CNetworkVar( float, m_flChargeStartTime );
	CNetworkVar( bool, m_bCharging );
	CNetworkVar( bool, m_bInZoom );

	// Various states for the crossbow's charger
	enum ChargerState_t
	{
		CHARGER_STATE_START_LOAD,
		CHARGER_STATE_START_CHARGE,
		CHARGER_STATE_READY,
		CHARGER_STATE_DISCHARGE,
		CHARGER_STATE_OFF,
	};

	void	CreateChargerEffects( void );
	void	SetChargerState( ChargerState_t state );
	void	DoLoadEffect( void );

	void	ZoomIn( void );
	void	ZoomOut( void );

private:

	// Charger effects
	ChargerState_t		m_nChargeState;
	CHandle<CSprite>	m_hChargerSprite;

	bool				m_bMustReload;
	bool				m_bPlayingIdleSound;
};

LINK_ENTITY_TO_CLASS( weapon_crossbow, CWeaponCrossbow );

PRECACHE_WEAPON_REGISTER( weapon_crossbow );

IMPLEMENT_SERVERCLASS_ST( CWeaponCrossbow, DT_WeaponCrossbow )
SendPropBool( SENDINFO( m_bCharging ) ),
SendPropFloat(SENDINFO(m_flChargeStartTime)),
SendPropBool(SENDINFO(m_bInZoom)),
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponCrossbow )

DEFINE_FIELD( m_bInZoom, FIELD_BOOLEAN ),
DEFINE_FIELD( m_nChargeState, FIELD_INTEGER ),
DEFINE_FIELD( m_hChargerSprite, FIELD_EHANDLE ),
DEFINE_FIELD( m_flChargeStartTime, FIELD_FLOAT ),
DEFINE_FIELD(m_bPlayingIdleSound, FIELD_BOOLEAN),

END_DATADESC()

#ifdef MAPBASE
acttable_t	CWeaponCrossbow::m_acttable[] =
{
#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_CROSSBOW, true },
	{ ACT_RELOAD, ACT_RELOAD_CROSSBOW, true },
	{ ACT_IDLE, ACT_IDLE_CROSSBOW, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_CROSSBOW, true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED, ACT_IDLE_CROSSBOW_RELAXED, false },//never aims
	{ ACT_IDLE_STIMULATED, ACT_IDLE_CROSSBOW_STIMULATED, false },
	{ ACT_IDLE_AGITATED, ACT_IDLE_ANGRY_CROSSBOW, false },//always aims

	{ ACT_WALK_RELAXED, ACT_WALK_CROSSBOW_RELAXED, false },//never aims
	{ ACT_WALK_STIMULATED, ACT_WALK_CROSSBOW_STIMULATED, false },
	{ ACT_WALK_AGITATED, ACT_WALK_AIM_CROSSBOW, false },//always aims

	{ ACT_RUN_RELAXED, ACT_RUN_CROSSBOW_RELAXED, false },//never aims
	{ ACT_RUN_STIMULATED, ACT_RUN_CROSSBOW_STIMULATED, false },
	{ ACT_RUN_AGITATED, ACT_RUN_AIM_CROSSBOW, false },//always aims

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED, ACT_IDLE_CROSSBOW_RELAXED, false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED, ACT_IDLE_AIM_CROSSBOW_STIMULATED, false },
	{ ACT_IDLE_AIM_AGITATED, ACT_IDLE_ANGRY_CROSSBOW, false },//always aims

	{ ACT_WALK_AIM_RELAXED, ACT_WALK_CROSSBOW_RELAXED, false },//never aims
	{ ACT_WALK_AIM_STIMULATED, ACT_WALK_AIM_CROSSBOW_STIMULATED, false },
	{ ACT_WALK_AIM_AGITATED, ACT_WALK_AIM_CROSSBOW, false },//always aims

	{ ACT_RUN_AIM_RELAXED, ACT_RUN_CROSSBOW_RELAXED, false },//never aims
	{ ACT_RUN_AIM_STIMULATED, ACT_RUN_AIM_CROSSBOW_STIMULATED, false },
	{ ACT_RUN_AIM_AGITATED, ACT_RUN_AIM_CROSSBOW, false },//always aims
	//End readiness activities

	{ ACT_WALK, ACT_WALK_CROSSBOW, true },
	{ ACT_WALK_AIM, ACT_WALK_AIM_CROSSBOW, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_CROSSBOW, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_CROSSBOW, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_CROSSBOW, true },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_CROSSBOW_LOW, true },
	{ ACT_COVER_LOW, ACT_COVER_CROSSBOW_LOW, false },
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_CROSSBOW_LOW, false },
	{ ACT_RELOAD_LOW, ACT_RELOAD_CROSSBOW_LOW, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_CROSSBOW, true },

	{ ACT_ARM, ACT_ARM_RIFLE, false },
	{ ACT_DISARM, ACT_DISARM_RIFLE, false },
#else
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SMG1, true },
	{ ACT_RELOAD, ACT_RELOAD_SMG1, true },
	{ ACT_IDLE, ACT_IDLE_SMG1, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SMG1, true },

	{ ACT_WALK, ACT_WALK_RIFLE, true },
	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED, ACT_IDLE_SMG1_RELAXED, false },//never aims
	{ ACT_IDLE_STIMULATED, ACT_IDLE_SMG1_STIMULATED, false },
	{ ACT_IDLE_AGITATED, ACT_IDLE_ANGRY_SMG1, false },//always aims

	{ ACT_WALK_RELAXED, ACT_WALK_RIFLE_RELAXED, false },//never aims
	{ ACT_WALK_STIMULATED, ACT_WALK_RIFLE_STIMULATED, false },
	{ ACT_WALK_AGITATED, ACT_WALK_AIM_RIFLE, false },//always aims

	{ ACT_RUN_RELAXED, ACT_RUN_RIFLE_RELAXED, false },//never aims
	{ ACT_RUN_STIMULATED, ACT_RUN_RIFLE_STIMULATED, false },
	{ ACT_RUN_AGITATED, ACT_RUN_AIM_RIFLE, false },//always aims

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED, ACT_IDLE_SMG1_RELAXED, false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED, ACT_IDLE_AIM_RIFLE_STIMULATED, false },
	{ ACT_IDLE_AIM_AGITATED, ACT_IDLE_ANGRY_SMG1, false },//always aims

	{ ACT_WALK_AIM_RELAXED, ACT_WALK_RIFLE_RELAXED, false },//never aims
	{ ACT_WALK_AIM_STIMULATED, ACT_WALK_AIM_RIFLE_STIMULATED, false },
	{ ACT_WALK_AIM_AGITATED, ACT_WALK_AIM_RIFLE, false },//always aims

	{ ACT_RUN_AIM_RELAXED, ACT_RUN_RIFLE_RELAXED, false },//never aims
	{ ACT_RUN_AIM_STIMULATED, ACT_RUN_AIM_RIFLE_STIMULATED, false },
	{ ACT_RUN_AIM_AGITATED, ACT_RUN_AIM_RIFLE, false },//always aims
	//End readiness activities

	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_RIFLE, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_SMG1, true },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SMG1_LOW, true },
	{ ACT_COVER_LOW, ACT_COVER_SMG1_LOW, false },
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_SMG1_LOW, false },
	{ ACT_RELOAD_LOW, ACT_RELOAD_SMG1_LOW, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_SMG1, true },
#endif

#if EXPANDED_HL2_COVER_ACTIVITIES
	{ ACT_RANGE_AIM_MED, ACT_RANGE_AIM_CROSSBOW_MED, false },
	{ ACT_RANGE_ATTACK1_MED, ACT_RANGE_ATTACK_CROSSBOW_MED, false },
#endif

#ifdef MAPBASE
	// HL2:DM activities (for third-person animations in SP)
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_CROSSBOW, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_CROSSBOW, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_CROSSBOW, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_CROSSBOW, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_CROSSBOW, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_CROSSBOW, false },
#if EXPANDED_HL2DM_ACTIVITIES
	{ ACT_HL2MP_WALK, ACT_HL2MP_WALK_CROSSBOW, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2, ACT_HL2MP_GESTURE_RANGE_ATTACK2_CROSSBOW, false },
#endif
#endif
};

IMPLEMENT_ACTTABLE(CWeaponCrossbow);

acttable_t* GetCrossbowActtable()
{
	return CWeaponCrossbow::m_acttable;
}

int GetCrossbowActtableCount()
{
	return ARRAYSIZE(CWeaponCrossbow::m_acttable);
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponCrossbow::CWeaponCrossbow( void )
{
	m_bFiresUnderwater = true;
	m_bAltFiresUnderwater = true;
	m_bInZoom = false;
	m_bMustReload = false;
	m_bCharging = false;
	m_flChargeStartTime = 0.0f;
	m_bPlayingIdleSound = false;

#ifdef MAPBASE
	m_fMinRange1 = 24;
	m_fMaxRange1 = 5000;
	m_fMinRange2 = 24;
	m_fMaxRange2 = 5000;
#endif
}

#define	CROSSBOW_GLOW_SPRITE	"sprites/light_glow02_noz.vmt"
#define	CROSSBOW_GLOW_SPRITE2	"sprites/blueflare1.vmt"
#define BATTERY_SPRITE			"sprites/glow02.vmt"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Precache( void )
{
	m_bNoEmptyReload = true;

	UTIL_PrecacheOther( "crossbow_bolt" );

	PrecacheScriptSound( "Weapon_Crossbow.BoltHitBody" );
	PrecacheScriptSound( "Weapon_Crossbow.BoltHitWorld" );
	PrecacheScriptSound( "Weapon_Crossbow.BoltSkewer" );

	PrecacheModel( CROSSBOW_GLOW_SPRITE );
	PrecacheModel( CROSSBOW_GLOW_SPRITE2 );
	PrecacheModel( BATTERY_SPRITE );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponCrossbow::PrimaryAttack( void )
{
	if ( m_bInZoom && g_pGameRules->IsMultiplayer() )
	{
		//		FireSniperBolt();
		FireBolt();
	}
	else
	{
		FireBolt();
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		m_iPrimaryAttacks++;
		gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );

#ifdef MAPBASE
		pPlayer->SetAnimation( PLAYER_ATTACK1 );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponCrossbow::SecondaryAttack( void )
{
	if ( !m_bCharging && Clip1() != 0 ){
		m_flChargeStartTime = gpGlobals->curtime;
		m_bCharging = true;
		WeaponSound( WPN_DOUBLE );
		SendWeaponAnim( ACT_VM_CHARGE_INTRO );
		SetContextThink( &CWeaponCrossbow::LoopChargeAnimation, gpGlobals->curtime + SequenceDuration(), "LoopContext" );
	}
}

void CWeaponCrossbow::ChargedFire( void )
{
	int iElec = floor( ( gpGlobals->curtime - m_flChargeStartTime ) * 3 / 2 ); // 0.67s per level of charge, 0 = level 1, 1 = level 2, 2 = level 3
	if ( iElec > 3 )
		iElec = 3;

	FireBolt( iElec );
	StopWeaponSound( WPN_DOUBLE );

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		m_iPrimaryAttacks++;
		gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );

#ifdef MAPBASE
		pPlayer->SetAnimation( PLAYER_ATTACK1 );
#endif
	}
	m_bCharging = false;
}

//-------------------------------------------------------------------------------------------
// Purpose: Overridden crossbow reload to handle the three different types of reload we have
//-------------------------------------------------------------------------------------------
bool CWeaponCrossbow::Reload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (!pOwner)
		return false;

	if (m_bInZoom)
		ZoomOut();

	if (Clip1() == 0 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) >= 2) {
		if (DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_SECONDARYRELOAD )) {
			m_bMustReload = false;
			return true;
		}
	}
	else if (Clip1() == 0 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) == 1) {
		if (DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD_EMPTY )) {
			m_bMustReload = false;
			return true;
		}
	}
	else if (DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD )) {
		m_bMustReload = false;
		return true;
	}
	return false;
}
#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Reload_NPC( bool bPlaySound )
{
	BaseClass::Reload_NPC( bPlaySound );

	SetBolt( 0 );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::CheckZoomToggle( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer->m_afButtonPressed & IN_ATTACK3 )
	{
		ToggleZoom();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::ItemBusyFrame( void )
{
	// stop any idle sounds since this is probably not an idle or fidget
	if (m_bPlayingIdleSound)
	{
		m_bPlayingIdleSound = false;
		StopWeaponSound( SPECIAL1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::ItemPostFrame( void )
{
	// Allow zoom toggling
	CheckZoomToggle();

	if (m_bMustReload && HasWeaponIdleTimeElapsed())
	{
		Reload();
	}
	
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	// handle idle steam sound
	if (!pPlayer)
	{
		StopWeaponSound( SPECIAL1 );
	}
	else if (!m_bPlayingIdleSound && GetActivity() == ACT_VM_IDLE)
	{
		m_bPlayingIdleSound = true;
		WeaponSound( SPECIAL1 );
	}
	else if (m_bPlayingIdleSound && GetActivity() != ACT_VM_IDLE && GetActivity() != ACT_VM_FIDGET)
	{
		m_bPlayingIdleSound = false;
		StopWeaponSound( SPECIAL1 );
	}

	if ( pPlayer && m_bCharging ){
		pPlayer->m_nButtons &= ~IN_ATTACK; // don't interrupt attack2 with attack1
		float flElapsedTime = gpGlobals->curtime - m_flChargeStartTime;
		if ( pPlayer->m_afButtonReleased & IN_ATTACK2 && flElapsedTime >= 0.667f ){
			// 1 battery minimum to fire
			ChargedFire();
		}
		else if ( pPlayer->m_afButtonReleased & IN_ATTACK2 && flElapsedTime < 0.667f ){
			// don't fire if 0 batteries
			StopWeaponSound( WPN_DOUBLE );
			SendWeaponAnim( ACT_VM_IDLE );
			m_bCharging = false;
		}
	}
	else {
		m_bCharging = false;
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::FireBolt( int iElec /* = -1 */ )
{
	/*if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}*/

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	pOwner->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAG_RESTART );

	Vector vecAiming = pOwner->GetAutoaimVector( 0 );
	Vector vecSrc = pOwner->Weapon_ShootPosition();

	QAngle angAiming;
	VectorAngles( vecAiming, angAiming );

#if defined(HL2_EPISODIC)
	// !!!HACK - the other piece of the Alyx crossbow bolt hack for Outland_10 (see ::BoltTouch() for more detail)
	if ( FStrEq( STRING( gpGlobals->mapname ), "ep2_outland_10" ) )
	{
		trace_t tr;
		UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 24.0f, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );

		if ( tr.m_pEnt != NULL && tr.m_pEnt->Classify() == CLASS_PLAYER_ALLY_VITAL )
		{
			// If Alyx is right in front of the player, make sure the bolt starts outside of the player's BBOX, or the bolt
			// will instantly collide with the player after the owner of the bolt is switched to Alyx in ::BoltTouch(). We 
			// avoid this altogether by making it impossible for the bolt to collide with the player.
			vecSrc += vecAiming * 24.0f;
		}
	}
#endif

	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate( vecSrc, angAiming, pOwner, iElec );

	float flVelScale = iElec != -1 ? BOLT_ELEC_VEL_SCALE : 1.0f;
	if ( pOwner->GetWaterLevel() == 3 )
	{
		pBolt->SetAbsVelocity( vecAiming * BOLT_WATER_VELOCITY * flVelScale );
	}
	else
	{
		pBolt->SetAbsVelocity( vecAiming * BOLT_AIR_VELOCITY * flVelScale );
	}

	m_iClip1--;
	if (m_iClip1 == 0) {
		m_bMustReload = true;
	}
#ifdef MAPBASE
	SetBolt( 1 );
#endif

	pOwner->ViewPunch( QAngle( -2, 0, 0 ) );

	WeaponSound( SINGLE );
	WeaponSound( SPECIAL2 );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 200, 0.2 );

	int iAct = -1;

	switch (iElec) {
		// determine what sequence we need to use, and how long to idle for
		case -1:
			iAct = ACT_VM_PRIMARYATTACK;
			break;
		case 1:
			iAct = ACT_VM_SECONDARYATTACK2;
			break;
		case 2:
			iAct = ACT_VM_SECONDARYATTACK3;
			break;
		case 3:
			iAct = ACT_VM_SECONDARYATTACK4;
			break;
		default:
			iAct = ACT_VM_PRIMARYATTACK;
			break;
	}
	
	SendWeaponAnim( iAct );
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration());
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();

	if ( !m_iClip1 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pOwner->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	}
	//SetChargerState( CHARGER_STATE_DISCHARGE );
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Sets whether or not the bolt is visible
//-----------------------------------------------------------------------------
inline void CWeaponCrossbow::SetBolt( int iSetting )
{
	int iBody = FindBodygroupByName( "bolt" );
	if ( iBody != -1 /*|| (GetOwner() && GetOwner()->IsPlayer())*/ ) // TODO: Player models check the viewmodel instead of the worldmodel, but setting the bodygroup regardless can cause a crash, so we need a better solution
		SetBodygroup( iBody, iSetting );
	else
		m_nSkin = iSetting;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::FireNPCBolt( CAI_BaseNPC *pOwner, Vector &vecShootOrigin, Vector &vecShootDir )
{
	Assert( pOwner );

	QAngle angAiming;
	VectorAngles( vecShootDir, angAiming );

	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate( vecShootOrigin, angAiming, pOwner );

	if ( pOwner->GetWaterLevel() == 3 )
	{
		pBolt->SetAbsVelocity( vecShootDir * BOLT_WATER_VELOCITY );
	}
	else
	{
		pBolt->SetAbsVelocity( vecShootDir * BOLT_AIR_VELOCITY );
	}

	m_iClip1--;

	SetBolt( 1 );

	WeaponSound( SINGLE_NPC );
	WeaponSound( SPECIAL2 );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 200, 0.2 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 2.5f;

	SetSkin( BOLT_SKIN_GLOW );
	//SetChargerState( CHARGER_STATE_DISCHARGE );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponCrossbow::Deploy( void )
{
	if ( m_iClip1 <= 0 )
	{
#ifdef MAPBASE
		SetBolt( 1 );
#endif
		return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_CROSSBOW_DRAW_UNLOADED, (char*)GetAnimPrefix() );
	}

	SetSkin( BOLT_SKIN_GLOW );

#ifdef MAPBASE
	SetBolt( 0 );
#endif

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponCrossbow::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	StopEffects();
	m_bCharging = false;
	StopWeaponSound( WPN_DOUBLE );
	StopWeaponSound( SPECIAL1 );
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::ToggleZoom( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	if ( m_bInZoom )
		ZoomOut();
	else
		ZoomIn();
}

#define	BOLT_TIP_ATTACHMENT	2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::CreateChargerEffects( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

#ifdef MAPBASE
	if ( m_hChargerSprite != NULL || pOwner == NULL )
#else
	if ( m_hChargerSprite != NULL )
#endif
		return;

	m_hChargerSprite = CSprite::SpriteCreate( CROSSBOW_GLOW_SPRITE, GetAbsOrigin(), false );

	if ( m_hChargerSprite )
	{
		m_hChargerSprite->SetAttachment( pOwner->GetViewModel(), BOLT_TIP_ATTACHMENT );
		m_hChargerSprite->SetTransparency( kRenderTransAdd, 255, 128, 0, 255, kRenderFxNoDissipation );
		m_hChargerSprite->SetBrightness( 0 );
		m_hChargerSprite->SetScale( 0.1f );
		m_hChargerSprite->TurnOff();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : skinNum - 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::SetSkin( int skinNum )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if ( pViewModel == NULL )
		return;

	pViewModel->m_nSkin = skinNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::DoLoadEffect( void )
{
	SetSkin( BOLT_SKIN_GLOW );

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if ( pViewModel == NULL )
		return;

	CEffectData	data;

	data.m_nEntIndex = pViewModel->entindex();
	data.m_nAttachmentIndex = 1;

	DispatchEffect( "CrossbowLoad", data );

	CSprite *pBlast = CSprite::SpriteCreate( CROSSBOW_GLOW_SPRITE2, GetAbsOrigin(), false );

	if ( pBlast )
	{
		pBlast->SetAttachment( pOwner->GetViewModel(), 1 );
		pBlast->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone );
		pBlast->SetBrightness( 128 );
		pBlast->SetScale( 0.2f );
		pBlast->FadeOutFromSpawn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::SetChargerState( ChargerState_t state )
{
	// Make sure we're setup
	CreateChargerEffects();

	// Don't do this twice
	if ( state == m_nChargeState )
		return;

	m_nChargeState = state;

	switch ( m_nChargeState )
	{
	case CHARGER_STATE_START_LOAD:
		// Shoot some sparks and draw a beam between the two outer points
		DoLoadEffect();

#ifdef MAPBASE
		SetBolt( 0 );
#endif

		break;

	case CHARGER_STATE_START_CHARGE:
	{
		if ( m_hChargerSprite == NULL )
			break;

		m_hChargerSprite->SetBrightness( 32, 0.5f );
		m_hChargerSprite->SetScale( 0.025f, 0.5f );
		m_hChargerSprite->TurnOn();
	}

	break;

	case CHARGER_STATE_READY:
	{
		// Get fully charged
		if ( m_hChargerSprite == NULL )
			break;

		m_hChargerSprite->SetBrightness( 80, 1.0f );
		m_hChargerSprite->SetScale( 0.1f, 0.5f );
		m_hChargerSprite->TurnOn();
	}

	break;

	case CHARGER_STATE_DISCHARGE:
	{
		SetSkin( BOLT_SKIN_NORMAL );

		if ( m_hChargerSprite == NULL )
			break;

		m_hChargerSprite->SetBrightness( 0 );
		m_hChargerSprite->TurnOff();
	}

	break;

	case CHARGER_STATE_OFF:
	{
		SetSkin( BOLT_SKIN_NORMAL );

		if ( m_hChargerSprite == NULL )
			break;

		m_hChargerSprite->SetBrightness( 0 );
		m_hChargerSprite->TurnOff();
	}
	break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch ( pEvent->event )
	{
	case EVENT_WEAPON_THROW:
		//SetChargerState( CHARGER_STATE_START_LOAD );
		break;

	case EVENT_WEAPON_THROW2:
		//SetChargerState( CHARGER_STATE_START_CHARGE );
		break;

	case EVENT_WEAPON_THROW3:
		//SetChargerState( CHARGER_STATE_READY );
		break;

#ifdef MAPBASE
	case EVENT_WEAPON_SMG1:
	{
		CAI_BaseNPC *pNPC = pOperator->MyNPCPointer();
		Assert( pNPC );

		Vector vecSrc = pNPC->Weapon_ShootPosition();
		Vector vecAiming = pNPC->GetActualShootTrajectory( vecSrc );

		FireNPCBolt( pNPC, vecSrc, vecAiming );
	}
	break;
#endif

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCBolt( pOperator->MyNPCPointer(), vecShootOrigin, vecShootDir );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Set the desired activity for the weapon and its viewmodel counterpart
// Input  : iActivity - activity to play
//-----------------------------------------------------------------------------
bool CWeaponCrossbow::SendWeaponAnim( int iActivity )
{
	int newActivity = iActivity;

	//For now, just set the ideal activity and be done with it
	return BaseClass::SendWeaponAnim( newActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Stop all zooming and special effects on the viewmodel
//-----------------------------------------------------------------------------
void CWeaponCrossbow::StopEffects( void )
{
	// Stop zooming
	if ( m_bInZoom )
	{
		ToggleZoom();
	}

	// Turn off our sprites
	//SetChargerState( CHARGER_STATE_OFF );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Drop( const Vector &vecVelocity )
{
	StopEffects();
	BaseClass::Drop( vecVelocity );
}

bool CWeaponCrossbow::ShouldWeaponFidget()
{
	return !m_bInZoom;
}

void CWeaponCrossbow::ZoomIn()
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (pPlayer->SetFOV( this, 20, 0.2f ))
	{
		UTIL_ScreenFade( pPlayer, { 0, 0, 0, 255 }, 0.4, 0, FFADE_IN );
		m_bInZoom = true;
		WeaponSound( SPECIAL3 );
	}
}

void CWeaponCrossbow::ZoomOut()
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (pPlayer->SetFOV( this, 0, 0.2f ))
	{
		WeaponSound( SPECIAL4 );
		m_bInZoom = false;
	}
}

void CWeaponCrossbow::LoopChargeAnimation()
{
	if (m_bCharging)
	{
		SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	}
}