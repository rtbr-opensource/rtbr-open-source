//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Grigori's personal shotgun (npc_monk)
//
//=============================================================================//

#include	"cbase.h"
#include	"npcevent.h"
#include	"basehlcombatweapon_shared.h"
#include	"basecombatcharacter.h"
#include	"ai_basenpc.h"
#include	"player.h"
#include	"gamerules.h"		// For g_pGameRules
#include	"in_buttons.h"
#include	"soundent.h"
#include	"vstdlib/random.h"

#ifdef RTBR_DLL
#include	"gamestats.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sk_auto_reload_time;

#ifdef MAPBASE
extern acttable_t *GetShotgunActtable();
extern int GetShotgunActtableCount();
#endif

class CWeaponAnnabelle : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponAnnabelle, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

private:
	bool	m_bNeedPump;		// When emptied completely
	bool	m_bDelayedFire1;	// Fire primary when finished reloading
	bool	m_bDelayedFire2;	// Fire secondary when finished reloading
	bool	m_bPrimary;			// Whether using primary fire.

public:
	void	Precache( void );

	bool Holster(CBaseCombatWeapon * pSwitchingTo);

	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;

#ifdef MAPBASE
		if (GetOwner() && GetOwner()->OverridingWeaponProficiency())
		{
			// If the owner's weapon proficiency is being overridden, return a more realistic spread
			static Vector cone2 = VECTOR_CONE_6DEGREES;
			return cone2;
		}
#endif

		if (m_bPrimary){
			cone = vec3_origin;
		}
		else{
			cone = VECTOR_CONE_5DEGREES;
		}
		return cone;
	}

	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 3; }

	void ItemHolsterFrame( void );
#ifdef RTBR_DLL
	void ItemPostFrame( void );
	void PrimaryAttack( void );
	void SecondaryAttack(void);
#endif
	bool StartReload( void );
	bool Reload( void );
	void FillClip( void );
	void FinishReload( void );
	void CheckHolsterReload( void );
	void Pump( void );
	void DryFire( void );
	virtual float GetFireRate( void ) { return 1.5; };
	virtual float			GetMinRestTime() { return 1.0; }
	virtual float			GetMaxRestTime() { return 1.5; }

	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

#ifdef MAPBASE
	virtual acttable_t		*GetBackupActivityList() { return GetShotgunActtable(); }
	virtual int				GetBackupActivityListCount() { return GetShotgunActtableCount(); }
#endif

	DECLARE_ACTTABLE();

	CWeaponAnnabelle(void);
};

IMPLEMENT_SERVERCLASS_ST(CWeaponAnnabelle, DT_WeaponAnnabelle)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_annabelle, CWeaponAnnabelle );
#ifndef HL2MP
PRECACHE_WEAPON_REGISTER(weapon_annabelle);
#endif

BEGIN_DATADESC( CWeaponAnnabelle )
	DEFINE_FIELD( m_bNeedPump, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDelayedFire1, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDelayedFire2, FIELD_BOOLEAN ),
END_DATADESC()

acttable_t	CWeaponAnnabelle::m_acttable[] = 
{
	// todo: remove the ! when rtbr gets fixed
#if !defined(EXPANDED_HL2_WEAPON_ACTIVITIES) && AR2_ACTIVITY_FIX == 1
	{ ACT_IDLE,						ACT_IDLE_AR2,						false },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_AR2,					true },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,				false },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_ANNABELLE,			true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_ANNABELLE_LOW,		true },
	{ ACT_RELOAD,					ACT_RELOAD_ANNABELLE,				true },
	{ ACT_WALK,						ACT_WALK_AR2,						true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_AR2,					true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				false },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			false },
	{ ACT_RUN,						ACT_RUN_AR2,						true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_AR2,					true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				false },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			false },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_ANNABELLE,	true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_ANNABELLE_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_ANNABELLE,		false },

	{ ACT_ARM,						ACT_ARM_RIFLE,				true },
	{ ACT_DISARM,					ACT_DISARM_RIFLE,				true },
#else
#ifdef MAPBASE
	{ ACT_IDLE,						ACT_IDLE_SMG1,						false },
