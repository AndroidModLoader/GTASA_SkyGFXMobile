#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <gtasa_things.h>
#include "GTASA_STRUCTS.h"
#include <shadows.h>

RwCamera* (*CreateShadowCamera)(ShadowCameraStorage*, int size);
void (*DestroyShadowCamera)(CShadowCamera*);
void (*MakeGradientRaster)(ShadowCameraStorage*);
void (*_rwObjectHasFrameSetFrame)(void*, RwFrame*);
void (*RwFrameDestroy)(RwFrame*);
void (*RpLightDestroy)(RpLight*);
void (*CreateRTShadow)(CRealTimeShadow* shadow, int size, bool blur, int blurPasses, bool extraBlur);
void (*SetShadowedObject)(CRealTimeShadow* shadow, CPhysical* physical);

int (*CamDistComp)(const void*, const void*);
bool (*StoreRealTimeShadow)(CPhysical* physical, float shadowDispX, float shadowDispY, float shadowFrontX, float shadowFrontY, float shadowSideX, float shadowSideY);
void (*UpdateRTShadow)(CRealTimeShadow* shadow);
float *TimeCycShadowDispX, *TimeCycShadowDispY, *TimeCycShadowFrontX, *TimeCycShadowFrontY, *TimeCycShadowSideX, *TimeCycShadowSideY;
int *TimeCycCurrentStoredValue;
int *RTShadowsQuality;
int *RTShadowsQuality2;

static CRealTimeShadowManager_NEW realTimeShadowManLocal;
CRealTimeShadowManager_NEW* g_realTimeShadowMan = &realTimeShadowManLocal;

CRealTimeShadowManager_NEW* RealTimeShadowManager(CRealTimeShadowManager_NEW* self)
{
    self->m_bInitialized = false;
    self->m_pBlurCamera.m_pShadowCamera = NULL;
    self->m_pBlurCamera.m_pShadowTexture = NULL;
    self->m_pGradientCamera.m_pShadowCamera = NULL;
    self->m_pGradientCamera.m_pShadowTexture = NULL;
    for(int i = 0; i < MAX_RT_SHADOWS; ++i)
    {
        self->m_pShadows[i] = NULL;
    }
    return self;
}

void DestructRealTimeShadowManager(CRealTimeShadowManager_NEW* self)
{
    DestroyShadowCamera(self->m_pBlurCamera.m_pShadowCamera);
    DestroyShadowCamera(self->m_pGradientCamera.m_pShadowCamera);
}

bool InitRealTimeShadowManager(CRealTimeShadowManager_NEW* self)
{
    if(self->m_bInitialized) return false;

    for(int i = 0; i < MAX_RT_SHADOWS; ++i)
    {
        CRealTimeShadow* shadow = new CRealTimeShadow;
        self->m_pShadows[i] = shadow;

        shadow->m_pOwner = NULL;
        shadow->m_bRenderedThisFrame = false;
        shadow->m_byteQuality = 0;
        shadow->m_pShadowCamera = NULL;
        shadow->m_bIsBlurred = false;
        shadow->m_nBlurPasses = 0;
        shadow->m_bExtraBlur = false;
        shadow->a7 = -1;
        shadow->m_pRpLight = NULL;

        //if(i < (int)(MAX_RT_SHADOWS * 0.1f)) CreateRTShadow(shadow, 0, false, 4, true);
        //else if(i < (int)(MAX_RT_SHADOWS * 0.33f)) CreateRTShadow(shadow, 1, false, 4, true);
        //else CreateRTShadow(shadow, 2, false, 4, true);

        if(i < 4) CreateRTShadow(shadow, 0, false, 4, true);
        else if(i < 12) CreateRTShadow(shadow, 1, false, 4, true);
        else CreateRTShadow(shadow, 2, false, 4, true);
    }

    self->m_pBlurCamera.m_pShadowCamera->m_pRwCamera = CreateShadowCamera(&self->m_pBlurCamera, 7);
    self->m_pGradientCamera.m_pShadowCamera->m_pRwCamera = CreateShadowCamera(&self->m_pGradientCamera, 7);
    MakeGradientRaster(&self->m_pGradientCamera);

    self->m_bInitialized = true;
    return true;
}

void ExitRealTimeShadowManager(CRealTimeShadowManager_NEW* self)
{
    if(self->m_bInitialized)
    {
        for(int i = 0; i < MAX_RT_SHADOWS; ++i)
        {
            CRealTimeShadow* shadow = self->m_pShadows[i];
            if(shadow)
            {
                CShadowCamera* shadowCamera = shadow->m_pShadowCamera;
                if(shadowCamera)
                {
                    DestroyShadowCamera(shadowCamera);
                    delete shadowCamera;
                    shadow->m_pShadowCamera = NULL;
                }
                shadow->m_pOwner = NULL;
                shadow->a7 = -1;

                RpLight* shadowLight = shadow->m_pRpLight;
                if(shadowLight)
                {
                    RwFrameDestroy((RwFrame*)shadowLight->object.object.parent);
                    _rwObjectHasFrameSetFrame((void*)shadowLight, NULL);
                    RpLightDestroy(shadowLight);
                }
                delete self->m_pShadows[i];
            }
            self->m_pShadows[i] = NULL;
        }
        DestroyShadowCamera(self->m_pBlurCamera.m_pShadowCamera);
        DestroyShadowCamera(self->m_pGradientCamera.m_pShadowCamera);
        self->m_bInitialized = false;
    }
}

