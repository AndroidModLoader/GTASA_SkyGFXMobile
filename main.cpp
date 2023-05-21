#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <string> // memset
#include <dlfcn.h> // dlopen

#include <gtasa_things.h>

#include <shader.h>
#include <pipeline.h>
#include <effects.h>
#include <plantsurfprop.h>
#include <colorfilter.h>
#include <shading.h>
#include <shadows.h>

// SAUtils
#include <isautils.h>
ISAUtils* sautils = NULL;

#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))
MYMODCFG(net.rusjj.skygfxmobile, SkyGfx Mobile Beta, 0.1.2, aap & TheOfficialFloW & RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.0.2.1)
END_DEPLIST()

/* Config */
//ConfigEntry* pBonesOptimization;
//ConfigEntry* pMVPOptimization;
ConfigEntry* pDisablePedSpec;
ConfigEntry* pPS2Sun;
ConfigEntry* pPS2Shading;
ConfigEntry* pPS2Reflections;
ConfigEntry* pColorfilter;
ConfigEntry* pDisableDetailTexture;
ConfigEntry* pDisableClouds;
ConfigEntry* pPS2Pipeline;
ConfigEntry* pPS2PipelineRenderWay;
ConfigEntry* pPlantsDatLoading;
ConfigEntry* pProcobjDatLoading;
ConfigEntry* pExponentialFog;
ConfigEntry* pDisableCamnorm;
ConfigEntry* pScreenFog;
ConfigEntry* pGrainEffect;
ConfigEntry* pGrainIntensityPercent;
ConfigEntry* pWaterFog;
ConfigEntry* pWaterFogBlocksLimits;
ConfigEntry* pMovingFog;
ConfigEntry* pVolumetricClouds;
ConfigEntry* pPedShadowDistance;
ConfigEntry* pVehicleShadowDistance;
ConfigEntry* pMaxRTShadows;
ConfigEntry* pDynamicObjectsShadows;
ConfigEntry* pAllowPlayerClassicShadow;
ConfigEntry* pNewShaderLighting;

/* Patch Saves */
//const uint32_t sunCoronaRet = 0xF0F7ECE0;
const uint32_t sunCoronaDel = 0xBF00BF00;

/* Saves */
void* hGTASA = NULL;
uintptr_t pGTASA = 0;

extern DECL_HOOKv(PlantMgrInit);
extern DECL_HOOKv(PlantMgrRender);
extern DECL_HOOKv(PlantSurfPropMgrInit);
extern DECL_HOOK(CPlantLocTri*, PlantLocTriAdd, CPlantLocTri* self, CVector& p1, CVector& p2, CVector& p3, uint8_t surface, tColLighting lightning, bool createsPlants, bool createsObjects);

uintptr_t OGLSomething_BackTo;
extern "C" void OGLSomething(RwRaster* unkPtr)
{
    if(!unkPtr || (*(uint8_t*)((uintptr_t)unkPtr + 48) << 0x1F)) OGLSomething_BackTo = pGTASA + 0x222E04 + 0x1;
    else OGLSomething_BackTo = pGTASA + 0x222DBE + 0x1;
}
__attribute__((optnone)) __attribute__((naked)) void OGLSomething_Patch(void)
{
    asm volatile(
        "LDR R1, [R10]\n"
        "PUSH {R0-R11}\n"
        "LDR R0, [SP, #4]\n"
        "BL OGLSomething\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0-R11}\n"
        "BX R12\n"
    :: "r" (OGLSomething_BackTo));
}

DECL_HOOKv(InitRW)
{
    logger->Info("Initializing RW...");
    InitRW();
}

int emu_InternalSkinGetVectorCount(void)
{
    return 4 * *skin_num;
}

void SkinSetMatrices(uintptr_t skin, float* matrix)
{
    int num = *(int*)(skin + 4);
    memcpy(skin_map, matrix, 64 * num);
    *skin_dirty = 1;
    *skin_num = num;
}

void PedShadowDistanceChanged(int oldVal, int newVal, void* data)
{
    pPedShadowDistance->SetFloat(0.01f * newVal);
    *(float*)(pGTASA + 0x5BD2FC) = pPedShadowDistance->GetFloat();
    cfg->Save();
}
const char* PedShadowDistanceDraw(int nNewValue)
{
    static char ret[12];
    sprintf(ret, "%.2fm", 0.01f * nNewValue);
    return ret;
}

void GrainIntensityChanged(int oldVal, int newVal, void* data)
{
    pGrainIntensityPercent->SetInt(newVal);
    grainIntensity = newVal * 2.55f;
    cfg->Save();
}

void VehicleShadowDistanceChanged(int oldVal, int newVal, void* data)
{
    pVehicleShadowDistance->SetFloat(0.01f * newVal);
    *(float*)(pGTASA + 0x5B9AE0) = pVehicleShadowDistance->GetFloat();
    cfg->Save();
}
const char* VehicleShadowDistanceDraw(int nNewValue)
{
    static char ret[12];
    sprintf(ret, "%.2fm", 0.01f * nNewValue);
    return ret;
}

