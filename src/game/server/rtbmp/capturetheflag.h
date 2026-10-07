//=============== Copyright RTBMP Team, All rights reserved. ==================//
//
// Purpose: CTF gamemode and entity logic.
//
// $NoKeywords: $
//=============================================================================//

#ifndef CAPTURETHEFLAG_H
#define CAPTURETHEFLAG_H
#pragma once

#include "cbase.h"
#include "triggers.h"
#include "props.h"

enum FlagState
{
	// at its original location
	FLAG_HOME = 0,
	FLAG_PICKEDUP = 1,
	// flag carrier died, dropped on ground
	FLAG_DROPPED = 2,
};

class CCaptureZone : public CBaseTrigger
{
public:
	DECLARE_CLASS( CCaptureZone, CBaseTrigger );
	DECLARE_DATADESC();

	CCaptureZone();

	virtual void Spawn();
	virtual void StartTouch( CBaseEntity *pOther );

private:
	COutputEvent m_OnCapture;
	COutputEvent m_OnCapTeam1;
	COutputEvent m_OnCapTeam2;
};

class CCaptureFlag : public CDynamicProp
{
public:
	DECLARE_CLASS( CCaptureFlag, CDynamicProp );
	DECLARE_DATADESC();

	CCaptureFlag();

	virtual void Spawn( void );
	virtual void Precache( void );

	void FlagTouch( CBaseEntity *pOther );

	void OnFlagPickedUp( void );
	void OnFlagDropped( void );
	void OnFlagCaptured( void );
	void ReturnFlag();
	void ResetFlag();
	void InputForceDrop( inputdata_t &inputdata );
	void InputForceReturn( inputdata_t &inputdata ) { ReturnFlag(); }

	int GetFlagState( void ) { return m_iFlagState; }

	const char *GetTeamString();

private:
	int m_iReturnTime;
	bool m_bShotClockMode;

	int m_iFlagState;

	Vector m_vecOriginalPosition;
	QAngle m_angOriginalAngles;

	COutputEvent m_OnReturn;
	COutputEvent m_OnPickup;
	COutputEvent m_OnPickupTeam1;
	COutputEvent m_OnPickupTeam2;
	COutputEvent m_OnDrop;
	COutputEvent m_OnCapture;
	COutputEvent m_OnCapTeam1;
	COutputEvent m_OnCapTeam2;
	COutputEvent m_OnTouchSameTeam;
};
#endif