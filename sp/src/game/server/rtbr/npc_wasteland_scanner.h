#ifndef NPC_WASTELANDSCANNER_H
#define NPC_WASTELANDSCANNER_H

#include "npc_basescanner.h"
#include "server_class.h"

//------------------------------------
// Spawnflags
//------------------------------------
#define SF_WLSCANNER_NO_DYNAMIC_LIGHT		(1 << 16)
#define SF_WLSCANNER_STRIDER_SCOUT			(1 << 17)
#define SF_WLSCANNER_ALWAYS_DROP_BATTERY	(1 << 18)
#define SF_WLSCANNER_THRUSTERS_ON			(1 << 19)

class CBeam;
class CSprite;
class SmokeTrail;
class CSpotlightEnd;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_WastelandScanner : public CNPC_BaseScanner
{
	DECLARE_CLASS( CNPC_WastelandScanner, CNPC_BaseScanner );
	DECLARE_SERVERCLASS();

public:
	CNPC_WastelandScanner();

	int				GetSoundInterests( void ) { return (SOUND_WORLD|SOUND_COMBAT|SOUND_PLAYER|SOUND_DANGER); }
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );

	bool			FValidateHintType(CAI_Hint *pHint);

	virtual int		TranslateSchedule( int scheduleType );
	Disposition_t	IRelationType(CBaseEntity *pTarget);

	void			NPCThink( void );

	void			GatherConditions( void );
	void			PrescheduleThink( void );
	void			Precache(void);
	void			RunTask( const Task_t *pTask );
	int				SelectSchedule(void);
	virtual char	*GetScannerSoundPrefix( void );
	void			Spawn(void);
	void			Activate();
	void			StartTask( const Task_t *pTask );
	void			UpdateOnRemove( void );
	void			DeployMine();
	float			GetMaxSpeed();
	virtual void	Gib( void );
	virtual const char* GetScannerExplosionEffect() { return "npc_wasteland_scanner_death"; }
	void			HandleAnimEvent( animevent_t *pEvent );

	void			InputDisableSpotlight( inputdata_t &inputdata );
	void			InputSetFollowTarget( inputdata_t &inputdata );
	void			InputClearFollowTarget( inputdata_t &inputdata );
	void			InputInspectTargetPhoto( inputdata_t &inputdata );
	void			InputInspectTargetSpotlight( inputdata_t &inputdata );
	void			InputDeployMine( inputdata_t &inputdata );
	void			InputEquipMine( inputdata_t &inputdata );
	void			InputShouldInspect( inputdata_t &inputdata );

	void			InspectTarget( inputdata_t &inputdata, ScannerFlyMode_t eFlyMode );

	void			Event_Killed( const CTakeDamageInfo &info );

	char			*GetEngineSound( void );

	virtual float	MinGroundDist(void);

	virtual float	GetHeadTurnRate( void );

	virtual int		RangeAttack1Conditions( float flDot, float flDist );
	virtual void	MoveExecute_Alive(float flInterval);
