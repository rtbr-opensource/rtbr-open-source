
#include "cbase.h"
#include "func_break.h"
 
// memdbgon must be the last include file in a .cpp file!!! 
#include "tier0/memdbgon.h"

class CFuncDamageProxy : public CBreakable
{
	DECLARE_DATADESC();
	DECLARE_CLASS(CFuncDamageProxy, CBreakable);

public:

	virtual int OnTakeDamage(const CTakeDamageInfo& info);
	virtual void Spawn();

	EHANDLE m_hDamageRecipient;
};
LINK_ENTITY_TO_CLASS(func_damageproxy, CFuncDamageProxy);

BEGIN_DATADESC(CFuncDamageProxy)
DEFINE_KEYFIELD(m_hDamageRecipient, FIELD_EHANDLE, "dmgrecipient")
END_DATADESC()

void CFuncDamageProxy::Spawn() {
	BaseClass::Spawn();
}

int CFuncDamageProxy::OnTakeDamage(const CTakeDamageInfo& info) {

	CBaseEntity* pRecipient = m_hDamageRecipient.Get();
	if (pRecipient != NULL) {
		pRecipient->TakeDamage(info);
	}
	return 0;
	//return BaseClass::OnTakeDamage(info);
}