#include "cbase.h"

#define BUILDDATE __DATE__
#define BUILDTIME __TIME__

#ifdef RTBR_RELEASE
#ifndef RTBR_PLAYTEST
#define BUILDTYPE "Release"
#endif
#endif

#ifdef RTBR_PLAYTEST
#define BUILDTYPE "Playtest"
#endif

void RTBRBuild_CC(const CCommand &args)
{

	Msg(VarArgs("%s %s (%s) \n", BUILDDATE, BUILDTIME, BUILDTYPE));
}

ConCommand rtbr_build("rtbr_build", RTBRBuild_CC, "Displays build date for Raising the Bar: Redux", 0);