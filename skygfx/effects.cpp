#include <externs.h>
#include "include/renderqueue.h"

/* Variables */
bool g_bFixSandstorm = true;
bool g_bFixFog = true;
bool g_bExtendRainSplashes = true;
bool g_bMissingEffects = true;
bool g_bMissingPostEffects = true;
bool g_bRenderGrain = true;

int g_nGrainRainStrength = 0;
int g_nInitialGrain = 0;
RwRaster* pGrainRaster = NULL;

int postfxX = 0, postfxY = 0;
RwRaster* pSkyGFXPostFXRaster = NULL;

/* Configs */
ConfigEntry *pCFGRenderGrain;

/* Functions */
void RenderGrainSettingChanged(int oldVal, int newVal, void* data)
{
    if(oldVal == newVal) return;

    pCFGRenderGrain->SetBool(newVal != 0);
    g_bRenderGrain = pCFGRenderGrain->GetBool();

    cfg->Save();
}
void CreateGrainTexture()
{
    if(!pGrainRaster) return;

    RwUInt8 *pixels = RwRasterLock(pGrainRaster, 0, 1);
    vrinit(rand());
    int x = vrget(), pixSize = pGrainRaster->width * pGrainRaster->height;
    for(int i = 0; i < pixSize; ++i)
    {
        *(pixels++) = x;
        *(pixels++) = x;
        *(pixels++) = x;
        *(pixels++) = 80;
        x = vrnext();
    }
    RwRasterUnlock(pGrainRaster);
}
void RenderGrainEffect(uint8_t strength)
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    
    float uOffset = (float)rand() / (float)RAND_MAX;
    float vOffset = (float)rand() / (float)RAND_MAX;

    // Optimised algorithm. We dont need to create it every frame.
    float umin = uOffset;
    float vmin = vOffset;
    float umax = (1.5f * RsGlobal->maximumWidth) / 640.0f + uOffset;
    float vmax = (1.5f * RsGlobal->maximumHeight) / 640.0f + vOffset;
    
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    
    PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 255, 255, 255, strength, pGrainRaster);
    
    DrawQuadSetDefaultUVs();
    ImmediateModeRenderStatesReStore();
}
void GFX_GrabScreen()
{
    if(postfxX != *renderWidth || postfxY != *renderHeight)
    {
        postfxX = *renderWidth;
        postfxY = *renderHeight;

        if(pSkyGFXPostFXRaster) RwRasterDestroy(pSkyGFXPostFXRaster);
        pSkyGFXPostFXRaster = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMATDEFAULT);
    }

    if(pSkyGFXPostFXRaster)
    {
        ERQ_GrabFramebuffer(pSkyGFXPostFXRaster);
    #ifdef GPU_GRABBER
        emu_glBegin(5u);
        emu_glVertex3f(-1.0, 1.0, *gradeBlur);
        emu_glTexCoord2f(0.0, 1.0);
        emu_glVertex3f(1.0, 1.0, *gradeBlur);
        emu_glTexCoord2f(1.0, 1.0);
        emu_glVertex3f(-1.0, -1.0, *gradeBlur);
        emu_glTexCoord2f(0.0, 0.0);
        emu_glVertex3f(1.0, -1.0, *gradeBlur);
        emu_glTexCoord2f(1.0, 0.0);
        emu_glEnd();
    #endif
        ERQ_GrabFramebufferPost();
    }
}
void GFX_CCTV() // Completed
{
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)(true));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)(false));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)(true));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)(false));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pSkyGFXPostFXRaster);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)(true));
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)(rwBLENDSRCCOLOR));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)(rwBLENDINVSRCCOLOR));
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    uint32 lineHeight  = (uint32)(2.0f * ((float)RsGlobal->maximumHeight) / 448.0f);
    uint32 linePadding = 2 * lineHeight;
    uint32 numLines    = RsGlobal->maximumHeight / linePadding;
    for (auto i = 0u, y = 0u; i < numLines; i++, y += linePadding)
    {
        float umin = 0;
        float vmin = y / (float)RsGlobal->maximumHeight;
        float umax = 1.0f;
        float vmax = (y + lineHeight) / (float)RsGlobal->maximumHeight;
        
        DrawQuadSetUVs(umin, -vmin, umax, -vmin, umax, -vmax, umin, -vmax);
        PostEffectsDrawQuad(0.0f, y, RsGlobal->maximumWidth, lineHeight, 0, 64, 0, 255, pSkyGFXPostFXRaster);
    }

    ImmediateModeRenderStatesReStore();
}
void GFX_SpeedFX(float speed)
{
    fxSpeedSettings* fx = NULL;
    for(int i = 6; i >= 0; --i)
    {
        if(speed >= FX_SPEED_VARS[i].fSpeedThreshHold)
        {
            fx = &FX_SPEED_VARS[i];
            break;
        }
    }
    if(!fx) return;

#ifdef AML32
    int DirectionWasLooking = TheCamera->m_apCams[TheCamera->m_nCurrentActiveCam].DirectionWasLooking;
#else
    int DirectionWasLooking = TheCamera->Cams[TheCamera->ActiveCam].DirectionWasLooking;
#endif

    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)(rwBLENDSRCCOLOR));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)(rwBLENDINVSRCCOLOR));

    for(int i = 0; i < fx->nLoops; ++i)
    {
        float uOffset = 4.0f * ((float)rand() / (float)RAND_MAX) / (float)RsGlobal->maximumWidth;
        float vOffset = 4.0f * ((float)rand() / (float)RAND_MAX) / (float)RsGlobal->maximumHeight;
        float umin = -uOffset;
        float vmin = -vOffset;
        float umax = 1.0f + uOffset;
        float vmax = 1.0f + vOffset
        
        DrawQuadSetUVs(umin, vmax, umax, vmax, umax, vmin, umin, vmin);
        PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 255, 255, 255, 36, pSkyGFXPostFXRaster);
    }
    ImmediateModeRenderStatesReStore();
}

