//========= Copyright RTBA Team, All rights reserved. ============//
//
// Purpose:	New listen server creation menu for RTB:A.
//
//================================================================//

class ICreateServerPanel
{
public:
	virtual void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;
	virtual void		Activate( void ) = 0;
	virtual void		Open( void ) = 0;
};

extern ICreateServerPanel *createserverpanel;