public:
	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

	// ------------------------------
	//	Inspecting
	// ------------------------------
	Vector			m_vInspectPos;
	float			m_fInspectEndTime;
	float			m_fCheckCitizenTime;	// Time to look for citizens to harass
	float			m_fCheckHintTime;		// Time to look for hints to inspect
	bool			m_bShouldInspect;
	bool			m_bOnlyInspectPlayers;
	bool			m_bNeverInspectPlayers;

	void			SetInspectTargetToEnt(CBaseEntity *pEntity, float fInspectDuration);
	void			SetInspectTargetToPos(const Vector &vInspectPos, float fInspectDuration);
	void			SetInspectTargetToHint(CAI_Hint *pHint, float fInspectDuration);
	void			ClearInspectTarget(void);
	bool			HaveInspectTarget(void);
	Vector			InspectTargetPosition(void);
	bool			IsValidInspectTarget(CBaseEntity *pEntity);
	CBaseEntity*	BestInspectTarget(void);
	void			RequestInspectSupport(void);

	bool			IsStriderScout() { return HasSpawnFlags( SF_WLSCANNER_STRIDER_SCOUT ); }

	// ------------------------
	//  Photographing
	// ------------------------
	float			m_fNextPhotographTime;
	CSprite*		m_pEyeFlash;

	void			TakePhoto( void );
	void			BlindFlashTarget( CBaseEntity *pTarget );

	// ------------------------------
	//	Spotlight
	// ------------------------------
	Vector			m_vSpotlightTargetPos;
	Vector			m_vSpotlightCurrentPos;
	CHandle<CBeam>	m_hSpotlight;
	CHandle<CSpotlightEnd> m_hSpotlightTarget;
	Vector			m_vSpotlightDir;
	Vector			m_vSpotlightAngVelocity;
	float			m_flSpotlightCurLength;
	float			m_flSpotlightMaxLength;
	float			m_flSpotlightGoalWidth;
	float			m_fNextSpotlightTime;
	int				m_nHaloSprite;

	void			SpotlightUpdate(void);
	Vector			SpotlightTargetPos(void);
	Vector			SpotlightCurrentPos(void);
	void			SpotlightCreate(void);
	void			SpotlightDestroy(void);

	// ------------------------------
	//	Beam attack
	// ------------------------------
	void			BeamCreate();
	void			BeamDestroy();
	void			SignalAttackState(bool bAttack = true);
	float			m_flNextZapTime;
	bool			m_bAttacking;
	CNetworkVar( bool, m_bIsBeaming );
	CNetworkVar( EHANDLE, m_hTarget );

private:
	bool			MovingToInspectTarget( void );
	virtual float	GetGoalDistance( void );

	bool m_bIsOpen;			// Only for claw scanner

	COutputEvent		m_OnPhotographPlayer;
	COutputEvent		m_OnPhotographNPC;

	bool				OverrideMove(float flInterval);
	void				MoveToTarget(float flInterval, const Vector &MoveTarget);
	void				MoveToSpotlight(float flInterval);
	void				MoveToPhotograph(float flInterval);

	// Attacks
	bool				m_bNoLight;
	bool				m_bPhotoTaken;

	void				AttackPreFlash(void);
	void				AttackFlash(void);
	void				AttackFlashBlind(void);

	virtual void		AttackDivebomb(void);

	DEFINE_CUSTOM_AI;

	// Custom interrupt conditions
	enum
	{
		COND_WLSCANNER_HAVE_INSPECT_TARGET = BaseClass::NEXT_CONDITION,
		COND_WLSCANNER_INSPECT_DONE,							
		COND_WLSCANNER_CAN_PHOTOGRAPH,
		COND_WLSCANNER_SPOT_ON_TARGET,

		NEXT_CONDITION,
	};

	// Custom schedules
	enum
	{
		SCHED_WLSCANNER_SPOTLIGHT_HOVER = BaseClass::NEXT_SCHEDULE,
		SCHED_WLSCANNER_SPOTLIGHT_INSPECT_POS,
		SCHED_WLSCANNER_SPOTLIGHT_INSPECT_CIT,
		SCHED_WLSCANNER_PHOTOGRAPH_HOVER,
		SCHED_WLSCANNER_PHOTOGRAPH,
		SCHED_WLSCANNER_ATTACK_FLASH,
		SCHED_WLSCANNER_MOVE_TO_INSPECT,
		SCHED_WLSCANNER_PATROL,
		SCHED_WLSCANNER_RANGE_ATTACK1,

		NEXT_SCHEDULE,
	};

	// Custom tasks
	enum
	{
		TASK_WLSCANNER_SET_FLY_PHOTO = BaseClass::NEXT_TASK,
		TASK_WLSCANNER_SET_FLY_SPOT,
		TASK_WLSCANNER_PHOTOGRAPH,
		TASK_WLSCANNER_ATTACK_PRE_FLASH,
		TASK_WLSCANNER_ATTACK_FLASH,
		TASK_WLSCANNER_SPOT_INSPECT_ON,
		TASK_WLSCANNER_SPOT_INSPECT_WAIT,
		TASK_WLSCANNER_SPOT_INSPECT_OFF,
		TASK_WLSCANNER_CLEAR_INSPECT_TARGET,
		TASK_WLSCANNER_GET_PATH_TO_INSPECT_TARGET,
		TASK_WLSCANNER_ZAP,

		NEXT_TASK,
	};

	DECLARE_DATADESC();
};

#endif // NPC_WASTELANDSCANNER_H
