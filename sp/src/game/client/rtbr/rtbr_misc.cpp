#include "cbase.h"

#define BUILDDATE __DATE__
#define BUILDTIME __TIME__
#ifdef RTBR_RELEASE
#define BUILDTYPE "Release"
#endif

#ifdef LINUX
#define BUILDPLAT "Linux"
#else
#define BUILDPLAT "Windows"
#endif

void RTBRBuild_CC(const CCommand &args)
{
	Msg(VarArgs("%s %s (%s %s) \n", BUILDDATE, BUILDTIME, BUILDPLAT, BUILDTYPE));
}

ConCommand rtbr_build("rtbr_build", RTBRBuild_CC, "Displays build date for Raising the Bar: Redux", 0);
