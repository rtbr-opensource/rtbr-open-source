//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Definition for client-side advisor.
//
//=====================================================================================//



#include "cbase.h"
#include "npc_cremator_shared.h"

#include "particles_simple.h"
#include "particles_attractor.h"
#include "clienteffectprecachesystem.h"
#include "c_te_effect_dispatch.h"
#include "fx.h"
#include "particles_localspace.h"
#include "view.h"
#include "particles_new.h"
#include "c_ai_basenpc.h"
#include "iefx.h"

/*! Client-side reflection of the cremator class.
 */
class C_NPC_Cremator : public C_AI_BaseNPC
{
	DECLARE_CLASS(C_NPC_Cremator, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();

public:
	// Server to client message received
	virtual void	ReceiveMessage(int classID, bf_read& msg);

private:

	void StartImmoFX();
	void StopImmoFX();

	void StartTankFX();
	void StopTankFX();

};

IMPLEMENT_CLIENTCLASS_DT(C_NPC_Cremator, DT_NPC_Cremator, CNPC_Cremator)

END_RECV_TABLE()

// Server to client message received
void C_NPC_Cremator::ReceiveMessage(int classID, bf_read& msg)
{
	if (classID != GetClientClass()->m_ClassID)
	{
		// message is for subclass
		BaseClass::ReceiveMessage(classID, msg);
		return;
	}

	int messageType = msg.ReadByte();
	switch (messageType)
	{
	case CREMATOR_MSG_START_IMMO:
	{
		StartImmoFX();
	}
	break;

	case CREMATOR_MSG_STOP_IMMO:
	{
		StopImmoFX();

	}
	break;

	case CREMATOR_MSG_KILLED:
	{
		ParticleProp()->StopEmission();
	}
	break;
	case CREMATOR_MSG_START_TANK:
	{
		StartTankFX();
	}
	break;
	case CREMATOR_MSG_STOP_TANK:
	{
		StartTankFX();
	}
	break;

	default:
		AssertMsg1(false, "Received unknown message %d", messageType);
	}
}

//-----------------------------------------------------------------------------
// Create muzzle FX
//-----------------------------------------------------------------------------
void C_NPC_Cremator::StartImmoFX()
{

	if (ParticleProp()->FindEffect("immo_beam_muzzle02") == 0) {
		return;
	}

	CNewParticleEffect* pEffect = ParticleProp()->Create("immo_beam_muzzle02", PATTACH_ABSORIGIN_FOLLOW);

	Assert(pEffect);
	if (!pEffect) return;

	ParticleProp()->AddControlPoint(pEffect, 0, this, PATTACH_POINT_FOLLOW, "1");
}


//-----------------------------------------------------------------------------
// terminate muzzle FX
//-----------------------------------------------------------------------------
void C_NPC_Cremator::StopImmoFX()
{

	ParticleProp()->StopParticlesNamed("immo_beam_muzzle02");
}

//-----------------------------------------------------------------------------
// Create tank FX
//-----------------------------------------------------------------------------
void C_NPC_Cremator::StartTankFX()
{

	if (ParticleProp()->FindEffect("npc_cremator_tankjet") == 0) {
		return;
	}

	CNewParticleEffect* pEffect = ParticleProp()->Create("npc_cremator_tankjet", PATTACH_ABSORIGIN_FOLLOW);

	Assert(pEffect);
	if (!pEffect) return;

	ParticleProp()->AddControlPoint(pEffect, 0, this, PATTACH_POINT_FOLLOW, "tank");
}


//-----------------------------------------------------------------------------
// terminate tank FX
//-----------------------------------------------------------------------------
void C_NPC_Cremator::StopTankFX()
{

	ParticleProp()->StopParticlesNamed("npc_cremator_tankjet");
}