void ReturnRTShadow(CRealTimeShadowManager_NEW* self, CRealTimeShadow* shadow)
{
    if(self->m_bInitialized)
    {
        shadow->m_pOwner->m_pRTShadow = NULL;
        shadow->m_pOwner = NULL;
    }
}

void UpdateRealTimeShadowManager(CRealTimeShadowManager_NEW* self)
{
    if(*RTShadowsQuality2 == 0) return;

    CShadowCamera* pShadowValues[MAX_RT_SHADOWS + 12];
    int v3 = 4, v4 = 12, v5 = 0, v9, v10, v11;
    CRealTimeShadow* shadow;
    CShadowCamera* shadowCamera;
    int lowResShadow = 12, midResShadow = 0, highResShadow = 4;

    for(unsigned char i = 0; i < MAX_RT_SHADOWS; ++i)
    {
        shadowCamera = self->m_pShadows[i]->m_pShadowCamera;
        if(shadowCamera)
        {
            int framebufferWidth = shadowCamera->m_pRwCamera->framebuf->width;
            if(framebufferWidth == 16)
            {
                pShadowValues[lowResShadow] = shadowCamera;
                ++lowResShadow;
            }
            else if(framebufferWidth <= 256)
            {
                pShadowValues[highResShadow] = shadowCamera;
                ++highResShadow;
            }
            else
            {
                pShadowValues[midResShadow] = shadowCamera;
                ++midResShadow;
            }
        }
    }

    qsort((void*)(&self->m_pShadows[1]), MAX_RT_SHADOWS-2, 4, CamDistComp); // Shadow #0 is used for player!

    int v12 = 0;
    for(unsigned char i = 0; i < MAX_RT_SHADOWS; ++i)
    {
        shadow = self->m_pShadows[i];
        shadow->m_pShadowCamera = NULL;
        if(shadow->m_pOwner != NULL && v12 < 4 && shadow->m_pOwner->m_nType == ENTITY_TYPE_VEHICLE)
        {
            shadow->m_pShadowCamera = pShadowValues[v12];
            ++v12;
        }
    }

    for(unsigned char i = 0; i < MAX_RT_SHADOWS; ++i)
    {
        shadow = self->m_pShadows[i];
        if(!shadow->m_pShadowCamera)
        {
            shadow->m_pShadowCamera = pShadowValues[v12];
            ++v12;
        }
    }

    self->m_bSomethingForVehicleRenderPipe = true;
    for(unsigned char i = 0; i < MAX_RT_SHADOWS; ++i)
    {
        shadow = self->m_pShadows[i];
        if(shadow->m_pOwner != NULL)
        {
            // ANTI-CRASH
            if(shadow->m_pOwner->GetMatrix() == NULL) continue;
            // ANTI-CRASH

            char& quality = shadow->m_byteQuality;
            if (i < 11 && shadow->m_bRenderedThisFrame)
            {
                if(quality < 90) quality += 10;
                else quality = 100;
            }
            else
            {
                if(quality <= 10) quality = 0;
                else quality -= 10;
            }

            if (quality)
            {
                if(shadow->m_pShadowCamera && StoreRealTimeShadow(shadow->m_pOwner,
                    TimeCycShadowDispX[*TimeCycCurrentStoredValue],  TimeCycShadowDispY[*TimeCycCurrentStoredValue],
                    TimeCycShadowFrontX[*TimeCycCurrentStoredValue], TimeCycShadowFrontY[*TimeCycCurrentStoredValue],
                    TimeCycShadowSideX[*TimeCycCurrentStoredValue],  TimeCycShadowSideY[*TimeCycCurrentStoredValue]))
                {
                    UpdateRTShadow(shadow);
                }
            }
            else if(self->m_bInitialized)
            {
                shadow->m_pOwner->m_pRTShadow = NULL;
                shadow->m_pOwner = NULL;
            }
        }
    }
    self->m_bSomethingForVehicleRenderPipe = false;

    for(unsigned char i = 0; i < MAX_RT_SHADOWS; ++i)
    {
        self->m_pShadows[i]->m_bRenderedThisFrame = false;
    }
}

