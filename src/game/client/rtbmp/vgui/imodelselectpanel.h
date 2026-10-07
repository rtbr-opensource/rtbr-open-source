//========= Copyright RTBA Team, All rights reserved. ============//
//
// Purpose:	Model selection menu for RTB:A.
//
//================================================================//

class IModelSelectPanel
{
public:
	virtual void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;
	virtual void		Activate( void ) = 0;
	virtual void		Open( void ) = 0;
};

extern IModelSelectPanel *modelselectpanel;