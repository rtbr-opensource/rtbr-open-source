//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Definition for client-side cremator.
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
	
	void	Spawn( void );
	void	Precache( void );
	// Server to client message received
	virtual void	ReceiveMessage(int classID, bf_read& msg);
	virtual void	OnDataChanged( DataUpdateType_t updateType );

private:

	void StartImmoFX();
	void StopImmoFX();

	void StartTankFX();
	void StopTankFX();

	void UpdateImmolatorFlame();

	Vector m_vMuzzlePosition;
	Vector m_vAiming;

	CNewParticleEffect* m_hMuzzle;
	CNewParticleEffect* m_hFlame;
};

IMPLEMENT_CLIENTCLASS_DT( C_NPC_Cremator, DT_NPC_Cremator, CNPC_Cremator )
RecvPropVector( RECVINFO( m_vMuzzlePosition ) ),
RecvPropVector( RECVINFO( m_vAiming ) ),
END_RECV_TABLE()

void C_NPC_Cremator::Spawn()
{
	Precache();
	BaseClass::Spawn();
}

void C_NPC_Cremator::Precache( void )
{
	PrecacheParticleSystem( "weapon_immolator_muzzle_TP" );
	PrecacheParticleSystem( "weapon_immolator_flame_TP" );
	BaseClass::Precache();
}

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

void C_NPC_Cremator::OnDataChanged( DataUpdateType_t updateType )
{
	if (m_hMuzzle && m_hFlame)
		UpdateImmolatorFlame();
	BaseClass::OnDataChanged(updateType);
}

//-----------------------------------------------------------------------------
// Create muzzle FX
//-----------------------------------------------------------------------------
void C_NPC_Cremator::StartImmoFX()
{
	if (m_hMuzzle && m_hFlame) {
		return;
	}

	m_hMuzzle = ParticleProp()->Create("weapon_immolator_muzzle_TP", PATTACH_ABSORIGIN_FOLLOW);
	m_hFlame = ParticleProp()->Create( "weapon_immolator_flame_TP", PATTACH_CUSTOMORIGIN, 0, m_vMuzzlePosition);

	Assert( m_hMuzzle && m_hFlame );
	if ( !m_hMuzzle || !m_hFlame ) return;

	ParticleProp()->AddControlPoint(m_hMuzzle, 0, this, PATTACH_POINT_FOLLOW, "1");
	UpdateImmolatorFlame();
}


//-----------------------------------------------------------------------------
// terminate muzzle FX
//-----------------------------------------------------------------------------
void C_NPC_Cremator::StopImmoFX()
{
	ParticleProp()->StopEmission( m_hMuzzle );
	ParticleProp()->StopEmission( m_hFlame );
	m_hMuzzle = NULL;
	m_hFlame = NULL;
}

//-----------------------------------------------------------------------------
// Create tank FX
//-----------------------------------------------------------------------------
void C_NPC_Cremator::StartTankFX()
{

	if (ParticleProp()->FindEffect("npc_cremator_tankjet") >= 0) {
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

//-----------------------------------------------------------------------------
// Purpose: Update the immolator's flame stream particle effect.
//-----------------------------------------------------------------------------
void C_NPC_Cremator::UpdateImmolatorFlame()
{
	m_hFlame->SetControlPoint( 0, m_vMuzzlePosition );
	QAngle angAiming;
	VectorAngles( m_vAiming, angAiming );
	Quaternion qAiming;
	AngleQuaternion( angAiming, qAiming );
	m_hFlame->SetControlPointOrientation( 0, qAiming );
}
