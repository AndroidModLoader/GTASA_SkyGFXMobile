#include <mod/amlmod.h>
#include "GTASA_STRUCTS.h"

#define GRAIN_EFFECT_SCALE            0.75f
#define GRAIN_NIGHTVISION_STRENGTH    48
#define GRAIN_INFRAREDVISION_STRENGTH 64


// Functions
void SetUpWaterFog_stub(void);
void MobileEffectsRender_stub(void);
void EffectsRender_stub(void);

// Function-pointers
extern void (*RenderFx)(uintptr_t, uintptr_t, unsigned char);
extern void (*RenderWaterCannons)();
extern void (*RenderWaterFog)();
extern void (*RenderMovingFog)();
extern void (*RenderVolumetricClouds)();
extern void (*RenderScreenFogPostEffect)();
extern void (*RenderCCTVPostEffect)();

extern uintptr_t pg_fx;
extern uintptr_t pFxCamera;
extern uintptr_t pGTASAAddr_MobileEffectsRender;
extern uintptr_t pGTASAAddr_EffectsRender;
extern uintptr_t* pdword_952880;
extern bool* pbCCTV;
extern bool* pbFog;

// Grain
extern DECL_HOOKv(InitialisePostEffects);
extern RwRaster* (*RwRasterCreate)(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags);
extern RwUInt8* (*RwRasterLock)(RwRaster*, unsigned char, int);
extern void (*RwRasterUnlock)(RwRaster*);
extern void (*ImmediateModeRenderStatesStore)();
extern void (*ImmediateModeRenderStatesSet)();
extern void (*ImmediateModeRenderStatesReStore)();
extern void (*RwRenderStateSet)(RwRenderState, void*);
extern void (*DrawQuadSetUVs)(float,float,float,float,float,float,float,float);
extern void (*PostEffectsDrawQuad)(float,float,float,float,uint8_t,uint8_t,uint8_t,uint8_t,RwRaster*);
extern void (*DrawQuadSetDefaultUVs)(void);
extern uint8_t (*CamNoRain)();
extern uint8_t (*PlayerNoRain)();
extern RsGlobalType* RsGlobal;
extern int *pnGrainStrength;
extern int *currArea;
extern bool *pbGrainEnable;
extern bool *pbRainEnable;
extern bool *pbInCutscene;
extern bool *pbNightVision;
extern bool *pbInfraredVision;
extern float* pfWeatherRain;
extern float* pfWeatherUnderwaterness;
extern CCamera* effects_TheCamera;

extern unsigned char grainIntensity;