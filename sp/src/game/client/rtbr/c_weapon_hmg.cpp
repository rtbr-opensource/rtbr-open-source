//========= Copyright (c) RTBR Team, 2022 ============//
//
// Purpose: The HMG, the gun for suppressive fire.
//
//====================================================//

#include "cbase.h"
#include "c_basehlcombatweapon.h"
#include "c_baseplayer.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "c_weapon__stubs.h"

class C_WeaponHMG : public C_HLSelectFireMachineGun
{
	DECLARE_CLASS( C_WeaponHMG, C_HLSelectFireMachineGun );

public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	virtual void OnDataChanged( DataUpdateType_t updateType );

	void EnableScope();

	void DisableScope();

	bool m_bIsScoped;

};

STUB_WEAPON_CLASS_IMPLEMENT(weapon_hmg, C_WeaponHMG);

IMPLEMENT_CLIENTCLASS_DT( C_WeaponHMG, DT_WeaponHMG, CWeaponHMG )
RecvPropBool( RECVINFO( m_bIsScoped ) ),
END_RECV_TABLE()

void C_WeaponHMG::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	if (m_bIsScoped) {
		EnableScope();
	}
	else {
		DisableScope();
	}
}

void C_WeaponHMG::EnableScope() {
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	pPlayer->SetFX( SFX_HMG, true );
}
void C_WeaponHMG::DisableScope() {
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	pPlayer->SetFX( SFX_HMG, false );
}

class C_WeaponHMG2 : public C_WeaponHMG {
	DECLARE_CLASS( C_WeaponHMG2, C_WeaponHMG );
	DECLARE_CLIENTCLASS()
public:
	DECLARE_PREDICTABLE();
};