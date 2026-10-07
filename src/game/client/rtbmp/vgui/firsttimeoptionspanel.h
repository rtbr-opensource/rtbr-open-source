//========= Copyright RTBA Team, All rights reserved. ============//
//
// Purpose:	First time loading options menu for RTB : A.
//
//================================================================//
#ifndef FIRSTTIMEOPTIONSPANEL_H
#define FIRSTTIMEOPTIONSPANEL_H

class IFirstTimeOptionsMenu
{
public:
	virtual void Create( vgui::VPANEL parent ) = 0;

	virtual void Destroy( void ) = 0;

	virtual void Activate( void ) = 0;

	virtual void Open( void ) = 0;
};

#endif //FIRSTTIMEOPTIONSPANEL_H