/* Hooks */
DECL_HOOKv(WaterCannons_Render)
{
    WaterCannons_Render();

    RenderWaterFog();
    RenderMovingFog();
    RenderVolumetricClouds();
}
DECL_HOOKv(PostFX_Init)
{
    if(pGrainRaster)
    {
        RwRasterDestroy(pGrainRaster);
        pGrainRaster = NULL;
    }
    pGrainRaster = RwRasterCreate(64, 64, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
    if(pGrainRaster) CreateGrainTexture();

    *pbFog = false;
}
DECL_HOOKv(PostFX_Render)
{
    GFX_GrabScreen();

    if(*pbFog) RenderScreenFogPostEffect();

    PostFX_Render();

    //GFX_CCTV();
    GFX_SpeedFX(FindPlayerSpeed(-1)->Magnitude());

    if(g_bRenderGrain && pGrainRaster)
    {
        if(!*pbInCutscene)
        {
            if(*pbNightVision) RenderGrainEffect(GRAIN_NIGHTVISION_STRENGTH);
            else if(*pbInfraredVision) RenderGrainEffect(GRAIN_INFRAREDVISION_STRENGTH);
        }
        if(*pbRainEnable || g_nGrainRainStrength)
        {
            uint8_t nTargetStrength = (uint8_t)(*pfWeatherRain * 128.0f);
                 if(g_nGrainRainStrength < nTargetStrength) ++g_nGrainRainStrength;
            else if(g_nGrainRainStrength > nTargetStrength) --g_nGrainRainStrength;

            if(!CamNoRain() && !PlayerNoRain() && *pfWeatherUnderwaterness <= 0.0f && !*currArea)
            {
                CVector& cameraPos = TheCamera->GetPosition();
                if(cameraPos.z <= 900.0f) RenderGrainEffect(g_nGrainRainStrength / 4);
            }
        }
        if(*pbGrainEnable) RenderGrainEffect(*pnGrainStrength);
    }
}
DECL_HOOKv(ShowGameBuffer)
{
    ShowGameBuffer();
    GFX_GrabScreen();
}

/* Main */
void StartEffectsStuff()
{
    g_bFixSandstorm = cfg->GetBool("FixSandstormIntensity", g_bFixSandstorm, "Effects");
    g_bFixFog = cfg->GetBool("FixFogIntensity", g_bFixFog, "Effects");
    g_bExtendRainSplashes = cfg->GetBool("ExtendedRainSplashes", g_bExtendRainSplashes, "Effects");
    g_bMissingEffects = cfg->GetBool("MissingPCEffects", g_bMissingEffects, "Effects");
    g_bMissingPostEffects = cfg->GetBool("MissingPCPostEffects", g_bMissingPostEffects, "Effects");

    pCFGRenderGrain = cfg->Bind("RenderGrainEffect", g_bRenderGrain, "Effects");
    AddSetting("Grain Effect", g_bRenderGrain, 0, sizeofA(aYesNo)-1, aYesNo, RenderGrainSettingChanged, NULL);

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
        aml->PlaceNOP4(pGTASA + BYBIT(0x5CCB98, 0x6F16B4), 1); // CWeather::WaterFogFXControl, dont mult by 1.4
        aml->WriteFloat(aml->GetSym(hGTASA, "_ZN11CWaterLevel17m_fWaterFogHeightE"), 7.0f); // lower the WaterFog max height
    }

    if(g_bMissingPostEffects)
    {
        HOOK(PostFX_Init,   aml->GetSym(hGTASA, "_ZN12CPostEffects21SetupBackBufferVertexEv"));
        HOOK(PostFX_Render, aml->GetSym(hGTASA, "_ZN12CPostEffects12MobileRenderEv"));
        //HOOKBLX(ShowGameBuffer, pGTASA + BYBIT(0x1BC5D4 + 0x1, 0x24F994));
    }
}
