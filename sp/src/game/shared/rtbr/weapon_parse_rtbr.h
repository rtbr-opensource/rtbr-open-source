#ifndef WEAPON_PARSE_RTBR_H
#define WEAPON_PARSE_RTBR_H

#include "weapon_parse.h"

class CRTBRWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CRTBRWeaponInfo, FileWeaponInfo_t );

	void Parse( KeyValues *pKeyValuesData, const char *szWeaponName ) OVERRIDE;

	float m_flFireRate;
	float m_flFireRateScoped;
};

#endif // WEAPON_PARSE_RTBR_H