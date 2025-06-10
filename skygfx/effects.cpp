#include <externs.h>
#include "include/renderqueue.h"

/* Variables */
bool g_bFixSandstorm = true;
bool g_bFixFog = true;
bool g_bExtendRainSplashes = true;
bool g_bMissingEffects = true;

/* Configs */

/* Hooks */
DECL_HOOKv(WaterCannons_Render)
{
    WaterCannons_Render();

    RenderWaterFog();
    RenderMovingFog();
    RenderVolumetricClouds();
}
DECL_HOOKv(PostFX_Render)
{
    RwCameraEndUpdate(Scene->camera);
    //ERQ_RenderFast(*pRasterFrontBuffer);
    RsCameraBeginUpdate(Scene->camera);

    PostFX_Render();

    SpeedFX( FindPlayerSpeed(-1)->Magnitude() );
}

/* Functions */

/* Main */
void StartEffectsStuff()
{
    g_bFixSandstorm = cfg->GetBool("FixSandstormIntensity", g_bFixSandstorm, "Effects");
    g_bFixFog = cfg->GetBool("FixFogIntensity", g_bFixFog, "Effects");
    g_bExtendRainSplashes = cfg->GetBool("ExtendedRainSplashes", g_bExtendRainSplashes, "Effects");
    g_bMissingEffects = cfg->GetBool("MissingPCEffects", g_bMissingEffects, "Effects");

    if(g_bFixSandstorm)
    {
      #ifdef AML32
        aml->Write16(pGTASA + 0x5CE16C, 0xE00C);
      #else
        aml->Write32(pGTASA + 0x6F26F8, 0x1400000A);
      #endif
    }

    if(g_bFixFog)
    {
        aml->PlaceNOP(pGTASA + BYBIT(0x5CDB90, 0x6F2204), 1);
    }

    if(g_bExtendRainSplashes)
    {
      #ifdef AML32
        aml->Write32(pGTASA + 0x5CD9E0, 0x0000E9CD);
      #else
        aml->Write32(pGTASA + 0x6F2070, 0x52800024);
      #endif
    }

    if(g_bMissingEffects)
    {
        HOOK(WaterCannons_Render, aml->GetSym(hGTASA, "_ZN13CWaterCannons6RenderEv"));
    }

    // Cannot add PostEffects buffer, yet.
    //HOOK(PostFX_Render, aml->GetSym(hGTASA, "_ZN12CPostEffects12MobileRenderEv"));
}