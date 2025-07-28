#include <externs.h>
#include "include/renderqueue.h"

/* Enums */
enum eSpeedFX : uint8_t
{
    SPFX_INACTIVE = 0,
    SPFX_TOGGLED_ON,
    SPFX_LIKE_ON_PC,
    SPFX_LIKE_ON_PS2,

    SPEEDFX_SETTINGS
};
enum eChromaticAberration : uint8_t
{
    CRAB_INACTIVE = 0,
    CRAB_ENABLED,
    CRAB_INTENSE,

    CRAB_SETTINGS
};

/* Variables */
bool g_bFixSandstorm = true;
bool g_bFixFog = true;
bool g_bExtendRainSplashes = true;
bool g_bMissingEffects = true;
bool g_bMissingPostEffects = true;
bool g_bRenderGrain = true;
int g_nSpeedFX = SPFX_LIKE_ON_PC;
int g_nCrAb = CRAB_INACTIVE;
int g_nVignette = 0;

int g_nGrainRainStrength = 0;
int g_nInitialGrain = 0;
RwRaster* pGrainRaster = NULL;

int postfxX = 0, postfxY = 0;
RwRaster *pSkyGFXPostFXRaster1 = NULL, *pSkyGFXPostFXRaster2 = NULL;

RwRaster* pDarkRaster = NULL;

ES2Shader* g_pChromaticAberrationShader = NULL;
ES2Shader* g_pChromaticAberrationShader_Intensive = NULL;
ES2Shader* g_pVignetteShader = NULL;

/* Other */
static const char* aSpeedFXSettings[SPEEDFX_SETTINGS] = 
{
    "Disabled",
    "Enabled",
    "Enabled (PC 16:9 fix)",
    "Enabled (PS2 4:3 fix)"
};
static const char* aCrAbFXSettings[CRAB_SETTINGS] = 
{
    "Disabled",
    "Enabled",
    "Intensive"
};

/* Configs */
ConfigEntry *pCFGRenderGrain;
ConfigEntry *pCFGSpeedFX;
ConfigEntry *pCFGCrAbFX;
ConfigEntry *pCFGVignette;

