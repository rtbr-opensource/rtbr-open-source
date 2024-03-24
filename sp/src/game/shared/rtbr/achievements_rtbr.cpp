
#include "cbase.h"

#include "baseachievement.h"
#include "basegrenade_shared.h"
#include "basehlcombatweapon_shared.h"
#include "ammodef.h"
#include "cbase.h"
#include "achievementmgr.h"

CAchievementMgr g_AchievementMgrEpisodic;	// global achievement mgr for episodic

class CAchievementRTBRKillCremator : public CBaseAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetVictimFilter("npc_cremator");
		SetGoal(2);
		SetMapNameFilter("rtbr_d1_consulplaza02");

	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		IncrementCount();
	}
};
DECLARE_ACHIEVEMENT(CAchievementRTBRKillCremator, ACHIEVEMENT_RTBR_KILL_CREMATOR, "RTBR_KILL_CREMATOR", 10);

class CAchievementKill20Elites : public CBaseAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetVictimFilter("npc_elitepolice");
		SetGoal(20);

	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		IncrementCount();
	}
};
DECLARE_ACHIEVEMENT(CAchievementKill20Elites, ACHIEVEMENT_RTBR_KILL_20_ELITES, "RTBR_KILL_20_ELITES", 10);

class CAchievementRTBRKillWithAlyxgun : public CBaseAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetVictimFilter("npc_combine_s");
		SetGoal(15);

	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		CBasePlayer* pPlayer = (CBasePlayer*)pAttacker;
		if (pPlayer != NULL) {
			CBaseCombatWeapon* pWeapon = pPlayer->GetActiveWeapon();
			if (pWeapon != NULL) {
				if (FClassnameIs(pWeapon, "weapon_alyxgun")) {
					IncrementCount();
				}
			}
		}


	}
};
DECLARE_ACHIEVEMENT(CAchievementRTBRKillWithAlyxgun, ACHIEVEMENT_RTBR_KILL_WITH_ALYXGUN, "RTBR_KILL_WITH_ALYXGUN", 10);


DECLARE_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_RTBR_SECRET_REVOLVER, "RTBR_SECRET_REVOLVER", 15);