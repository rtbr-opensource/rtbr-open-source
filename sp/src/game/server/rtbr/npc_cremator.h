#define		CREM_AE_IMMO_SHOOT		( 4 )
#define		CREM_AE_IMMO_DONE		( 5 )
#define		CREM_BODY_GUNGONE			1
#define		SF_CREM_WANDER			1<<15


//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------
class CCrematorPlasmaBall : public CBaseCombatCharacter
{
	DECLARE_CLASS(CCrematorPlasmaBall, CBaseCombatCharacter);

public:
	Class_T Classify(void) { return CLASS_NONE; }

public:
	void Spawn(void);
	void Precache(void);
	void BoltTouch(CBaseEntity *pOther);
	void	IgniteOtherIfAllowed(CBaseEntity *pOther);
	bool CreateVPhysics(void);
	unsigned int PhysicsSolidMaskForEntity() const;

protected:

	DECLARE_DATADESC();
};
LINK_ENTITY_TO_CLASS(cremator_plasma_ball, CCrematorPlasmaBall);

BEGIN_DATADESC(CCrematorPlasmaBall)
// Function Pointers,
DEFINE_FUNCTION(BoltTouch),


END_DATADESC()

//=========================================================
//=========================================================
class CNPC_Cremator : public CAI_BaseNPC
{
	DECLARE_CLASS(CNPC_Cremator, CAI_BaseNPC);
	DECLARE_DATADESC();

public:
	void	Precache(void);
	void	Spawn(void);
	void ImmoBeam(int side);
	int SelectSchedule(void);
	void	IdleSound(void);
	void	PrescheduleThink(void);
	void Event_Killed(const CTakeDamageInfo &info);
	int TranslateSchedule(int scheduleType);
	Class_T Classify(void);
	int RangeAttack1Conditions(float flDot, float flDist);
	void HandleAnimEvent(animevent_t *pEvent);
	virtual float        InnateRange1MinRange(void) { return 128; }
	virtual float        InnateRange1MaxRange(void) { return 512; }
	virtual int			MeleeAttack1Conditions(float flDot, float flDist);
	void BuildScheduleTestBits(void);
	EHANDLE m_hDead;


	// This is a dummy field. In order to provide save/restore
	// code in this file, we must have at least one field
	// for the code to operate on. Delete this field when
	// you are ready to do your own save/restore for this
	// character.
	int		m_iDeleteThisField;

	DEFINE_CUSTOM_AI;
};
LINK_ENTITY_TO_CLASS(npc_cremator, CNPC_Cremator);
//IMPLEMENT_CUSTOM_AI(npc_cremator, CNPC_Cremator);

BEGIN_DATADESC(CNPC_Cremator)
DEFINE_FIELD(m_hDead, FIELD_EHANDLE),
END_DATADESC()

enum
{
	SCHED_CREM_WANDER = LAST_SHARED_SCHEDULE,
	SCHED_CREM_ELOF,
	SCHED_CREM_RANGE_ATTACK1,
};