#include "cbase.h"
#include "prop_vehiclegib.h"
#include "fmtstr.h"

BEGIN_DATADESC(CPropVehicleGib)
DEFINE_FIELD(m_iGibType, FIELD_INTEGER),
DEFINE_FIELD(m_iModelNumber, FIELD_INTEGER),
DEFINE_KEYFIELD(m_iGibType, FIELD_INTEGER, "gibtype"),
END_DATADESC();


LINK_ENTITY_TO_CLASS(prop_vehiclegib, CPropVehicleGib);

CPropVehicleGib::CPropVehicleGib() {
}

void CPropVehicleGib::Precache() {

	SetModelName(MAKE_STRING("models/gibs/generic_gibs/genericgib_01.mdl"));

	const char* numToStr[4] = { "1", "2", "3", "4" };

	switch (m_iGibType) {
		case VG_TYPE_GENERIC_GIB:
			for (int i = 0; i < 4; i++) {
				PrecacheModel((UTIL_VarArgs("models/gibs/generic_gibs/genericgib_0%s.mdl", numToStr[i])));
			}

			break;

		case VG_TYPE_APC1_GIB:
			for (int i = 0; i < 4; i++) {
				PrecacheModel((UTIL_VarArgs("models/gibs/apc1_gibs/apc1gib_0%s.mdl", numToStr[i])));
			}

			break;

		case VG_TYPE_APC2_GIB:
			for (int i = 0; i < 4; i++) {
				PrecacheModel(UTIL_VarArgs("models/gibs/apc2_gibs/apc2gib_0%s.mdl", numToStr[i]));
			}

			break;

		case VG_TYPE_CHOPPER_GIB:
			const char* chopperGibNames[6] = { "1", "2", "3", "4_cockpit", "5_tailfan", "6_body" };

			for (int i = 0; i < 6; i++) {
				PrecacheModel((UTIL_VarArgs("models/gibs/helicopter_brokenpiece_0%s.mdl", chopperGibNames[i])));
			}

			break;

	}

	BaseClass::Precache();
}

void CPropVehicleGib::Spawn() {

	Precache();

	const char* numToStr[4] = { "1", "2", "3", "4" };

	if (m_iModelNumber < 0 || (m_iGibType != 1 && m_iModelNumber >= 4) || (m_iGibType == 1 && m_iModelNumber >= 6)) {
		if (m_iGibType == VG_TYPE_CHOPPER_GIB) {
			m_iModelNumber = random->RandomInt(0, 5);
		}
		else {
			m_iModelNumber = random->RandomInt(0, 3);
		}
	}

	//DevMsg("%i\n", m_iModelNumber);

	switch (m_iGibType) {
	case VG_TYPE_GENERIC_GIB:
		SetModel(UTIL_VarArgs("models/gibs/generic_gibs/genericgib_0%s.mdl", numToStr[m_iModelNumber]));
		break;
	case VG_TYPE_APC1_GIB:
		SetModel(UTIL_VarArgs("models/gibs/apc1_gibs/apc1gib_0%s.mdl", numToStr[m_iModelNumber]));
		break;
	case VG_TYPE_APC2_GIB:
		SetModel(UTIL_VarArgs("models/gibs/apc2_gibs/apc2gib_0%s.mdl", numToStr[m_iModelNumber]));
		break;
	case VG_TYPE_CHOPPER_GIB:
		const char* chopperGibNames[6] = { "1", "2", "3", "4_cockpit", "5_tailfan", "6_body" };

		SetModel((UTIL_VarArgs("models/gibs/helicopter_brokenpiece_0%s.mdl", chopperGibNames[m_iModelNumber])));
		break;
	}
	
	BaseClass::Spawn();
}