/* Functions */
void CreateEffectsShaders()
{
    // https://lettier.github.io/3d-game-shaders-for-beginners/chromatic-aberration.html
    char sChromaPxl[] = "precision mediump float;\n"
                        "uniform sampler2D Diffuse;\n"
                        "varying mediump vec2 Out_Tex0;\n"
                        "varying mediump vec2 vPos;\n"
                        "void main() {\n"
                        "  vec4 Rcolor = texture2D(Diffuse, Out_Tex0 + vPos * 0.009);\n"
                        "  vec4 Gcolor = texture2D(Diffuse, Out_Tex0 + vPos * 0.006);\n"
                        "  vec4 Bcolor = texture2D(Diffuse, Out_Tex0 - vPos * 0.006);\n"
                        "  gl_FragColor = vec4(Rcolor.r, Gcolor.g, Bcolor.ba);\n"
                        "}";
    char sChromaVtx[] = "precision highp float;\n"
                        "attribute vec3 Position;\n"
                        "attribute vec2 TexCoord0;\n"
                        "varying mediump vec2 Out_Tex0;\n"
                        "varying mediump vec2 vPos;\n"
                        "void main() {\n"
                        "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                        "  vPos.x = pow(gl_Position.x * 0.5, 3.0);\n"
                        "  vPos.y = pow(gl_Position.y, 3.0);\n"
                        "  Out_Tex0 = TexCoord0;\n"
                        "}";
    g_pChromaticAberrationShader = CreateCustomShaderAlloc(0, sChromaPxl, sChromaVtx, sizeof(sChromaPxl), sizeof(sChromaVtx));
    
    char sChromaVtx2[] = "precision highp float;\n"
                         "attribute vec3 Position;\n"
                         "attribute vec2 TexCoord0;\n"
                         "varying mediump vec2 Out_Tex0;\n"
                         "varying mediump vec2 vPos;\n"
                         "void main() {\n"
                         "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                         "  vPos.x = gl_Position.x * 0.5;\n"
                         "  vPos.y = gl_Position.y;\n"
                         "  Out_Tex0 = TexCoord0;\n"
                         "}";
    g_pChromaticAberrationShader_Intensive = CreateCustomShaderAlloc(0, sChromaPxl, sChromaVtx2, sizeof(sChromaPxl), sizeof(sChromaVtx2));

    // https://github.com/TyLindberg/glsl-vignette
    char sVignettePxl[] = "precision mediump float;\n"
                          "uniform sampler2D Diffuse;\n"
                          "varying mediump vec2 Out_Tex0;\n"
                          "varying mediump vec2 vPos;\n"
                          "float vignette(vec2 uv, float radius, float smoothness) {\n"
                          "  float diff = radius - distance(uv, vec2(0.5, 0.5));\n"
                          "  return smoothstep(-smoothness, smoothness, diff);\n"
                          "}\n"
                          "void main() {\n"
                          "  float vignetteValue = 1.0 - vignette(Out_Tex0, 0.5, 0.5);\n"
                          "  gl_FragColor = texture2D(Diffuse, Out_Tex0) * vignetteValue;\n"
                          "}";
    char sVignetteVtx[] = "precision highp float;\n"
                          "attribute vec3 Position;\n"
                          "attribute vec2 TexCoord0;\n"
                          "varying mediump vec2 Out_Tex0;\n"
                          "void main() {\n"
                          "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                          "  Out_Tex0 = TexCoord0;\n"
                          "}";
    g_pVignetteShader = CreateCustomShaderAlloc(0, sVignettePxl, sVignetteVtx, sizeof(sVignettePxl), sizeof(sVignetteVtx));
}
void RenderGrainSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGRenderGrain->SetBool(newVal != 0);
    g_bRenderGrain = pCFGRenderGrain->GetBool();

    cfg->Save();
}
void SpeedFXSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGSpeedFX->SetBool(newVal);
    pCFGSpeedFX->Clamp(0, SPEEDFX_SETTINGS-1);
    g_nSpeedFX = pCFGSpeedFX->GetInt();

    cfg->Save();
}
void CrAbFXSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGCrAbFX->SetInt(newVal);
    pCFGCrAbFX->Clamp(0, CRAB_SETTINGS-1);
    g_nCrAb = pCFGCrAbFX->GetInt();

    cfg->Save();
}
void VignetteSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGVignette->SetInt(newVal);
    pCFGVignette->Clamp(0, 100);
    g_nVignette = pCFGVignette->GetInt();

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
void CreatePlainTexture(RwRaster* target, CRGBA clr)
{
    if(!target) return;

    RwUInt8 *pixels = RwRasterLock(target, 0, 1);
    int pixSize = target->width * target->height;
    for(int i = 0; i < pixSize; ++i)
    {
        *(pixels++) = clr.r;
        *(pixels++) = clr.g;
        *(pixels++) = clr.b;
        *(pixels++) = clr.a;
    }
    RwRasterUnlock(target);
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
void GFX_GrabScreen(bool second = false)
{
    if(postfxX != *renderWidth || postfxY != *renderHeight)
    {
        postfxX = *renderWidth;
        postfxY = *renderHeight;

        if(pSkyGFXPostFXRaster1) RwRasterDestroy(pSkyGFXPostFXRaster1);
        pSkyGFXPostFXRaster1 = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMATDEFAULT);

        if(pSkyGFXPostFXRaster2) RwRasterDestroy(pSkyGFXPostFXRaster2);
        pSkyGFXPostFXRaster2 = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMATDEFAULT);
    }

    if((!second && pSkyGFXPostFXRaster1) || (second && pSkyGFXPostFXRaster2))
    {
        ERQ_GrabFramebuffer(second ? pSkyGFXPostFXRaster2 : pSkyGFXPostFXRaster1);
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
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    uint32 lineHeight  = (uint32)(2.0f * ((float)RsGlobal->maximumHeight) / 448.0f);
    uint32 linePadding = 2 * lineHeight;
    uint32 numLines    = RsGlobal->maximumHeight / linePadding;
    for (auto i = 0u, y = 0u; i < numLines; i++, y += linePadding)
    {
        // Pure black lines. We dont need to waste CPU by fixing this:
        /*float umin = 0;
        float vmin = y / (float)RsGlobal->maximumHeight;
        float umax = 1.0f;
        float vmax = (y + lineHeight) / (float)RsGlobal->maximumHeight;
        
        DrawQuadSetUVs(umin, -vmin, umax, -vmin, umax, -vmax, umin, -vmax);*/
        PostEffectsDrawQuad(0.0f, y, RsGlobal->maximumWidth, lineHeight, 0, 0, 0, 255, pSkyGFXPostFXRaster1);
    }

    ImmediateModeRenderStatesReStore();
}
void GFX_SpeedFX(float speed) // Completed
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

    const float ar43 = 4.0f / 3.0f;
    const float ar169 = 16.0f / 9.0f;
    float ar = (float)RsGlobal->maximumWidth / (float)RsGlobal->maximumHeight;
    float arM = 1.0f;
         if(g_nSpeedFX == SPFX_LIKE_ON_PC)  arM = ar169 / ar;
    else if(g_nSpeedFX == SPFX_LIKE_ON_PS2) arM = ar43  / ar;
    

    int targetShift = fx->nShift;
    int targetShake = fx->nShake;
    int nLoops = fx->nLoops;
    if(DirectionWasLooking <= 2)
    {
        if(targetShift < 1) targetShift = 1;
        targetShift *= 2;

        targetShake = 0;
    }

    // Calculated once per frame
    float uOffset = 0.0f, vOffset = 0.0f, fShiftOffset = (float)targetShift / 400.0f;
    if(targetShake > 0)
    {
        uOffset = ((float)targetShake / 250.0f) * ((float)rand() / (float)RAND_MAX);
        vOffset = ((float)targetShake / 250.0f) * ((float)rand() / (float)RAND_MAX);
    }

    // Unique per loop (gets bigger every iteration)
    float fLoopShiftX1 = fShiftOffset, fLoopShiftX2 = fShiftOffset,
          fLoopShiftY1 = fShiftOffset, fLoopShiftY2 = fShiftOffset;
    for(int i = 0; i < nLoops; ++i)
    {
        float umin = arM * ( ( (DirectionWasLooking > 2) ? 0.0f : uOffset ) + ( (DirectionWasLooking == 2) ? 0.0f : fLoopShiftX1 ) );
        float vmin = ( (DirectionWasLooking <= 2) ? 0.0f : vOffset ) + ( (DirectionWasLooking > 2) ? 0.0f : fLoopShiftY1 );
        float umax = 1.0f - arM * ( ( (DirectionWasLooking > 2) ? 0.0f : uOffset ) + ( (DirectionWasLooking == 1) ? 0.0f : fLoopShiftX2 ) );
        float vmax = 1.0f - ( (DirectionWasLooking > 2) ? 0.0f : fLoopShiftY2 );
        DrawQuadSetUVs(umin, vmax - ( (DirectionWasLooking > 2) ? 0.0f : vOffset ), umax, vmax - ( (DirectionWasLooking > 2) ? 0.0f : uOffset ), umax, vmin, umin, vmin);

        PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 255, 255, 255, 36, pSkyGFXPostFXRaster1);

        if(i > 0) // Just a little optimisation, don't waste CPU for that
        {
            fLoopShiftX1 = fShiftOffset + ( (DirectionWasLooking == 2) ? 0.0f : fLoopShiftX1 );
            fLoopShiftY1 = fShiftOffset + ( (DirectionWasLooking > 2) ? 0.0f : fLoopShiftY1 );
            fLoopShiftX2 = fShiftOffset + ( (DirectionWasLooking == 1) ? 0.0f : fLoopShiftX2 );
            fLoopShiftY2 = fShiftOffset + ( (DirectionWasLooking > 2) ? 0.0f : fLoopShiftY2 );
        }
    }
    ImmediateModeRenderStatesReStore();
}
void GFX_Radiosity(int intensityLimit, int filterPasses, int renderPasses, int intensity)
{
    
}
void GFX_HeatHaze(float intensity, bool alphaMaskMode)
{
    
}
void GFX_UnderWaterRipple(CRGBA col, float xo, float yo, int strength, float speed, float freq)
{
    
}
void GFX_DarknessFilter(int alpha) // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmax, umax, vmax, umax, vmin, umin, vmin);
    PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 0, 0, 0, alpha, NULL);
    
    ImmediateModeRenderStatesReStore();
}
void GFX_ChromaticAberration() // Completed
{
    GFX_GrabScreen(true);

    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    pForcedShader = ( g_nCrAb == CRAB_INTENSE ? g_pChromaticAberrationShader_Intensive : g_pChromaticAberrationShader );
    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0f, 0.0f, 2.0f, 2.0f, 255, 255, 255, 255, pSkyGFXPostFXRaster2);
    pForcedShader = NULL;
    
    ImmediateModeRenderStatesReStore();
}
void GFX_Vignette(int alpha) // Completed
{
    if(alpha <= 0) return;
    if(alpha > 255) alpha = 255;

    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    pForcedShader = g_pVignetteShader;
    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmax, umax, vmax, umax, vmin, umin, vmin);
    PostEffectsDrawQuad(0.0f, 0.0f, 2.0f, 2.0f, 0, 0, 0, alpha, pDarkRaster);
    pForcedShader = NULL;
    
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

    if(pDarkRaster)
    {
        RwRasterDestroy(pDarkRaster);
        pDarkRaster = NULL;
    }
    pDarkRaster = RwRasterCreate(64, 64, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
    CreatePlainTexture(pDarkRaster, CRGBA(0, 0, 0, 255));

    *pbFog = false; // uninitialised variable
}
DECL_HOOKv(PostFX_Render)
{
    GFX_GrabScreen();

    if(*pbFog) RenderScreenFogPostEffect();

    PostFX_Render();

    //GFX_CCTV();
    if(!*pbInCutscene && g_nSpeedFX != SPFX_INACTIVE)
    {
        CAutomobile* veh = (CAutomobile*)FindPlayerVehicle(-1, false);
        if(veh && (veh->m_nVehicleType < VEHICLE_TYPE_HELI || veh->m_nVehicleType > VEHICLE_TYPE_TRAIN) )
        {
            float dirDot = DotProduct(veh->m_vecMoveSpeed, veh->m_matrix->m_forward);
        #ifdef AML32
            if(veh->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE && dirDot > 0.2f && (veh->m_nLocalFlags & 0x80000) != 0 && *(float*)( ((char*)veh) + 2232) < 0.0f )
        #else
            if(veh->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE && dirDot > 0.2f && (veh->hFlagsLocal & 0x80000) != 0 && veh->m_fTyreTemp < 0.0f )
        #endif
            {
                GFX_SpeedFX(2.0f * dirDot * (veh->m_fGasPedal + 1.0f));
            }
            else
            {
                GFX_SpeedFX(FindPlayerSpeed(-1)->Magnitude());
            }
        }
    }

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

    if(g_nCrAb != CRAB_INACTIVE) GFX_ChromaticAberration();
    GFX_Vignette(g_nVignette * 2.55f);
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

    pCFGSpeedFX = cfg->Bind("SpeedFXType", g_nSpeedFX, "Effects");
    AddSetting("Speed FX", g_nSpeedFX, 0, sizeofA(aSpeedFXSettings)-1, aSpeedFXSettings, SpeedFXSettingChanged, NULL);

    pCFGCrAbFX = cfg->Bind("ChromaticAberration", g_nCrAb, "EnchancedEffects");
    AddSetting("Chromatic Aberration", g_nCrAb, 0, sizeofA(aCrAbFXSettings)-1, aCrAbFXSettings, CrAbFXSettingChanged, NULL);

    pCFGVignette = cfg->Bind("VignetteIntensity", g_nVignette, "EnchancedEffects");
    VignetteSettingChanged(0, g_nVignette);
    AddSlider("Vignette Intensity", g_nVignette, 0, 100, VignetteSettingChanged, NULL, NULL);

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
        shadercreation += CreateEffectsShaders;
        //HOOKBLX(ShowGameBuffer, pGTASA + BYBIT(0x1BC5D4 + 0x1, 0x24F994));
    }
}
