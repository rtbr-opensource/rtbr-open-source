#include "cbase.h"

#include <KeyValues.h>
#include "weapon_parse_rtbr.h"

// membdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

FileWeaponInfo_t *CreateWeaponInfo()
{
	return new CRTBRWeaponInfo;
}

void CRTBRWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_flFireRate = pKeyValuesData->GetFloat( "FireRate", 1.0f );
	m_flFireRateScoped = pKeyValuesData->GetFloat( "FireRateScoped", 1.0f );
}