#endif
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,				true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SHOTGUN,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,					true },
	{ ACT_WALK,						ACT_WALK_RIFLE,						true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,					true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				false },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			false },
	{ ACT_RUN,						ACT_RUN_RIFLE,						true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,					true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				false },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			false },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,				false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,			false },
#endif

#ifdef MAPBASE
	// HL2:DM activities (for third-person animations in SP)
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_AR2,                    false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_AR2,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_AR2,            false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_AR2,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_AR2,        false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_AR2,                    false },
#if EXPANDED_HL2DM_ACTIVITIES
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_AR2,						false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_SHOTGUN,    false },
#endif
#endif
};

IMPLEMENT_ACTTABLE(CWeaponAnnabelle);


void CWeaponAnnabelle::Precache( void )
{
	CBaseCombatWeapon::Precache();
}

bool CWeaponAnnabelle::Holster(CBaseCombatWeapon *pSwitchingTo)
{

	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponAnnabelle::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_SHOTGUN_FIRE:
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition();
			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );
			WeaponSound( SINGLE_NPC );
			pOperator->DoMuzzleFlash();
			m_iClip1 = m_iClip1 - 1;

			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
			pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0 );
		}
		break;

		default:
			CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponAnnabelle::StartReload( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	// If shotgun totally emptied then a pump animation is needed
	if (m_iClip1 <= 0)
	{
		m_bNeedPump = true;
	}

	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	SendWeaponAnim( ACT_SHOTGUN_RELOAD_START );

	// Make shotgun shell visible
	SetBodygroup(1,0);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

#ifdef MAPBASE
	if ( pOwner->IsPlayer() )
	{
		static_cast<CBasePlayer*>(pOwner)->SetAnimation( PLAYER_RELOAD );
	}
#endif

	m_bInReload = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponAnnabelle::Reload( void )
{
	// Check that StartReload was called first
	if (!m_bInReload)
	{
		Warning("ERROR: Shotgun Reload called incorrectly!\n");
	}

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	FillClip();
	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(RELOAD);
	SendWeaponAnim( ACT_VM_RELOAD );

	pOwner->m_flNextAttack = gpGlobals->curtime;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponAnnabelle::FinishReload( void )
{
	// Make shotgun shell invisible
	SetBodygroup(1,1);

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return;

	m_bInReload = false;

	// Finish reload animation
	SendWeaponAnim( ACT_SHOTGUN_RELOAD_FINISH );

	pOwner->m_flNextAttack = gpGlobals->curtime;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponAnnabelle::FillClip( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return;

	// Add them to the clip
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		if ( Clip1() < GetMaxClip1() )
		{
			m_iClip1++;
			pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play weapon pump anim
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponAnnabelle::Pump( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();

	if ( pOwner == NULL )
		return;
	
	m_bNeedPump = false;
	
	WeaponSound( SPECIAL1 );

	// Finish reload animation
	if (m_bPrimary){
		SendWeaponAnim(ACT_SHOTGUN_PUMP);
	}

	pOwner->m_flNextAttack	= gpGlobals->curtime + SequenceDuration();
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponAnnabelle::DryFire( void )
{
	WeaponSound(EMPTY);
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

#ifdef RTBR_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponAnnabelle::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if (!pPlayer)
	{
		return;
	}
	m_bPrimary = true;
	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
	m_iClip1 -= 1;

	Vector	vecSrc		= pPlayer->Weapon_ShootPosition( );
	Vector	vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 1.0 );
	
	// Fire the bullets
	pPlayer->FireBullets( 1, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0, -1, -1, 0, NULL, true, true );
	
	
	pPlayer->ViewPunch( QAngle( random->RandomFloat( -2, -1 ), random->RandomFloat( -2, 2 ), 0 ) );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2, GetOwner() );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	if( m_iClip1 )
	{
		// pump so long as some rounds are left.
		m_bNeedPump = true;
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
}
//-----------------------------------------------------------------------------
// Purpose: Fire fast. Bullets, now.
//-----------------------------------------------------------------------------
void CWeaponAnnabelle::SecondaryAttack(void)
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	
	if (!pPlayer)
	{
		return;
	}
	m_bPrimary = false;
	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_SECONDARYATTACK);
	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
	m_iClip1 -= 1;

	Vector	vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);

	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 1.0);

	// Fire the bullets
	pPlayer->FireBullets(1, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0, -1, -1, 0, NULL, true, true);

	pPlayer->ViewPunch(QAngle(random->RandomFloat(-2, -1), random->RandomFloat(-2, 2), 0));

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2, GetOwner());

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());
}

