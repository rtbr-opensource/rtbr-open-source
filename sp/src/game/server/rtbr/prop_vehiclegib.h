#include "cbase.h"
#include "props.h"

class CPropVehicleGib : public CPhysicsProp  {
	
public:
	DECLARE_CLASS(CPropVehicleGib, CPhysicsProp);
	//DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CPropVehicleGib();

	void SetGibType(int gibType) { m_iGibType = gibType; };
	void SetGibModel(int modelNum) { m_iModelNumber = modelNum; };

	int GetGibType() { return m_iGibType; };
	int GetGibModel() { return m_iModelNumber; };

	virtual void Spawn();

private:

	virtual void Precache(void);

	int m_iGibType = 0;

	int m_iModelNumber = 0;
};

enum GibTypes {
	VG_TYPE_GENERIC_GIB = 0,
	VG_TYPE_CHOPPER_GIB = 1,
	VG_TYPE_APC1_GIB = 2,
	VG_TYPE_APC2_GIB = 3,
};

enum APC1GibModels {
	VG_APC1_01 = 1,
	VG_APC1_02 = 2,
	VG_APC1_03 = 3,
	VG_APC1_04 = 4,
};

enum APC2GibModels {
	VG_APC2_01 = 1,
	VG_APC2_02 = 2,
	VG_APC2_03 = 3,
	VG_APC2_04 = 4,
};

enum ChopperGibModels {
	VG_CHOPPER_01 = 0,
	VG_CHOPPER_02 = 1,
	VG_CHOPPER_03 = 2,
	VG_CHOPPER_04_COCKPIT = 3,
	VG_CHOPPER_05_TAIL = 4,
	VG_CHOPPER_06_BODY = 5,
};

enum GenericGibModels {
	VG_GENERIC_01 = 1,
	VG_GENERIC_02 = 2,
	VG_GENERIC_03 = 3,
	VG_GENERIC_04 = 4,
};