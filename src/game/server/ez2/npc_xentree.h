//=//=============================================================================//
//
// Purpose:		Plant-like Xen creature that swings a cruel scythe-like
//				appendage to defend itself.
//
//				These things really made me nervous when I first played Half-Life.
//
// Author:		1upD
//
//=============================================================================//


#include "ai_basenpc.h"
#include "npc_baseflora.h"

class CNPC_XenTree : public CNPC_BaseFlora
{
	DECLARE_CLASS( CNPC_XenTree, CNPC_BaseFlora );

public:
	void Spawn();
	void Precache( void );

	void OnChangeActivity( Activity eNewActivity );
	void HandleAnimEvent( animevent_t *pEvent );
	
	virtual bool CanBoogie() OVERRIDE { return false; }

protected:

	int		StimulusMask() {
		return /*bits_REACT_XENFLORA_ENTITY_APPROACH |*/
			bits_REACT_XENFLORA_HURT |
			/*bits_REACT_XENFLORA_HEAR_DANGER |*/
			bits_REACT_XENFLORA_NEARBY_GUNSHOT |
			bits_REACT_XENFLORA_ENEMY_IN_HIT_RANGE;
	}

	float GetViewDistance() { return 160.0f; }
	float GetFieldOfView() { return 0.6f; }

private:
};