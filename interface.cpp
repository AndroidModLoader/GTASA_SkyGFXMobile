#include <mod/amlmod.h>
#include "skygfx/include/renderqueue.h"
#include "iskygfx.h"

ISkyGFX skygfxInterface;

extern int g_nDualPass;
extern float g_fDualPassThreshold;
extern unsigned char g_nColorFilter;
extern bool g_bUsePCTimecyc;
extern bool g_bFixSandstorm;
extern bool g_bFixFog;
extern bool g_bExtendRainSplashes;
extern bool g_bMissingEffects;
extern bool g_bMissingPostEffects;
extern bool g_bRenderGrain;
extern int g_nEnvMapType;
extern bool g_bRemoveDumbWaterColorCalculations;
extern bool g_bFixMirrorIssue;
extern bool g_bPS2Sun;
extern bool g_bMoonPhases;
extern bool g_bPS2Flare;
extern int g_nShading;
extern bool g_bAmbLightNoMultiplier;
extern bool g_bVehicleAtomicsPipeline;
extern bool g_bTransparentLockOn;
extern bool g_bDarkerModelsAtSun;
extern bool g_bFixCarLightsIntensity;
extern int g_nSpeedFX;
extern bool g_bRadiosity;
extern int g_nDOF;
extern bool g_bDOFUseScale;
extern float g_fDOFStrength;
extern float g_fDOFDistance;
extern int g_nVignette;
extern int g_nUWR;
extern bool g_bCSB;
extern bool g_bBloom;

static int Global_GetFeatureLevel(eSkyGFXFeature f)
{
    switch(f)
    {
        default: return 0;

        case GFX_GETMAXFEATURES: return GFX_MAX_FEATURES;

        case GFX_DUALPASS: return g_nDualPass;
        case GFX_DUALPASS_THRESHOLD: return (int)(255 * g_fDualPassThreshold);
        case GFX_COLORFILTER: return g_nColorFilter;
        case GFX_USINGPCTIMECYC: return g_bUsePCTimecyc;
        case GFX_FIXSANDSTORM: return g_bFixSandstorm;
        case GFX_FIXFOG: return g_bFixFog;
        case GFX_EXTENDRAINSPLASHES: return g_bExtendRainSplashes;
        case GFX_MISSINGEFFECTS: return g_bMissingEffects;
        case GFX_MISSINGPOSTFX: return g_bMissingPostEffects;
        case GFX_GRAIN: return g_bMissingPostEffects ? g_bRenderGrain : false;
        case GFX_ENVMAPTYPE: return g_nEnvMapType;
        case GFX_WATERCOLORFIX: return g_bRemoveDumbWaterColorCalculations;
        case GFX_FIXMIRRORISSUE: return g_bFixMirrorIssue;
        case GFX_PS2SUNZ: return g_bPS2Sun;
        case GFX_MOONPHASES: return g_bMoonPhases;
        case GFX_PS2FLARE: return g_bPS2Flare;
        case GFX_SHADING: return g_nShading;
        case GFX_NOAMBLIGHTMULT: return g_bAmbLightNoMultiplier;
        case GFX_VEHICLEPARTSPIPELINE: return g_bVehicleAtomicsPipeline;
        case GFX_TRANSPARENTLOCKON: return g_bTransparentLockOn;
        case GFX_DARKERMODELSATSUN: return g_bDarkerModelsAtSun;
        case GFX_CARLIGHTSINTENSITY: return g_bFixCarLightsIntensity;
        case GFX_SPEEDFX: return g_bMissingPostEffects ? g_nSpeedFX : 0;
        case GFX_RADIOSITY: return g_bMissingPostEffects ? g_bRadiosity : 0;
        case GFX_DOF: return g_bMissingPostEffects ? g_nDOF : 0;
        case GFX_DOFUSESCALING: return g_bMissingPostEffects ? g_bDOFUseScale : 0;
        case GFX_DOFSTRENGTH: return g_bMissingPostEffects ? (100.0f * g_fDOFStrength) : 0;
        case GFX_DOFDISTANCEORSCALE:
        {
            if(g_bDOFUseScale)
            {
                return g_bMissingPostEffects ? (100.0f * g_fDOFDistance) : 0;
            }
            return (int)g_fDOFDistance;
        }
        case GFX_VIGNETTE: return g_bMissingPostEffects ? g_nVignette : 0;
        case GFX_UNDERWATERRIPPLE: return g_bMissingPostEffects ? g_nUWR : 0;
        case GFX_CSBFILTER: return g_bMissingPostEffects ? g_bCSB : 0;
        case GFX_BLOOM: return g_bMissingPostEffects ? g_bBloom : 0;
    }
}

void StartInterface()
{
    skygfxInterface.GetFeatureLevel = Global_GetFeatureLevel;
    skygfxInterface.PreMoonMaskRender = []()
    {
        ERQ_BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ZERO);
    };
    skygfxInterface.PostMoonMaskRender = []()
    {
        ERQ_BlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO); // initial values
    };

    RegisterInterface("SkyGFX", &skygfxInterface);
}