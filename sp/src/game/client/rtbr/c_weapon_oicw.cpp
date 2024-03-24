#include "cbase.h"
#include "c_basehlcombatweapon.h"
#include "c_weapon__stubs.h"

class C_WeaponOICW : public C_HLSelectFireMachineGun
{
	DECLARE_CLASS(C_WeaponOICW, C_HLSelectFireMachineGun);

public:
	DECLARE_PREDICTABLE();
};

STUB_WEAPON_CLASS_IMPLEMENT(weapon_oicw, C_WeaponOICW);