extern "C" void OnModLoad()
{
    logger->SetTag("SkyGfx Mobile");
// Addresses
    hGTASA = dlopen("libGTASA.so", RTLD_LAZY);
    pGTASA = aml->GetLib("libGTASA.so");
    sautils = (ISAUtils*)GetInterface("SAUtils");
// Config
    cfg->Bind("Author", "", "About")->SetString("[-=KILL MAN=-]");
    cfg->Bind("IdeasFrom", "", "About")->SetString("aap, TheOfficialFloW");
    cfg->Bind("Discord", "", "About")->SetString("https://discord.gg/2MY7W39kBg");
    cfg->Bind("GitHub", "", "About")->SetString("https://github.com/AndroidModLoader/GTASA_SkyGfxMobile");

    // Config: Information
    cfg->Bind("About_PS2_Reflections", "Works only on low reflections setting with PS2 Shading enabled", "About");
    cfg->Bind("Colorfilter_Values", "default none ps2 pc", "About");
    
    cfg->Save();

    // Config: Render
    //pBonesOptimization = cfg->Bind("BonesOptim", false);
    //pMVPOptimization = cfg->Bind("MVPOptim", false);
    pDisablePedSpec = cfg->Bind("DisablePedSpec", true, "Render");
    pPS2Sun = cfg->Bind("PS2_Sun", true, "Render");
    pPS2Shading = cfg->Bind("PS2_Shading", true, "Render");
    pPS2Reflections = cfg->Bind("PS2_Reflections", true, "Render");
    pColorfilter = cfg->Bind("Colorfilter", "ps2", "Render");
    pDisableDetailTexture = cfg->Bind("DisableDetailTexture", false, "Render");
    pDisableClouds = cfg->Bind("DisableClouds", false, "Render");
    pPS2Pipeline = cfg->Bind("PS2_Pipeline", true, "Render");
    pPS2PipelineRenderWay = cfg->Bind("PS2_Pipeline_RenderWay", DPWay_LastMesh, "Render");
    pPlantsDatLoading = cfg->Bind("PlantsDatLoading", false, "Features");
    pProcobjDatLoading = cfg->Bind("ProcObjDatLoading", true, "Features");
    pExponentialFog = cfg->Bind("ExponentialFog", false, "Render"); // Incomplete?
    pDisableCamnorm = cfg->Bind("DisableCamnorm", false, "Render");

    if(pPS2PipelineRenderWay->GetInt() <= 0) pipelineWay = DPWay_Default;
    if(pPS2PipelineRenderWay->GetInt() >= DPWay_MaxWays) pipelineWay = DPWay_Everything;
    else pipelineWay = (ePipelineDualpassWay)pPS2PipelineRenderWay->GetInt();

    if(pDisableCamnorm->GetBool()) aml->PlaceNOP(pGTASA + 0x1E7C6C, 9);

    // Config: PostEffects
    pScreenFog = cfg->Bind("ScreenFog", true, "PostEffects");
    pGrainEffect = cfg->Bind("Grain", true, "PostEffects");
    pGrainIntensityPercent = cfg->Bind("GrainIntensityPercent", 60, "PostEffects");

    if(pGrainIntensityPercent->GetInt() >= 100) grainIntensity = 255;
    else if(pGrainIntensityPercent->GetInt() <= 0) grainIntensity = 0;
    else grainIntensity = pGrainIntensityPercent->GetInt() * 2.55f;

    // Config: Effects
    pWaterFog = cfg->Bind("WaterFog", true, "Effects");
    pWaterFogBlocksLimits = cfg->Bind("WaterFogBlocksLimit", 512, "Effects"); // 70 is default
    pMovingFog = cfg->Bind("MovingFog", true, "Effects");
    pVolumetricClouds = cfg->Bind("VolumetricClouds", true, "Effects");

    // Config: Shadows (not working)
    //aml->Unprot(pGTASA + 0x5BD2FC, sizeof(float));
    //pPedShadowDistance = cfg->Bind("PedShadowDistance", *(float*)(pGTASA + 0x5BD2FC), "Shadows"); *(float*)(pGTASA + 0x5BD2FC) = pPedShadowDistance->GetFloat();
    //if(sautils != NULL) sautils->AddSliderItem(Display, "Ped Shadow Distance", 100 * pPedShadowDistance->GetFloat(), 100 * 4.0f, 100 * 256.0f, PedShadowDistanceChanged, PedShadowDistanceDraw);
    //aml->Unprot(pGTASA + 0x5B9AE0, sizeof(float));
    //pVehicleShadowDistance = cfg->Bind("VehicleShadowDistance", *(float*)(pGTASA + 0x5B9AE0), "Shadows"); *(float*)(pGTASA + 0x5B9AE0) = pVehicleShadowDistance->GetFloat();
    //if(sautils != NULL) sautils->AddSliderItem(Display, "Vehicle Shadow Distance", 100 * pVehicleShadowDistance->GetFloat(), 100 * 4.0f, 100 * 256.0f, VehicleShadowDistanceChanged, VehicleShadowDistanceDraw);

    //pMaxRTShadows = cfg->Bind("MaxRTShadows", 64, "Shadows"); // Default value is 40
    //pDynamicObjectsShadows = cfg->Bind("DynamicObjectsShadows", false, "Shadows");
    //pAllowPlayerClassicShadow = cfg->Bind("AllowPlayerClassicShadow", false, "Shadows");

    HOOKPLT(InitRW, pGTASA + 0x66F2D0);

// Patches
    SET_TO(m_fDNBalanceParam,               aml->GetSym(hGTASA, "_ZN25CCustomBuildingDNPipeline17m_fDNBalanceParamE"));
    // Patches: Shading
    if(pPS2Shading->GetBool())
    {
        logger->Info("PS2 Shading enabled!");
        aml->PlaceNOP(pGTASA + 0x1C1382);
        aml->PlaceNOP(pGTASA + 0x1C13BA);
        SET_TO(RwFrameTransform,                aml->GetSym(hGTASA, "_Z16RwFrameTransformP7RwFramePK11RwMatrixTag15RwOpCombineType"));
        SET_TO(RpLightSetColor,                 aml->GetSym(hGTASA, "_Z15RpLightSetColorP7RpLightPK10RwRGBAReal"));
        SET_TO(p_pAmbient,                      aml->GetSym(hGTASA, "pAmbient"));
        SET_TO(p_pDirect,                       aml->GetSym(hGTASA, "pDirect"));
        SET_TO(p_AmbientLightColourForFrame,    aml->GetSym(hGTASA, "AmbientLightColourForFrame"));
        SET_TO(p_AmbientLightColourForFrame_PedsCarsAndObjects, aml->GetSym(hGTASA, "AmbientLightColourForFrame_PedsCarsAndObjects"));
        SET_TO(p_DirectionalLightColourForFrame,aml->GetSym(hGTASA, "DirectionalLightColourForFrame"));
        SET_TO(p_DirectionalLightColourFromDay, aml->GetSym(hGTASA, "DirectionalLightColourFromDay"));
        SET_TO(p_CTimeCycle__m_CurrentColours,  aml->GetSym(hGTASA, "_ZN10CTimeCycle16m_CurrentColoursE"));
        SET_TO(p_CTimeCycle__m_vecDirnLightToSun, aml->GetSym(hGTASA, "_ZN10CTimeCycle19m_vecDirnLightToSunE"));
        SET_TO(p_gfLaRiotsLightMult,            aml->GetSym(hGTASA, "gfLaRiotsLightMult"));
        SET_TO(p_CCoronas__LightsMult,          aml->GetSym(hGTASA, "_ZN8CCoronas10LightsMultE"));
        SET_TO(p_CWeather__LightningFlash,      aml->GetSym(hGTASA, "_ZN8CWeather14LightningFlashE"));
        SET_TO(openglAmbientLight,              aml->GetSym(hGTASA, "openglAmbientLight"));
        SET_TO(p_rwOpenGLColorMaterialEnabled,  aml->GetSym(hGTASA, "_rwOpenGLColorMaterialEnabled"));
        SET_TO(emu_glLightModelfv,              aml->GetSym(hGTASA, "_Z18emu_glLightModelfvjPKf"));
        SET_TO(emu_glMaterialfv,                aml->GetSym(hGTASA, "_Z16emu_glMaterialfvjjPKf"));
        SET_TO(emu_glColorMaterial,             aml->GetSym(hGTASA, "_Z19emu_glColorMaterialjj"));
        SET_TO(emu_glEnable,                    aml->GetSym(hGTASA, "_Z12emu_glEnablej"));
        SET_TO(emu_glDisable,                   aml->GetSym(hGTASA, "_Z13emu_glDisablej"));
        #ifdef NEW_LIGHTING
            pNewShaderLighting = cfg->Bind("PS2_Shading_NewLight", true, "Render");
            if(pNewShaderLighting->GetBool())
            {
                aml->Redirect(aml->GetSym(hGTASA, "_Z36_rwOpenGLLightsSetMaterialPropertiesPK10RpMaterialj"), (uintptr_t)_rwOpenGLLightsSetMaterialProperties);
                aml->Redirect(aml->GetSym(hGTASA, "_Z28SetLightsWithTimeOfDayColourP7RpWorld"), (uintptr_t)SetLightsWithTimeOfDayColour);
            }
        #endif // NEW_LIGHTING
        // Bones Optim (for SkyGfx shaders only, not yet, doesnt draw ped models)
        /*if(pBonesOptimization->GetBool())
        {
            SET_TO(skin_map,                    aml->GetSym(hGTASA, "skin_map"));
            SET_TO(skin_dirty,                  aml->GetSym(hGTASA, "skin_dirty"));
            SET_TO(skin_num,                    aml->GetSym(hGTASA, "skin_num"));
            aml->Unprot((uintptr_t)skin_map, 0x1800);
            aml->Redirect(aml->GetSym(hGTASA, "_Z30emu_InternalSkinGetVectorCountv"), (uintptr_t)emu_InternalSkinGetVectorCount);
            aml->Redirect(pGTASA + 0x1C8670 + 0x1, (uintptr_t)SkinSetMatrices);
        }*/
    }

    // PS2 Sun Corona
    if(pPS2Sun->GetBool())
    {
        logger->Info("PS2 Sun Corona enabled!");
        aml->Unprot(pGTASA + 0x5A26B0, 4);
        *(uint32_t*)(pGTASA + 0x5A26B0) = sunCoronaDel;
    }

    // Detail textures
    if(pDisableDetailTexture->GetBool())
    {
        logger->Info("Detail textures are disabled!");
        aml->PlaceRET(pGTASA + 0x1BCBC4);
    }

    // Color Filter
    if(!strcmp(pColorfilter->GetString(), "none")) nColorFilter = 1;
    else if(!strcmp(pColorfilter->GetString(), "ps2")) nColorFilter = 2;
    else if(!strcmp(pColorfilter->GetString(), "pc")) nColorFilter = 3;
    if(nColorFilter != 0 || sautils != NULL)
    {
        logger->Info("Colorfilter \"%s\" enabled!", pColorfilter->GetString());
        pGTASA_Colorfilter =                pGTASA + 0x5B6444 + 0x1;
        aml->Redirect(pGTASA + 0x5B643C + 0x1, (uintptr_t)ColorFilter_stub);
        aml->Unprot(pGTASA + 0x5B6444, sizeof(uint16_t));
        memcpy((void *)(pGTASA + 0x5B6444), (void *)(pGTASA + 0x5B63DC), sizeof(uint16_t));
        aml->Unprot(pGTASA + 0x5B6446, sizeof(uint16_t));
        memcpy((void *)(pGTASA + 0x5B6446), (void *)(pGTASA + 0x5B63EA), sizeof(uint16_t));

        if(sautils != NULL) sautils->AddClickableItem(SetType_Display, "Colorfilter", nColorFilter, 0, sizeofA(pColorFilterSettings)-1, pColorFilterSettings, ColorfilterChanged, NULL);
    }

    if(pDisableClouds->GetBool())
    {
        logger->Info("Clouds are disabled!");
        aml->Redirect(pGTASA + 0x59F340 + 0x1, pGTASA + 0x59F40A + 0x1);
    }

    if(pPS2Pipeline->GetBool())
    {
        logger->Info("PS2 Pipeline is enabled!");

        SET_TO(_rxPipelineDestroy,              aml->GetSym(hGTASA, "_Z18_rxPipelineDestroyP10RxPipeline"));
        SET_TO(RxPipelineCreate,                aml->GetSym(hGTASA, "_Z16RxPipelineCreatev"));
        SET_TO(RxPipelineLock,                  aml->GetSym(hGTASA, "_Z14RxPipelineLockP10RxPipeline"));
        SET_TO(RxNodeDefinitionGetOpenGLAtomicAllInOne, aml->GetSym(hGTASA, "_Z39RxNodeDefinitionGetOpenGLAtomicAllInOnev"));
        SET_TO(RxLockedPipeAddFragment,         aml->GetSym(hGTASA, "_Z23RxLockedPipeAddFragmentP10RxPipelinePjP16RxNodeDefinitionz"));
        SET_TO(RxLockedPipeUnlock,              aml->GetSym(hGTASA, "_Z18RxLockedPipeUnlockP10RxPipeline"));
        SET_TO(RxPipelineFindNodeByName,        aml->GetSym(hGTASA, "_Z24RxPipelineFindNodeByNameP10RxPipelinePKcP14RxPipelineNodePi"));
        SET_TO(RxOpenGLAllInOneSetInstanceCallBack, aml->GetSym(hGTASA, "_Z35RxOpenGLAllInOneSetInstanceCallBackP14RxPipelineNodePFiPvP24RxOpenGLMeshInstanceDataiiE"));
        SET_TO(RxOpenGLAllInOneSetRenderCallBack, aml->GetSym(hGTASA, "_Z33RxOpenGLAllInOneSetRenderCallBackP14RxPipelineNodePFvP10RwResEntryPvhjE"));
        //SET_TO(_rwOpenGLSetRenderState,         aml->GetSym(hGTASA, "_Z23_rwOpenGLSetRenderState13RwRenderStatePv"));
        //SET_TO(_rwOpenGLGetRenderState,         aml->GetSym(hGTASA, "_Z23_rwOpenGLGetRenderState13RwRenderStatePv"));
        SET_TO(_rwOpenGLSetRenderState,         aml->GetSym(hGTASA, "_Z16RwRenderStateSet13RwRenderStatePv"));
        SET_TO(_rwOpenGLGetRenderState,         aml->GetSym(hGTASA, "_Z16RwRenderStateGet13RwRenderStatePv"));
        SET_TO(_rwOpenGLSetRenderStateNoExtras, aml->GetSym(hGTASA, "_Z31_rwOpenGLSetRenderStateNoExtras13RwRenderStatePv"));
        SET_TO(_rwOpenGLLightsSetMaterialPropertiesORG, aml->GetSym(hGTASA, "_Z36_rwOpenGLLightsSetMaterialPropertiesPK10RpMaterialj"));
        SET_TO(SetNormalMatrix,                 aml->GetSym(hGTASA, "_Z15SetNormalMatrixff5RwV2d"));
        SET_TO(DrawStoredMeshData,              aml->GetSym(hGTASA, "_ZN24RxOpenGLMeshInstanceData10DrawStoredEv"));
        SET_TO(ResetEnvMap,                     aml->GetSym(hGTASA, "_Z11ResetEnvMapv"));

        SET_TO(rwOpenGLOpaqueBlack,             aml->GetSym(hGTASA, "_rwOpenGLOpaqueBlack"));
        SET_TO(rwOpenGLLightingEnabled,         aml->GetSym(hGTASA, "_rwOpenGLLightingEnabled"));
        SET_TO(rwOpenGLColorMaterialEnabled,    aml->GetSym(hGTASA, "_rwOpenGLColorMaterialEnabled"));
        SET_TO(ms_envMapPluginOffset,           aml->GetSym(hGTASA, "_ZN24CCustomCarEnvMapPipeline21ms_envMapPluginOffsetE"));
        SET_TO(ppline_RasterExtOffset,          aml->GetSym(hGTASA, "RasterExtOffset"));
        SET_TO(byte_70BF3C,                     pGTASA + 0x70BF3C);

        SET_TO(ppline_SetSecondVertexColor,     aml->GetSym(hGTASA, "_Z24emu_SetSecondVertexColorhf"));
        SET_TO(ppline_EnableAlphaModulate,      aml->GetSym(hGTASA, "_Z23emu_EnableAlphaModulatef"));
        SET_TO(ppline_DisableAlphaModulate,     aml->GetSym(hGTASA, "_Z24emu_DisableAlphaModulatev"));
        SET_TO(ppline_glDisable,                aml->GetSym(hGTASA, "_Z13emu_glDisablej"));
        SET_TO(ppline_glColor4fv,               aml->GetSym(hGTASA, "_Z14emu_glColor4fvPKf"));
        SET_TO(ppline_SetEnvMap,                aml->GetSym(hGTASA, "_Z13emu_SetEnvMapPvfi"));
        if(sautils != NULL) sautils->AddClickableItem(SetType_Display, "Building DualPass", pipelineWay, 0, sizeofA(pPipelineSettings)-1, pPipelineSettings, PipelineChanged, NULL);

        aml->Redirect(aml->GetSym(hGTASA, "_ZN25CCustomBuildingDNPipeline19CreateCustomObjPipeEv"), (uintptr_t)CCustomBuildingDNPipeline_CreateCustomObjPipe_SkyGfx);

    }

    if(pScreenFog->GetBool() || pGrainEffect->GetBool())
    {
        logger->Info("Post-Effects patch is enabled!");
        pGTASA_MobileEffectsRender =        pGTASA + 0x5B6790 + 0x1;
        aml->Redirect(pGTASA + 0x5B677E + 0x1,   (uintptr_t)MobileEffectsRender_stub);
        SET_TO(pbCCTV,                          aml->GetSym(hGTASA, "_ZN12CPostEffects7m_bCCTVE"));
        SET_TO(RenderCCTVPostEffect,            aml->GetSym(hGTASA, "_ZN12CPostEffects4CCTVEv"));

        if(pGrainEffect->GetBool())
        {
            HOOKPLT(InitialisePostEffects,      pGTASA + 0x672200);
            SET_TO(RwRasterCreate,              aml->GetSym(hGTASA, "_Z14RwRasterCreateiiii"));
            SET_TO(RwRasterLock,                aml->GetSym(hGTASA, "_Z12RwRasterLockP8RwRasterhi"));
            SET_TO(RwRasterUnlock,              aml->GetSym(hGTASA, "_Z14RwRasterUnlockP8RwRaster"));
            SET_TO(ImmediateModeRenderStatesStore, aml->GetSym(hGTASA, "_ZN12CPostEffects30ImmediateModeRenderStatesStoreEv"));
            SET_TO(ImmediateModeRenderStatesSet, aml->GetSym(hGTASA, "_ZN12CPostEffects28ImmediateModeRenderStatesSetEv"));
            SET_TO(ImmediateModeRenderStatesReStore, aml->GetSym(hGTASA, "_ZN12CPostEffects32ImmediateModeRenderStatesReStoreEv"));
            SET_TO(RwRenderStateSet,            aml->GetSym(hGTASA, "_Z16RwRenderStateSet13RwRenderStatePv"));
            SET_TO(DrawQuadSetUVs,              aml->GetSym(hGTASA, "_ZN12CPostEffects14DrawQuadSetUVsEffffffff"));
            SET_TO(PostEffectsDrawQuad,         aml->GetSym(hGTASA, "_ZN12CPostEffects8DrawQuadEffffhhhhP8RwRaster"));
            SET_TO(DrawQuadSetDefaultUVs,       aml->GetSym(hGTASA, "_ZN12CPostEffects21DrawQuadSetDefaultUVsEv"));
            SET_TO(CamNoRain,                   aml->GetSym(hGTASA, "_ZN10CCullZones9CamNoRainEv"));
            SET_TO(PlayerNoRain,                aml->GetSym(hGTASA, "_ZN10CCullZones12PlayerNoRainEv"));
            SET_TO(RsGlobal,                    aml->GetSym(hGTASA, "RsGlobal"));
            SET_TO(pnGrainStrength,             aml->GetSym(hGTASA, "_ZN12CPostEffects15m_grainStrengthE"));
            SET_TO(currArea,                    aml->GetSym(hGTASA, "_ZN5CGame8currAreaE"));
            SET_TO(pbGrainEnable,               aml->GetSym(hGTASA, "_ZN12CPostEffects14m_bGrainEnableE"));
            SET_TO(pbRainEnable,                aml->GetSym(hGTASA, "_ZN12CPostEffects13m_bRainEnableE"));
            SET_TO(pbInCutscene,                aml->GetSym(hGTASA, "_ZN12CPostEffects13m_bInCutsceneE"));
            SET_TO(pbNightVision,               aml->GetSym(hGTASA, "_ZN12CPostEffects14m_bNightVisionE"));
            SET_TO(pbInfraredVision,            aml->GetSym(hGTASA, "_ZN12CPostEffects17m_bInfraredVisionE"));
            SET_TO(pfWeatherRain,               aml->GetSym(hGTASA, "_ZN8CWeather4RainE"));
            SET_TO(pfWeatherUnderwaterness,     aml->GetSym(hGTASA, "_ZN8CWeather14UnderWaternessE"));
            SET_TO(effects_TheCamera,           aml->GetSym(hGTASA, "TheCamera"));
            if(sautils != NULL) sautils->AddSliderItem(SetType_Display, "Grain Intensity", pGrainIntensityPercent->GetInt(), 0, 100, GrainIntensityChanged, NULL, NULL);
        }
        if(pScreenFog->GetBool())
        {
            SET_TO(pbFog,                       aml->GetSym(hGTASA, "_ZN12CPostEffects6m_bFogE"));
            SET_TO(RenderScreenFogPostEffect,   aml->GetSym(hGTASA, "_ZN12CPostEffects3FogEv"));
        }
    }

    if(pWaterFog->GetBool() || pMovingFog->GetBool() || pVolumetricClouds->GetBool())
    {
        logger->Info("Effects patch is enabled!");
        pGTASA_EffectsRender =              pGTASA + 0x3F63A6 + 0x1;
        SET_TO(pdword_952880,                   pGTASA + 0x952880);
        SET_TO(pg_fx,                           aml->GetSym(hGTASA, "g_fx"));
        SET_TO(RenderFx,                        aml->GetSym(hGTASA, "_ZN4Fx_c6RenderEP8RwCamerah"));
        SET_TO(RenderWaterCannons,              aml->GetSym(hGTASA, "_ZN13CWaterCannons6RenderEv"));
        aml->Redirect(pGTASA + 0x3F638C + 0x1,   (uintptr_t)EffectsRender_stub);

        if(pWaterFog->GetBool())
        {
            aml->Redirect(pGTASA + 0x599FB4 + 0x1, (uintptr_t)SetUpWaterFog_stub);
            void** ms_WaterFog_New = new void*[pWaterFogBlocksLimits->GetInt()];
            aml->Write(pGTASA + 0x6779A0,   (uintptr_t)&ms_WaterFog_New, sizeof(void*));
            SET_TO(RenderWaterFog,              aml->GetSym(hGTASA, "_ZN11CWaterLevel14RenderWaterFogEv"));
        }
        if(pMovingFog->GetBool())
        {
            SET_TO(RenderMovingFog,             aml->GetSym(hGTASA, "_ZN7CClouds15MovingFogRenderEv"));
        }
        if(pVolumetricClouds->GetBool())
        {
            SET_TO(RenderVolumetricClouds,      aml->GetSym(hGTASA, "_ZN7CClouds22VolumetricCloudsRenderEv"));
        }
    }

    // Shaders
    if(pPS2Shading->GetBool() || pPS2Reflections->GetBool() || pDisablePedSpec->GetBool())
    {
        logger->Info("Shader patches are active!");
        PatchShaders();
    }

    // CPlantSurfPropMgr
    if(pPlantsDatLoading->GetBool() || pProcobjDatLoading->GetBool())
    {
        logger->Info("procobj.dat and/or plants.dat loading patch is enabled!");
        SET_TO(FileMgrSetDir,                   aml->GetSym(hGTASA, "_ZN8CFileMgr6SetDirEPKc"));
        SET_TO(FileMgrOpenFile,                 aml->GetSym(hGTASA, "_ZN8CFileMgr8OpenFileEPKcS1_"));
        SET_TO(FileMgrCloseFile,                aml->GetSym(hGTASA, "_ZN8CFileMgr9CloseFileEj"));
        SET_TO(FileLoaderLoadLine,              aml->GetSym(hGTASA, "_ZN11CFileLoader8LoadLineEj"));
        SET_TO(GetSurfaceIdFromName,            aml->GetSym(hGTASA, "_ZN14SurfaceInfos_c20GetSurfaceIdFromNameEPc"));
        SET_TO(m_SurfPropPtrTab,                aml->GetSym(hGTASA, "_ZN17CPlantSurfPropMgr16m_SurfPropPtrTabE"));
        SET_TO(m_countSurfPropsAllocated,       aml->GetSym(hGTASA, "_ZN17CPlantSurfPropMgr25m_countSurfPropsAllocatedE"));
        SET_TO(m_SurfPropTab,                   aml->GetSym(hGTASA, "_ZN17CPlantSurfPropMgr13m_SurfPropTabE"));
        HOOK(PlantSurfPropMgrInit,              aml->GetSym(hGTASA, "_ZN17CPlantSurfPropMgr10InitialiseEv"));

        if(pPlantsDatLoading->GetBool())
        {
            SET_TO(StreamingMakeSpaceFor,       aml->GetSym(hGTASA, "_ZN10CStreaming12MakeSpaceForEi"));

            SET_TO(LoadTextureDB,               aml->GetSym(hGTASA, "_ZN22TextureDatabaseRuntime4LoadEPKcb21TextureDatabaseFormat"));
            SET_TO(GetTextureDB,                aml->GetSym(hGTASA, "_ZN22TextureDatabaseRuntime11GetDatabaseEPKc"));
            SET_TO(RegisterTextureDB,           aml->GetSym(hGTASA, "_ZN22TextureDatabaseRuntime8RegisterEPS_"));
            SET_TO(UnregisterTextureDB,         aml->GetSym(hGTASA, "_ZN22TextureDatabaseRuntime10UnregisterEPS_"));
            SET_TO(GetTextureFromTextureDB,     aml->GetSym(hGTASA, "_ZN22TextureDatabaseRuntime10GetTextureEPKc"));
            SET_TO(AddImageToList,              aml->GetSym(hGTASA, "_ZN10CStreaming14AddImageToListEPKcb"));
            SET_TO(SetPlantModelsTab,           aml->GetSym(hGTASA, "_ZN14CGrassRenderer17SetPlantModelsTabEjPP8RpAtomic"));
            SET_TO(SetCloseFarAlphaDist,        aml->GetSym(hGTASA, "_ZN14CGrassRenderer20SetCloseFarAlphaDistEff"));
            SET_TO(FlushTriPlantBuffer,         aml->GetSym(hGTASA, "_ZN14CGrassRenderer19FlushTriPlantBufferEv"));
            SET_TO(SetAmbientColours,           aml->GetSym(hGTASA, "_Z17SetAmbientColoursP10RwRGBAReal"));
            SET_TO(DeActivateDirectional,       aml->GetSym(hGTASA, "_Z21DeActivateDirectionalv"));

            SET_TO(PC_PlantSlotTextureTab,      aml->GetSym(hGTASA, "_ZN9CPlantMgr22PC_PlantSlotTextureTabE"));
            SET_TO(PC_PlantTextureTab0,         aml->GetSym(hGTASA, "_ZN9CPlantMgr19PC_PlantTextureTab0E"));
            SET_TO(PC_PlantTextureTab1,         aml->GetSym(hGTASA, "_ZN9CPlantMgr19PC_PlantTextureTab1E"));
            SET_TO(PC_PlantModelSlotTab,        aml->GetSym(hGTASA, "_ZN9CPlantMgr20PC_PlantModelSlotTabE"));
            SET_TO(PC_PlantModelsTab0,          aml->GetSym(hGTASA, "_ZN9CPlantMgr18PC_PlantModelsTab0E"));
            SET_TO(PC_PlantModelsTab1,          aml->GetSym(hGTASA, "_ZN9CPlantMgr18PC_PlantModelsTab1E"));
            SET_TO(ms_pColPool,                 aml->GetSym(hGTASA, "_ZN9CColStore11ms_pColPoolE"));

            SET_TO(InitColStore,                aml->GetSym(hGTASA, "_ZN9CColStore10InitialiseEv"));
            SET_TO(RwStreamOpen,                aml->GetSym(hGTASA, "_Z12RwStreamOpen12RwStreamType18RwStreamAccessTypePKv"));
            SET_TO(RwStreamFindChunk,           aml->GetSym(hGTASA, "_Z17RwStreamFindChunkP8RwStreamjPjS1_"));
            SET_TO(RpClumpStreamRead,           aml->GetSym(hGTASA, "_Z17RpClumpStreamReadP8RwStream"));
            SET_TO(RwStreamClose,               aml->GetSym(hGTASA, "_Z13RwStreamCloseP8RwStreamPv"));
            SET_TO(GetFirstAtomic,              aml->GetSym(hGTASA, "_Z14GetFirstAtomicP7RpClump"));
            SET_TO(SetFilterModeOnAtomicsTextures, aml->GetSym(hGTASA, "_Z30SetFilterModeOnAtomicsTexturesP8RpAtomic19RwTextureFilterMode"));
            SET_TO(RpGeometryLock,              aml->GetSym(hGTASA, "_Z14RpGeometryLockP10RpGeometryi"));
            SET_TO(RpGeometryUnlock,            aml->GetSym(hGTASA, "_Z16RpGeometryUnlockP10RpGeometry"));
            SET_TO(RpGeometryForAllMaterials,   aml->GetSym(hGTASA, "_Z25RpGeometryForAllMaterialsP10RpGeometryPFP10RpMaterialS2_PvES3_"));
            SET_TO(RpMaterialSetTexture,        aml->GetSym(hGTASA, "_Z20RpMaterialSetTextureP10RpMaterialP9RwTexture"));
            SET_TO(RpAtomicClone,               aml->GetSym(hGTASA, "_Z13RpAtomicCloneP8RpAtomic"));
            SET_TO(RpClumpDestroy,              aml->GetSym(hGTASA, "_Z14RpClumpDestroyP7RpClump"));
            SET_TO(RwFrameCreate,               aml->GetSym(hGTASA, "_Z13RwFrameCreatev"));
            SET_TO(RpAtomicSetFrame,            aml->GetSym(hGTASA, "_Z16RpAtomicSetFrameP8RpAtomicP7RwFrame"));
            SET_TO(RenderAtomicWithAlpha,       aml->GetSym(hGTASA, "_ZN18CVisibilityPlugins21RenderAtomicWithAlphaEP8RpAtomici"));
            SET_TO(RpGeometryCreate,            aml->GetSym(hGTASA, "_Z16RpGeometryCreateiij"));
            SET_TO(RpGeometryTriangleGetMaterial, aml->GetSym(hGTASA, "_Z29RpGeometryTriangleGetMaterialPK10RpGeometryPK10RpTriangle"));
            SET_TO(RpGeometryTriangleSetMaterial, aml->GetSym(hGTASA, "_Z29RpGeometryTriangleSetMaterialP10RpGeometryP10RpTriangleP10RpMaterial"));
            SET_TO(RpAtomicSetGeometry,         aml->GetSym(hGTASA, "_Z19RpAtomicSetGeometryP8RpAtomicP10RpGeometryj"));

            SET_TO(PlantMgr_rwOpenGLSetRenderState, aml->GetSym(hGTASA, "_Z23_rwOpenGLSetRenderState13RwRenderStatePv"));
            SET_TO(IsSphereVisibleForCamera,    aml->GetSym(hGTASA, "_ZN7CCamera15IsSphereVisibleERK7CVectorf"));
            SET_TO(AddTriPlant,                 aml->GetSym(hGTASA, "_ZN14CGrassRenderer11AddTriPlantEP10PPTriPlantj"));
            SET_TO(MoveLocTriToList,            aml->GetSym(hGTASA, "_ZN9CPlantMgr16MoveLocTriToListEPP12CPlantLocTriS2_S1_"));
            SET_TO(PlantMgr_TheCamera,          aml->GetSym(hGTASA, "TheCamera"));
            SET_TO(m_CloseLocTriListHead,       aml->GetSym(hGTASA, "_ZN9CPlantMgr21m_CloseLocTriListHeadE"));
            SET_TO(m_UnusedLocTriListHead,      aml->GetSym(hGTASA, "_ZN9CPlantMgr22m_UnusedLocTriListHeadE"));

            HOOKPLT(PlantMgrInit,               pGTASA + 0x673C90);
            HOOKPLT(PlantMgrRender,             pGTASA + 0x6726D0);
            HOOKPLT(PlantLocTriAdd,             pGTASA + 0x675504);
            
            aml->Redirect(pGTASA + 0x222DB2 + 0x1, (uintptr_t)OGLSomething_Patch);
            //aml->Redirect(pGTASA + 0x222DB2 + 0x1, pGTASA + 0x222E04 + 0x1);
        }
    }

    //if(cfg->Bind("BumpShadowsLimit", false, "Shadows")->GetBool())
    //{
    //    RTShadows();
    //}
}
