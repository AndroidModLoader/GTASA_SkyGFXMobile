#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#ifdef AML32
    #include "GTASA_STRUCTS.h"
    #define BYVER(__for32, __for64) (__for32)
#else
    #include "GTASA_STRUCTS_210.h"
    #define BYVER(__for32, __for64) (__for64)
#endif
#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

// SAUtils
#if __has_include(<isautils.h>)
    #include <isautils.h>
    ISAUtils* sautils = NULL;
#endif

#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))
MYMODCFG(net.rusjj.skygfxmobile, SkyGfx Mobile Beta, 0.3, aap & TheOfficialFloW & RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.0.2.1)
END_DEPLIST()

/* Config */

/* Saves */
void* hGTASA;
uintptr_t pGTASA;

extern "C" void OnModLoad()
{
    /* Logging */
    logger->SetTag("SkyGFX Mobile");

    /* Libraries */
    hGTASA = aml->GetLibHandle("libGTASA.so");
    pGTASA = aml->GetLib("libGTASA.so");
    sautils = (ISAUtils*)GetInterface("SAUtils");
}