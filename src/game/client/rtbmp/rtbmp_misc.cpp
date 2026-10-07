#include "cbase.h"

#define BUILDDATE __DATE__
#define BUILDTIME __TIME__

#ifdef LINUX
#define BUILDPLAT "Linux"
#else
#define BUILDPLAT "Windows"
#endif

void RTBMPBuild_CC( const CCommand &args )
{

	Msg( VarArgs("%s %s (%s)\n", BUILDDATE, BUILDTIME, BUILDPLAT) );
}

ConCommand rtba_build( "rtba_build", RTBMPBuild_CC, "Displays build date for Raising the Bar: Multiplayer", 0 );