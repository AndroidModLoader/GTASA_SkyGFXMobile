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