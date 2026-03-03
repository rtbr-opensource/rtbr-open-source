//========= Copyright (c) RTBR Team, 2018 ============//
//
// Purpose: OICW, the gun that scopes and shoots grenades
//
//====================================================//

#include "cbase.h"
#include "c_basehlcombatweapon.h"
#include "c_baseplayer.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "c_weapon__stubs.h"

class C_WeaponOICW : public C_HLSelectFireMachineGun
{
	DECLARE_CLASS(C_WeaponOICW, C_HLSelectFireMachineGun);

public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	virtual void OnDataChanged(DataUpdateType_t updateType);

	void EnableScope();

	void DisableScope();

	bool m_bIsScoped;

	bool m_bIsPlayer;
};

STUB_WEAPON_CLASS_IMPLEMENT(weapon_oicw, C_WeaponOICW);

IMPLEMENT_CLIENTCLASS_DT(C_WeaponOICW, DT_WeaponOICW, CWeaponOICW)
RecvPropBool(RECVINFO(m_bIsScoped)),
RecvPropBool(RECVINFO(m_bIsPlayer)),
END_RECV_TABLE()

void C_WeaponOICW::OnDataChanged(DataUpdateType_t updateType)
{
		BaseClass::OnDataChanged(updateType);
		if (m_bIsScoped && m_bIsPlayer) {
			EnableScope();
		}
		else if (!m_bIsScoped && m_bIsPlayer) {
			DisableScope();
		}
}

void C_WeaponOICW::EnableScope() {
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	pPlayer->SetFX(SFX_OICW, true);
}
void C_WeaponOICW::DisableScope() {
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	pPlayer->SetFX(SFX_OICW, false);
}