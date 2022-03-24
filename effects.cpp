#include <mod/config.h>
#include <gtasa_things.h>
#include "GTASA_STRUCTS.h"
#include <effects.h>

RwUInt8* (*RwRasterLock)(RwRaster*, unsigned char, int) = NULL;
void (*RwRasterUnlock)(RwRaster*) = NULL;
void (*ImmediateModeRenderStatesStore)() = NULL;
void (*ImmediateModeRenderStatesSet)() = NULL;
void (*ImmediateModeRenderStatesReStore)() = NULL;
void (*RwRenderStateSet)(RwRenderState, void*) = NULL;


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

//void GrainEffect(int strength, bool generate)
//{
//    if(grainRaster == NULL)
//    {
//        
//    }
//    if(generate)
//    {
//		RwUInt8 *pixels = RwRasterLock(grainRaster, 0, 1);
//		vrinit(rand());
//		int x = vrget();
//		for(int i = 0; i < 64*64; ++i)
//        {
//			*pixels++ = x;
//			*pixels++ = x;
//			*pixels++ = x;
//			*pixels++ = x & strength;
//			x = vrnext();
//		}
//		RwRasterUnlock(grainRaster);
//	}
//
//	ImmediateModeRenderStatesStore();
//	ImmediateModeRenderStatesSet();
//	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
//	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
//
//	float umin = 0.0f;
//	float vmin = 0.0f;
//	float umax = 5.0f * RsGlobal->MaximumWidth / 640.0f;
//	float vmax = 7.0f * RsGlobal->MaximumHeight / 448.0f;
//
//	DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
//
//	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)D3DBLEND_DESTCOLOR);
//	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)D3DBLEND_SRCALPHA);
//
//	PostEffectsDrawQuad(0.0, 0.0, RsGlobal->MaximumWidth, RsGlobal->MaximumHeight, 0xFFu, 0xFFu, 0xFFu, 0xFF, grainRaster);
//
//	DrawQuadSetDefaultUVs();
//	ImmediateModeRenderStatesReStore();
//}

// Original
void (*RenderFx)(uintptr_t, uintptr_t, unsigned char) = NULL;
void (*RenderWaterCannons)() = NULL;

uintptr_t pg_fx;
bool* pbCCTV;
bool* pbFog;
void (*RenderWaterFog)() = NULL;
void (*RenderMovingFog)() = NULL;
void (*RenderVolumetricClouds)() = NULL;
void (*RenderScreenFogPostEffect)() = NULL;
void (*RenderCCTVPostEffect)() = NULL;

// PostEffects
uintptr_t pGTASAAddr_MobileEffectsRender = 0;
extern "C" void MobileEffectsRender()
{
    if(*pbFog) RenderScreenFogPostEffect(); // CPostEffects::Fog == Screen Fog
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