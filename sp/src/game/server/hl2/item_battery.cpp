//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Handling for the suit batteries.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CItemBattery : public CItem
{
public:
	DECLARE_CLASS( CItemBattery, CItem );

	void Spawn( void )
	{ 
		Precache( );
		if (FClassnameIs(this, "item_battery")) {
			SetModel("models/items/battery.mdl");
		}
		else {
			SetModel("models/props_eli/dayone_battery.mdl");
		}
		
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		PrecacheModel ("models/items/battery.mdl");
		PrecacheModel("models/props_eli/dayone_battery.mdl");

		PrecacheScriptSound( "ItemBattery.Touch" );
		PrecacheScriptSound("DayOne_Battery.Pickup");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player *>( pPlayer );
		if (FClassnameIs(this, "item_battery")) {
			return (pHL2Player && pHL2Player->ApplyBattery());
		}
		else {
			pHL2Player->IncrementArmorValue(1);
			EmitSound("DayOne_Battery.Pickup");
			return (true);
		}
	}

#ifdef MAPBASE
	void	InputSetPowerMultiplier( inputdata_t &inputdata ) { m_flPowerMultiplier = inputdata.value.Float(); }
	float	m_flPowerMultiplier = 1.0f;

	DECLARE_DATADESC();
#endif
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);
PRECACHE_REGISTER(item_battery);
LINK_ENTITY_TO_CLASS(item_battery_old, CItemBattery);
PRECACHE_REGISTER(item_battery_old);

#ifdef MAPBASE
BEGIN_DATADESC( CItemBattery )

	DEFINE_KEYFIELD( m_flPowerMultiplier, FIELD_FLOAT, "PowerMultiplier" ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPowerMultiplier", InputSetPowerMultiplier ),

END_DATADESC()
#endif