//-----------------------------------------------------------------------------
// Purpose: Override so shotgun can do mulitple reloads in a row
//-----------------------------------------------------------------------------
void CWeaponAnnabelle::ItemPostFrame( void )
{

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
	{
		return;
	}

	if (m_bInReload)
	{
		// If I'm primary firing and have one round stop reloading and fire
		if ((pOwner->m_nButtons & IN_ATTACK ) && (m_iClip1 >=1))
		{
			m_bInReload		= false;
			m_bNeedPump		= false;
			m_bDelayedFire1 = true;
		}
		// If I'm secondary firing and have one round stop reloading and fire
		else if ((pOwner->m_nButtons & IN_ATTACK2 ) && (m_iClip1 >=2))
		{
			m_bInReload		= false;
			m_bNeedPump		= false;
			m_bDelayedFire2 = true;
		}
		else if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			// If out of ammo end reload
			if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <=0)
			{
				FinishReload();
				return;
			}
			// If clip not full reload again
			if (m_iClip1 < GetMaxClip1())
			{
				Reload();
				return;
			}
			// Clip full, stop reloading
			else
			{
				FinishReload();
				return;
			}
		}
	}
	else
	{			
		// Make shotgun shell invisible
		SetBodygroup(1,1);
	}

	if ((m_bNeedPump) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		Pump();
		return;
	}
	
	// Shotgun uses same timing and ammo for secondary attack
	if ((m_bDelayedFire2 || pOwner->m_nButtons & IN_ATTACK2)&&(m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		m_bDelayedFire2 = false;
		
		if ( (m_iClip1 <= 1 && UsesClipsForAmmo1()))
		{
			// If only one shell is left, do a single shot instead	
			if ( m_iClip1 == 1 )
			{
				PrimaryAttack();
			}
			else if (!pOwner->GetAmmoCount(m_iPrimaryAmmoType))
			{
				DryFire();
			}
			else
			{
				StartReload();
			}
		}

		// Fire underwater?
		else if (GetOwner()->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			if ( pOwner->m_afButtonPressed & IN_ATTACK )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}
			SecondaryAttack();
		}
	}
	else if ( (m_bDelayedFire1 || pOwner->m_nButtons & IN_ATTACK) && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		m_bDelayedFire1 = false;
		if ( (m_iClip1 <= 0 && UsesClipsForAmmo1()) || ( !UsesClipsForAmmo1() && !pOwner->GetAmmoCount(m_iPrimaryAmmoType) ) )
		{
			if (!pOwner->GetAmmoCount(m_iPrimaryAmmoType))
			{
				DryFire();
			}
			else
			{
				StartReload();
			}
		}
		// Fire underwater?
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
			if ( pPlayer && pPlayer->m_afButtonPressed & IN_ATTACK )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}
			PrimaryAttack();
		}
	}

	if ( pOwner->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		StartReload();
	}
	else 
	{
		// no fire buttons down
		m_bFireOnEmpty = false;

		if ( !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime ) 
		{
			// weapon isn't useable, switch.
			if ( !(GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && pOwner->SwitchToNextBestWeapon( this ) )
			{
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
				return;
			}
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if ( m_iClip1 <= 0 && !(GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < gpGlobals->curtime )
			{
				if (StartReload())
				{
					// if we've successfully started to reload, we're done
					return;
				}
			}
		}

		WeaponIdle( );
		return;
	}

}
#endif // RTBR_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAnnabelle::ItemHolsterFrame( void )
{
	// Must be player held
	if ( GetOwner() && GetOwner()->IsPlayer() == false )
		return;

	// We can't be active
	if ( GetOwner()->GetActiveWeapon() == this )
		return;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponAnnabelle::CWeaponAnnabelle( void )
{
	m_bReloadsSingly = true;

	m_bNeedPump		= false;
	m_bDelayedFire1 = false;
	m_bDelayedFire2 = false;

	m_fMinRange1		= 0.0;
	m_fMaxRange1		= 500;
	m_fMinRange2		= 0.0;
	m_fMaxRange2		= 200;
}
