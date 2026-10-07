//=============================================================================
//
// Purpose: Wasteland Scanner
//
//=============================================================================

#include "cbase.h"
#include "c_ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_NPC_WastelandScanner : public C_AI_BaseNPC 
{
	DECLARE_CLASS(C_NPC_WastelandScanner, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();

public:
	virtual void OnDataChanged(DataUpdateType_t type);
	virtual void Precache(void);
	virtual void Spawn(void);

private:
	bool m_bIsBeaming = false;
	EHANDLE m_hTarget;
	CNewParticleEffect *m_hBeam;
};

//---------------------------------------------------------
// Networking.
//---------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT(C_NPC_WastelandScanner, DT_NPC_WastelandScanner, CNPC_WastelandScanner)
RecvPropBool(RECVINFO(m_bIsBeaming)), 
RecvPropEHandle(RECVINFO(m_hTarget)),
END_RECV_TABLE()

void C_NPC_WastelandScanner::Spawn(void)
{
	BaseClass::Spawn();
	m_hTarget = NULL;
}

//---------------------------------------------------------
// Precache.
//---------------------------------------------------------
void C_NPC_WastelandScanner::Precache(void) 
{
	// Particles
	PrecacheParticleSystem( "npc_wasteland_scanner_beam" );
	PrecacheParticleSystem( "weapon_gauss_beam" );
	PrecacheParticleSystem( "weapon_cguard_chargelaser" );
	BaseClass::Precache();
}

//---------------------------------------------------------
// Handle state changes from the server.
//---------------------------------------------------------
void C_NPC_WastelandScanner::OnDataChanged(DataUpdateType_t type) 
{
	BaseClass::OnDataChanged(type);

	if (m_hTarget != NULL) 
	{
		if (!m_hBeam)
		{
			m_hBeam = ParticleProp()->Create("npc_wasteland_scanner_beam", PATTACH_ABSORIGIN_FOLLOW);
			if (!m_hBeam)
				return;

			ParticleProp()->AddControlPoint( m_hBeam, 0, this, PATTACH_POINT_FOLLOW, "eyes" );
		}

		Vector vPosition;
		if (!m_hTarget->MyCombatCharacterPointer()->GetAttachment("eyes", vPosition))
		{
			vPosition = m_hTarget->WorldSpaceCenter();
		}
		ParticleProp()->AddControlPoint( m_hBeam, 1, NULL, PATTACH_WORLDORIGIN, 0, vPosition );
	}
	else if (m_hTarget == NULL && m_hBeam) 
	{
		ParticleProp()->StopEmission(m_hBeam);
		m_hBeam = NULL;
	}
}
