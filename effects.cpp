#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <gtasa_things.h>
#include <effects.h>

RwRaster* (*RwRasterCreate)(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags) = NULL;
RwUInt8* (*RwRasterLock)(RwRaster*, unsigned char, int) = NULL;
void (*RwRasterUnlock)(RwRaster*) = NULL;
void (*ImmediateModeRenderStatesStore)() = NULL;
void (*ImmediateModeRenderStatesSet)() = NULL;
void (*ImmediateModeRenderStatesReStore)() = NULL;
void (*RwRenderStateSet)(RwRenderState, void*) = NULL;
void (*DrawQuadSetUVs)(float,float,float,float,float,float,float,float) = NULL;
void (*PostEffectsDrawQuad)(float,float,float,float,uint8_t,uint8_t,uint8_t,uint8_t,RwRaster*) = NULL;
void (*DrawQuadSetDefaultUVs)(void) = NULL;
uint8_t (*CamNoRain)() = NULL;
uint8_t (*PlayerNoRain)() = NULL;
RsGlobalType* RsGlobal = NULL;
int *pnGrainStrength = NULL;
int *currArea = NULL;
bool *pbGrainEnable = NULL;
bool* pbRainEnable = NULL;
bool *pbInCutscene = NULL;
bool *pbNightVision = NULL;
bool *pbInfraredVision = NULL;
float* pfWeatherRain = NULL;
float* pfWeatherUnderwaterness = NULL;
CCamera* effects_TheCamera = NULL;
unsigned char grainIntensity;


RwRaster *grainRaster = NULL;
// VU style random number generator -- taken from pcsx2
uint32_t R;
void vrinit(uint32_t x) { R = 0x3F800000 | x & 0x007FFFFF; }
void vradvance(void)
{
    int x = (R >> 4) & 1;
    int y = (R >> 22) & 1;
    R <<= 1;
    R ^= x ^ y;
    R = (R & 0x7FFFFF) | 0x3F800000;
}
inline uint32_t vrget(void) { return R; }
inline uint32_t vrnext(void) { vradvance(); return R; }

void GrainEffect(int strength, bool generate)
{
    if(generate)
    {
        RwUInt8 *pixels = RwRasterLock(grainRaster, 0, 1);
        vrinit(rand());
        int x = vrget();
        for(int i = 0; i < 64*64; ++i)
        {
            *pixels++ = x;
            *pixels++ = x;
            *pixels++ = x;
            *pixels++ = x & strength;
            x = vrnext();
        }
        RwRasterUnlock(grainRaster);
    }
    
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    
    float umin = 0.0f;
    float vmin = 0.0f;
    float umax = (float)(GRAIN_EFFECT_SCALE) * 5.0f * RsGlobal->maximumWidth / 640.0f;
    float vmax = (float)(GRAIN_EFFECT_SCALE) * 7.0f * RsGlobal->maximumHeight / 448.0f;
    
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    
    PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 0xFFu, 0xFFu, 0xFFu, grainIntensity, grainRaster);
    
    DrawQuadSetDefaultUVs();
    ImmediateModeRenderStatesReStore();
}

// Original
void (*RenderFx)(uintptr_t, uintptr_t, unsigned char) = NULL;
void (*RenderWaterCannons)() = NULL;

uintptr_t pg_fx;
bool* pbCCTV;
bool* pbFog = NULL;
void (*RenderWaterFog)() = NULL;
void (*RenderMovingFog)() = NULL;
void (*RenderVolumetricClouds)() = NULL;
void (*RenderScreenFogPostEffect)() = NULL;
void (*RenderCCTVPostEffect)() = NULL;

// Hooks
DECL_HOOKv(InitialisePostEffects)
{
    InitialisePostEffects();
    grainRaster = RwRasterCreate(64, 64, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
}


// PostEffects
uintptr_t pGTASAAddr_MobileEffectsRender = 0;
int nRainGrain = 0;
extern "C" void MobileEffectsRender()
{
    if(pbFog != NULL && *pbFog) RenderScreenFogPostEffect(); // CPostEffects::Fog == Screen Fog
    if(grainRaster != NULL)
    {
        if(!*pbInCutscene)
        {
            if(*pbNightVision)
            {
                GrainEffect(GRAIN_NIGHTVISION_STRENGTH, true);
            }
            else if(*pbInfraredVision)
            {
                GrainEffect(GRAIN_INFRAREDVISION_STRENGTH, true);
            }
        }
        if(*pbRainEnable)
        {
            float scaled = *pfWeatherRain * 128.0f;
            int tempo = nRainGrain;
            if(nRainGrain < scaled) tempo = ++nRainGrain;
            else if(nRainGrain > scaled) --tempo;
            nRainGrain = tempo < 0 ? 0 : tempo;

            if(!CamNoRain() && !PlayerNoRain() && *pfWeatherUnderwaterness <= 0.0f && !*currArea)
            {
                CVector& cameraPos = effects_TheCamera->GetPosition();
                if(cameraPos.z <= 900.0f)
                {
                    GrainEffect(0.25f * nRainGrain, true);
                }
            }
        }
        if(*pbGrainEnable) GrainEffect(*pnGrainStrength, true);
    }

    // Original
    if(*pbCCTV) RenderCCTVPostEffect(); // CCTV
}
uintptr_t pGTASAAddr_EffectsRender = 0;
uintptr_t* pdword_952880;
extern "C" void EffectsRender()
{
    RenderFx(pg_fx, *pdword_952880, 0);
    RenderWaterCannons();
    
    if(RenderWaterFog) RenderWaterFog();
    if(RenderMovingFog) RenderMovingFog();
    if(RenderVolumetricClouds) RenderVolumetricClouds();
}

__attribute__((optnone)) __attribute__((naked)) void MobileEffectsRender_stub(void)
{
    asm volatile(
        "push {r0-r11}\n"
        "bl MobileEffectsRender\n"
    );

    asm volatile(
        "mov r12, %0\n"
        "pop {r0-r11}\n"
        "bx r12\n"
    :: "r" (pGTASAAddr_MobileEffectsRender));
}
__attribute__((optnone)) __attribute__((naked)) void EffectsRender_stub(void)
{
    asm volatile(
        "push {r0-r11}\n"
        "bl EffectsRender\n"
    );

    asm volatile(
        "mov r12, %0\n"
        "pop {r0-r11}\n"
        "bx r12\n"
    :: "r" (pGTASAAddr_EffectsRender));
}

uintptr_t retTo;
extern ConfigEntry* pWaterFogBlocksLimits;
extern "C" void SetUpWaterFog()
{
    if(*(int*)(pGTASAAddr + 0xA1DC9C) >= pWaterFogBlocksLimits->GetInt()) retTo = pGTASAAddr + 0x59A0DC + 0x1;
    else retTo = pGTASAAddr + 0x599FC0 + 0x1;
}

__attribute__((optnone)) __attribute__((naked)) void SetUpWaterFog_stub(void)
{
    asm volatile(
        "push {r0-r11}\n"
        "bl SetUpWaterFog\n"
    );
    asm volatile(
        "mov r12, %0\n"
        "pop {r0-r11}\n"
        "bx r12\n"
      :: "r"(retTo) );
}