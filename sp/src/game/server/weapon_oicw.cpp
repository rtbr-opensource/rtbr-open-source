//========= Copyright (c) RTBR Team, 2018 ============//

#include "cbase.h"
#include "basehlcombatweapon.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponOICW : public CHLSelectFireMachineGun {

public:
	DECLARE_CLASS(CWeaponOICW, CHLSelectFireMachineGun);
	CWeaponOICW();
};

LINK_ENTITY_TO_CLASS(weapon_oicw, CWeaponOICW);
PRECACHE_WEAPON_REGISTER(weapon_oicw);


//-----------------------------------------------------------------------------
// Purpose: Set some OICW variables
//-----------------------------------------------------------------------------
CWeaponOICW::CWeaponOICW() {

}