void DoShadowThisFrame(CRealTimeShadowManager_NEW* self, CPhysical* physical)
{
    //if((physical->m_nType == ENTITY_TYPE_PED && ((CPed*)physical)->m_nPedType == PED_TYPE_PLAYER1) || *RTShadowsQuality == 2)
    if(*RTShadowsQuality2 == 2)
    {
        CRealTimeShadow* rtShadow = physical->m_pRTShadow;
        if(rtShadow)
        {
            rtShadow->m_bRenderedThisFrame = true;
        }
        else
        {
            if(self->m_bInitialized)
            {
                if(physical->m_nType == ENTITY_TYPE_PED && ((CPed*)physical)->m_nPedType == PED_TYPE_PLAYER1)
                {
                    rtShadow = self->m_pShadows[0];
                }
                else
                {
                    rtShadow = NULL;
                    for(int i = 1; i < MAX_RT_SHADOWS; ++i)
                    {
                        CRealTimeShadow* tmpShadow = self->m_pShadows[i];
                        if(tmpShadow->m_pOwner == NULL)
                        {
                            rtShadow = tmpShadow;
                            break;
                        }
                    }
                }

                if(rtShadow)
                {
                    SetShadowedObject(rtShadow, physical);
                    physical->m_pRTShadow = rtShadow;
                    rtShadow->m_bRenderedThisFrame = true;
                }
            }
        }
    }
}

void PatchRTShadowMan()
{
    SET_TO(CreateRTShadow,                aml->GetSym(pGTASA, "_ZN15CRealTimeShadow6CreateEibib"));
    SET_TO(CreateShadowCamera,            aml->GetSym(pGTASA, "_ZN13CShadowCamera6CreateEi"));
    SET_TO(DestroyShadowCamera,           aml->GetSym(pGTASA, "_ZN13CShadowCamera7DestroyEv"));
    SET_TO(MakeGradientRaster,            aml->GetSym(pGTASA, "_ZN13CShadowCamera18MakeGradientRasterEv"));
    SET_TO(_rwObjectHasFrameSetFrame,     aml->GetSym(pGTASA, "_Z25_rwObjectHasFrameSetFramePvP7RwFrame"));
    SET_TO(RwFrameDestroy,                aml->GetSym(pGTASA, "_Z14RwFrameDestroyP7RwFrame"));
    SET_TO(RpLightDestroy,                aml->GetSym(pGTASA, "_Z14RpLightDestroyP7RpLight"));
    SET_TO(SetShadowedObject,             aml->GetSym(pGTASA, "_ZN15CRealTimeShadow17SetShadowedObjectEP9CPhysical"));

    SET_TO(CamDistComp,                   aml->GetSym(pGTASA, "_ZN22CRealTimeShadowManager11CamDistCompEPKvS1_"));
    SET_TO(StoreRealTimeShadow,           aml->GetSym(pGTASA, "_ZN8CShadows19StoreRealTimeShadowEP9CPhysicalffffff"));
    SET_TO(UpdateRTShadow,                aml->GetSym(pGTASA, "_ZN15CRealTimeShadow6UpdateEv"));
    SET_TO(TimeCycShadowDispX,            aml->GetSym(pGTASA, "_ZN10CTimeCycle22m_fShadowDisplacementXE"));
    SET_TO(TimeCycShadowDispY,            aml->GetSym(pGTASA, "_ZN10CTimeCycle22m_fShadowDisplacementYE"));
    SET_TO(TimeCycShadowFrontX,           aml->GetSym(pGTASA, "_ZN10CTimeCycle15m_fShadowFrontXE"));
    SET_TO(TimeCycShadowFrontY,           aml->GetSym(pGTASA, "_ZN10CTimeCycle15m_fShadowFrontYE"));
    SET_TO(TimeCycShadowSideX,            aml->GetSym(pGTASA, "_ZN10CTimeCycle14m_fShadowSideXE"));
    SET_TO(TimeCycShadowSideY,            aml->GetSym(pGTASA, "_ZN10CTimeCycle14m_fShadowSideYE"));
    SET_TO(TimeCycCurrentStoredValue,     aml->GetSym(pGTASA, "_ZN10CTimeCycle20m_CurrentStoredValueE"));
    SET_TO(RTShadowsQuality,              pGTASAAddr + 0x6E049C);
    SET_TO(RTShadowsQuality2,             pGTASAAddr + 0x6E0494);

    aml->Write(pGTASAAddr + 0x679A98, (uintptr_t)&g_realTimeShadowMan, sizeof(void*));
    Redirect(aml->GetSym(pGTASA, "_ZN22CRealTimeShadowManagerC2Ev"), (uintptr_t)RealTimeShadowManager);
    Redirect(aml->GetSym(pGTASA, "_ZN22CRealTimeShadowManagerD2Ev"), (uintptr_t)DestructRealTimeShadowManager);
    Redirect(aml->GetSym(pGTASA, "_ZN22CRealTimeShadowManager4InitEv"), (uintptr_t)InitRealTimeShadowManager);
    Redirect(aml->GetSym(pGTASA, "_ZN22CRealTimeShadowManager4ExitEv"), (uintptr_t)ExitRealTimeShadowManager);
    Redirect(aml->GetSym(pGTASA, "_ZN22CRealTimeShadowManager6UpdateEv"), (uintptr_t)UpdateRealTimeShadowManager);
    Redirect(aml->GetSym(pGTASA, "_ZN22CRealTimeShadowManager20ReturnRealTimeShadowEP15CRealTimeShadow"), (uintptr_t)ReturnRTShadow);
    Redirect(aml->GetSym(pGTASA, "_ZN22CRealTimeShadowManager17DoShadowThisFrameEP9CPhysical"), (uintptr_t)DoShadowThisFrame);
}