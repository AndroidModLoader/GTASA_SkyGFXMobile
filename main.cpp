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
MYMODCFG(net.rusjj.gtasa.skygfx, SkyGfx Mobile Beta, 0.1, aap & TheOfficialFloW & RusJJ)
NEEDGAME(com.rockstargames.gtasa)

/* Config */
//ConfigEntry* pBonesOptimization;
//ConfigEntry* pMVPOptimization;
ConfigEntry* pDisablePedSpec;
ConfigEntry* pPS2Sun;
ConfigEntry* pPS2Shading;
ConfigEntry* pPS2Reflections;
ConfigEntry* pColorfilter;
ConfigEntry* pDisableDetailTexture;
ConfigEntry* pFixGreenDetailTexture;
ConfigEntry* pFixMipMaps;
ConfigEntry* pDontShadeUnexploredMap;
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

/* Patch Saves */
//const uint32_t sunCoronaRet = 0xF0F7ECE0;
const uint32_t sunCoronaDel = 0xBF00BF00;

/* Saves */
void* pGTASA = NULL;
uintptr_t pGTASAAddr = 0;

////////////////////////////////////////////////////////////////////////
// Was taken from TheOfficialFloW's git repo (will be in AML 1.0.0.6) //
////////////////////////////////////////////////////////////////////////
void Redirect(uintptr_t addr, uintptr_t to)
{
    if(!addr) return;
    uint32_t hook[2] = {0xE51FF004, to};
    if(addr & 1)
    {
        addr &= ~1;
        if (addr & 2)
        {
            aml->PlaceNOP(addr, 1); 
            addr += 2;
        }
        hook[0] = 0xF000F8DF;
    }
    aml->Write(addr, (uintptr_t)hook, sizeof(hook));
}
////////////////////////////////////////////////////////////////////////
// Was taken from TheOfficialFloW's git repo (will be in AML 1.0.0.6) //
////////////////////////////////////////////////////////////////////////

extern DECL_HOOKv(PlantMgrInit);
extern DECL_HOOKv(PlantMgrRender);
extern DECL_HOOKv(PlantSurfPropMgrInit);

DECL_HOOKv(InitRW)
{
    logger->Info("Initializing RW...");
    InitRW();
}

#define GREEN_TEXTURE_ID 14
inline void* GetDetailTexturePtr(int texId)
{
    return *(void**)(**(int**)(*detailTexturesStorage + 4 * (texId-1)) + *RasterExtOffset);
}

DECL_HOOKv(emu_TextureSetDetailTexture, void* texture, unsigned int tilingScale)
{
    if(texture == NULL)
    {
        emu_TextureSetDetailTexture(NULL, 0);
        return;
    }
    if(texture == GetDetailTexturePtr(GREEN_TEXTURE_ID))
    {
        *textureDetail = 0;
        emu_TextureSetDetailTexture(NULL, 0);
        return;
    }
    emu_TextureSetDetailTexture(texture, tilingScale);
    *textureDetail = 1;
}

DECL_HOOKv(glCompressedTex2D, GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
    if (level == 0 || (width >= 4 && height >= 4) || (format != 0x8C01 && format != 0x8C02))
        glCompressedTex2D(target, level, format, width, height, border, imageSize, data);
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

void PedShadowDistanceChanged(int oldVal, int newVal)
{
    pPedShadowDistance->SetFloat(0.01f * newVal);
    *(float*)(pGTASAAddr + 0x5BD2FC) = pPedShadowDistance->GetFloat();
    cfg->Save();
}
const char* PedShadowDistanceDraw(int nNewValue)
{
    static char ret[12];
    sprintf(ret, "%.2fm", 0.01f * nNewValue);
    return ret;
}

void GrainIntensityChanged(int oldVal, int newVal)
{
    pGrainIntensityPercent->SetInt(newVal);
    grainIntensity = newVal * 2.55f;
    cfg->Save();
}

void VehicleShadowDistanceChanged(int oldVal, int newVal)
{
    pVehicleShadowDistance->SetFloat(0.01f * newVal);
    *(float*)(pGTASAAddr + 0x5B9AE0) = pVehicleShadowDistance->GetFloat();
    cfg->Save();
}
const char* VehicleShadowDistanceDraw(int nNewValue)
{
    static char ret[12];
    sprintf(ret, "%.2fm", 0.01f * nNewValue);
    return ret;
}

DECL_HOOKv(PlantLocTriAdd)
{
    logger->Info("PlantLocTriAdd");
}

extern "C" void OnModLoad()
{
    logger->SetTag("SkyGfx Mobile");
// Addresses
    pGTASA = dlopen("libGTASA.so", RTLD_LAZY);
    pGTASAAddr = aml->GetLib("libGTASA.so");
    sautils = (ISAUtils*)GetInterface("SAUtils");
// Config
    // Config: Render
    //pBonesOptimization = cfg->Bind("BonesOptim", false);
    //pMVPOptimization = cfg->Bind("MVPOptim", false);
    pDisablePedSpec = cfg->Bind("DisablePedSpec", true, "Render");
    pPS2Sun = cfg->Bind("PS2_Sun", true, "Render");
    pPS2Shading = cfg->Bind("PS2_Shading", true, "Render");
    pPS2Reflections = cfg->Bind("PS2_Reflections", true, "Render");
    pColorfilter = cfg->Bind("Colorfilter", "ps2", "Render");
    pDisableDetailTexture = cfg->Bind("DisableDetailTexture", false, "Render");
    pFixGreenDetailTexture = cfg->Bind("FixGreenDetailTexture", true, "Render");
    pFixMipMaps = cfg->Bind("FixMipMaps", false, "Render");
    pDontShadeUnexploredMap = cfg->Bind("DontShadeUnexploredMap", false, "Render");
    pDisableClouds = cfg->Bind("DisableClouds", false, "Render");
    pPS2Pipeline = cfg->Bind("PS2_Pipeline", true, "Render");
    pPS2PipelineRenderWay = cfg->Bind("PS2_Pipeline_RenderWay", DPWay_Everything, "Render");
    pPlantsDatLoading = cfg->Bind("PlantsDatLoading", false, "Render");
    pProcobjDatLoading = cfg->Bind("ProcObjDatLoading", true, "Render");
    pExponentialFog = cfg->Bind("ExponentialFog", true, "Render");
    pDisableCamnorm = cfg->Bind("DisableCamnorm", false, "Render");

    if(pPS2PipelineRenderWay->GetInt() <= 0) pipelineWay = DPWay_Default;
    if(pPS2PipelineRenderWay->GetInt() >= DPWay_MaxWays) pipelineWay = DPWay_Everything;
    else pipelineWay = (ePipelineDualpassWay)pPS2PipelineRenderWay->GetInt();

    if(pDisableCamnorm->GetBool()) aml->PlaceNOP(pGTASAAddr + 0x1E7C6C, 9);

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
    //aml->Unprot(pGTASAAddr + 0x5BD2FC, sizeof(float));
    //pPedShadowDistance = cfg->Bind("PedShadowDistance", *(float*)(pGTASAAddr + 0x5BD2FC), "Shadows"); *(float*)(pGTASAAddr + 0x5BD2FC) = pPedShadowDistance->GetFloat();
    //if(sautils != NULL) sautils->AddSliderItem(Display, "Ped Shadow Distance", 100 * pPedShadowDistance->GetFloat(), 100 * 4.0f, 100 * 256.0f, PedShadowDistanceChanged, PedShadowDistanceDraw);
    //aml->Unprot(pGTASAAddr + 0x5B9AE0, sizeof(float));
    //pVehicleShadowDistance = cfg->Bind("VehicleShadowDistance", *(float*)(pGTASAAddr + 0x5B9AE0), "Shadows"); *(float*)(pGTASAAddr + 0x5B9AE0) = pVehicleShadowDistance->GetFloat();
    //if(sautils != NULL) sautils->AddSliderItem(Display, "Vehicle Shadow Distance", 100 * pVehicleShadowDistance->GetFloat(), 100 * 4.0f, 100 * 256.0f, VehicleShadowDistanceChanged, VehicleShadowDistanceDraw);

    pMaxRTShadows = cfg->Bind("MaxRTShadows", 64, "Shadows"); // Default value is 40
    pDynamicObjectsShadows = cfg->Bind("DynamicObjectsShadows", false, "Shadows");
    pAllowPlayerClassicShadow = cfg->Bind("AllowPlayerClassicShadow", false, "Shadows");

    // Config: Information
    cfg->Bind("About_PS2_Reflections", "Works only on low reflections setting with PS2 Shading enabled", "?Information");
    cfg->Bind("Colorfilter_Values", "default none ps2 pc", "?Information");

    HOOKPLT(InitRW, pGTASAAddr + 0x66F2D0);

// Patches
    // Patches: Shading
    if(pPS2Shading->GetBool())
    {
        logger->Info("PS2 Shading enabled!");
        aml->PlaceNOP(pGTASAAddr + 0x1C1382);
        aml->PlaceNOP(pGTASAAddr + 0x1C13BA);
        SET_TO(RwFrameTransform,                aml->GetSym(pGTASA, "_Z16RwFrameTransformP7RwFramePK11RwMatrixTag15RwOpCombineType"));
        SET_TO(RpLightSetColor,                 aml->GetSym(pGTASA, "_Z15RpLightSetColorP7RpLightPK10RwRGBAReal"));
        SET_TO(p_pAmbient,                      aml->GetSym(pGTASA, "pAmbient"));
        SET_TO(p_pDirect,                       aml->GetSym(pGTASA, "pDirect"));
        SET_TO(p_AmbientLightColourForFrame,    aml->GetSym(pGTASA, "AmbientLightColourForFrame"));
        SET_TO(p_AmbientLightColourForFrame_PedsCarsAndObjects, aml->GetSym(pGTASA, "AmbientLightColourForFrame_PedsCarsAndObjects"));
        SET_TO(p_DirectionalLightColourForFrame,aml->GetSym(pGTASA, "DirectionalLightColourForFrame"));
        SET_TO(p_DirectionalLightColourFromDay, aml->GetSym(pGTASA, "DirectionalLightColourFromDay"));
        SET_TO(p_CTimeCycle__m_CurrentColours,  aml->GetSym(pGTASA, "_ZN10CTimeCycle16m_CurrentColoursE"));
        SET_TO(p_CTimeCycle__m_vecDirnLightToSun, aml->GetSym(pGTASA, "_ZN10CTimeCycle19m_vecDirnLightToSunE"));
        SET_TO(p_gfLaRiotsLightMult,            aml->GetSym(pGTASA, "gfLaRiotsLightMult"));
        SET_TO(p_CCoronas__LightsMult,          aml->GetSym(pGTASA, "_ZN8CCoronas10LightsMultE"));
        SET_TO(p_CWeather__LightningFlash,      aml->GetSym(pGTASA, "_ZN8CWeather14LightningFlashE"));
        SET_TO(openglAmbientLight,              aml->GetSym(pGTASA, "openglAmbientLight"));
        SET_TO(p_rwOpenGLColorMaterialEnabled,  aml->GetSym(pGTASA, "_rwOpenGLColorMaterialEnabled"));
        SET_TO(emu_glLightModelfv,              aml->GetSym(pGTASA, "_Z18emu_glLightModelfvjPKf"));
        SET_TO(emu_glMaterialfv,                aml->GetSym(pGTASA, "_Z16emu_glMaterialfvjjPKf"));
        SET_TO(emu_glColorMaterial,             aml->GetSym(pGTASA, "_Z19emu_glColorMaterialjj"));
        SET_TO(emu_glEnable,                    aml->GetSym(pGTASA, "_Z12emu_glEnablej"));
        SET_TO(emu_glDisable,                   aml->GetSym(pGTASA, "_Z13emu_glDisablej"));
        Redirect(aml->GetSym(pGTASA, "_Z36_rwOpenGLLightsSetMaterialPropertiesPK10RpMaterialj"), (uintptr_t)_rwOpenGLLightsSetMaterialProperties);
        Redirect(aml->GetSym(pGTASA, "_Z28SetLightsWithTimeOfDayColourP7RpWorld"), (uintptr_t)SetLightsWithTimeOfDayColour);
        // Bones Optim (for SkyGfx shaders only, not yet, doesnt draw ped models)
        /*if(pBonesOptimization->GetBool())
        {
            SET_TO(skin_map,                    aml->GetSym(pGTASA, "skin_map"));
            SET_TO(skin_dirty,                  aml->GetSym(pGTASA, "skin_dirty"));
            SET_TO(skin_num,                    aml->GetSym(pGTASA, "skin_num"));
            aml->Unprot((uintptr_t)skin_map, 0x1800);
            Redirect(aml->GetSym(pGTASA, "_Z30emu_InternalSkinGetVectorCountv"), (uintptr_t)emu_InternalSkinGetVectorCount);
            Redirect(pGTASAAddr + 0x1C8670 + 0x1, (uintptr_t)SkinSetMatrices);
        }*/
    }

    // PS2 Sun Corona
    if(pPS2Sun->GetBool())
    {
        logger->Info("PS2 Sun Corona enabled!");
        aml->Unprot(pGTASAAddr + 0x5A26B0, 4);
        *(uint32_t*)(pGTASAAddr + 0x5A26B0) = sunCoronaDel;
    }

    // Detail textures
    if(pDisableDetailTexture->GetBool())
    {
        logger->Info("Detail textures are disabled!");
        aml->PlaceRET(pGTASAAddr + 0x1BCBC4);
    }
    else if(pFixGreenDetailTexture->GetBool())
    {
        logger->Info("Fixing a green detail texture is enabled!");
        aml->PlaceNOP(pGTASAAddr + 0x1B00B0, 5); // Dont set textureDetail variable! We'll handle it by ourselves!
        SET_TO(detailTexturesStorage,           aml->GetSym(pGTASA, "_ZN22TextureDatabaseRuntime14detailTexturesE") + 8); // pGTASAAddr + 0x6BD1D8
        SET_TO(textureDetail,                   aml->GetSym(pGTASA, "textureDetail"));
        SET_TO(RasterExtOffset,                 aml->GetSym(pGTASA, "RasterExtOffset"));
        HOOK(emu_TextureSetDetailTexture,       aml->GetSym(pGTASA, "_Z27emu_TextureSetDetailTexturePvj"));
    }

    // Color Filter
    if(!strcmp(pColorfilter->GetString(), "none")) nColorFilter = 1;
    else if(!strcmp(pColorfilter->GetString(), "ps2")) nColorFilter = 2;
    else if(!strcmp(pColorfilter->GetString(), "pc")) nColorFilter = 3;
    if(nColorFilter != 0 || sautils != NULL)
    {
        logger->Info("Colorfilter \"%s\" enabled!", pColorfilter->GetString());
        pGTASAAddr_Colorfilter =                pGTASAAddr + 0x5B6444 + 0x1;
        Redirect(pGTASAAddr + 0x5B643C + 0x1, (uintptr_t)ColorFilter_stub);
        aml->Unprot(pGTASAAddr + 0x5B6444, sizeof(uint16_t));
        memcpy((void *)(pGTASAAddr + 0x5B6444), (void *)(pGTASAAddr + 0x5B63DC), sizeof(uint16_t));
        aml->Unprot(pGTASAAddr + 0x5B6446, sizeof(uint16_t));
        memcpy((void *)(pGTASAAddr + 0x5B6446), (void *)(pGTASAAddr + 0x5B63EA), sizeof(uint16_t));

        if(sautils != NULL) sautils->AddClickableItem(Display, "Colorfilter", nColorFilter, 0, sizeofA(pColorFilterSettings)-1, pColorFilterSettings, ColorfilterChanged);
    }

    // Mipmaps
    if(pFixMipMaps->GetBool())
    {
        logger->Info("MipMaps fix is enabled!");
        HOOKPLT(glCompressedTex2D, pGTASAAddr + 0x674838);
    }

    // Unexplored map shading
    if(pDontShadeUnexploredMap->GetBool())
    {
        logger->Info("Unexplored map sectors shading disabled!");
        Redirect(pGTASAAddr + 0x2AADE0 + 0x1, pGTASAAddr + 0x2AAF9A + 0x1);
    }

    if(pDisableClouds->GetBool())
    {
        logger->Info("Clouds are disabled!");
        Redirect(pGTASAAddr + 0x59F340 + 0x1, pGTASAAddr + 0x59F40A + 0x1);
    }

    if(pPS2Pipeline->GetBool())
    {
        logger->Info("PS2 Pipeline is enabled!");

        SET_TO(_rxPipelineDestroy,              aml->GetSym(pGTASA, "_Z18_rxPipelineDestroyP10RxPipeline"));
        SET_TO(RxPipelineCreate,                aml->GetSym(pGTASA, "_Z16RxPipelineCreatev"));
        SET_TO(RxPipelineLock,                  aml->GetSym(pGTASA, "_Z14RxPipelineLockP10RxPipeline"));
        SET_TO(RxNodeDefinitionGetOpenGLAtomicAllInOne, aml->GetSym(pGTASA, "_Z39RxNodeDefinitionGetOpenGLAtomicAllInOnev"));
        SET_TO(RxLockedPipeAddFragment,         aml->GetSym(pGTASA, "_Z23RxLockedPipeAddFragmentP10RxPipelinePjP16RxNodeDefinitionz"));
        SET_TO(RxLockedPipeUnlock,              aml->GetSym(pGTASA, "_Z18RxLockedPipeUnlockP10RxPipeline"));
        SET_TO(RxPipelineFindNodeByName,        aml->GetSym(pGTASA, "_Z24RxPipelineFindNodeByNameP10RxPipelinePKcP14RxPipelineNodePi"));
        SET_TO(RxOpenGLAllInOneSetInstanceCallBack, aml->GetSym(pGTASA, "_Z35RxOpenGLAllInOneSetInstanceCallBackP14RxPipelineNodePFiPvP24RxOpenGLMeshInstanceDataiiE"));
        SET_TO(RxOpenGLAllInOneSetRenderCallBack, aml->GetSym(pGTASA, "_Z33RxOpenGLAllInOneSetRenderCallBackP14RxPipelineNodePFvP10RwResEntryPvhjE"));
        SET_TO(_rwOpenGLSetRenderState,         aml->GetSym(pGTASA, "_Z23_rwOpenGLSetRenderState13RwRenderStatePv"));
        SET_TO(_rwOpenGLGetRenderState,         aml->GetSym(pGTASA, "_Z23_rwOpenGLGetRenderState13RwRenderStatePv"));
        SET_TO(_rwOpenGLSetRenderStateNoExtras, aml->GetSym(pGTASA, "_Z31_rwOpenGLSetRenderStateNoExtras13RwRenderStatePv"));
        SET_TO(_rwOpenGLLightsSetMaterialPropertiesORG, aml->GetSym(pGTASA, "_Z36_rwOpenGLLightsSetMaterialPropertiesPK10RpMaterialj"));
        SET_TO(SetNormalMatrix,                 aml->GetSym(pGTASA, "_Z15SetNormalMatrixff5RwV2d"));
        SET_TO(DrawStoredMeshData,              aml->GetSym(pGTASA, "_ZN24RxOpenGLMeshInstanceData10DrawStoredEv"));
        SET_TO(ResetEnvMap,                     aml->GetSym(pGTASA, "_Z11ResetEnvMapv"));

        SET_TO(m_fDNBalanceParam,               aml->GetSym(pGTASA, "_ZN25CCustomBuildingDNPipeline17m_fDNBalanceParamE"));
        SET_TO(rwOpenGLOpaqueBlack,             aml->GetSym(pGTASA, "_rwOpenGLOpaqueBlack"));
        SET_TO(rwOpenGLLightingEnabled,         aml->GetSym(pGTASA, "_rwOpenGLLightingEnabled"));
        SET_TO(rwOpenGLColorMaterialEnabled,    aml->GetSym(pGTASA, "_rwOpenGLColorMaterialEnabled"));
        SET_TO(ms_envMapPluginOffset,           aml->GetSym(pGTASA, "_ZN24CCustomCarEnvMapPipeline21ms_envMapPluginOffsetE"));
        SET_TO(ppline_RasterExtOffset,          aml->GetSym(pGTASA, "RasterExtOffset"));
        SET_TO(byte_70BF3C,                     pGTASAAddr + 0x70BF3C);

        SET_TO(ppline_SetSecondVertexColor,     aml->GetSym(pGTASA, "_Z24emu_SetSecondVertexColorhf"));
        SET_TO(ppline_EnableAlphaModulate,      aml->GetSym(pGTASA, "_Z23emu_EnableAlphaModulatef"));
        SET_TO(ppline_DisableAlphaModulate,     aml->GetSym(pGTASA, "_Z24emu_DisableAlphaModulatev"));
        SET_TO(ppline_glDisable,                aml->GetSym(pGTASA, "_Z13emu_glDisablej"));
        SET_TO(ppline_glColor4fv,               aml->GetSym(pGTASA, "_Z14emu_glColor4fvPKf"));
        SET_TO(ppline_SetEnvMap,                aml->GetSym(pGTASA, "_Z13emu_SetEnvMapPvfi"));
        if(sautils != NULL) sautils->AddClickableItem(Display, "Building DualPass", pipelineWay, 0, sizeofA(pPipelineSettings)-1, pPipelineSettings, PipelineChanged);

        Redirect(aml->GetSym(pGTASA, "_ZN25CCustomBuildingDNPipeline19CreateCustomObjPipeEv"), (uintptr_t)CCustomBuildingDNPipeline_CreateCustomObjPipe_SkyGfx);

    }

    if(pScreenFog->GetBool() || pGrainEffect->GetBool())
    {
        logger->Info("Post-Effects patch is enabled!");
        pGTASAAddr_MobileEffectsRender =        pGTASAAddr + 0x5B6790 + 0x1;
        Redirect(pGTASAAddr + 0x5B677E + 0x1,   (uintptr_t)MobileEffectsRender_stub);
        SET_TO(pbCCTV,                          aml->GetSym(pGTASA, "_ZN12CPostEffects7m_bCCTVE"));
        SET_TO(RenderCCTVPostEffect,            aml->GetSym(pGTASA, "_ZN12CPostEffects4CCTVEv"));

        if(pGrainEffect->GetBool())
        {
            HOOKPLT(InitialisePostEffects,      pGTASAAddr + 0x672200);
            SET_TO(RwRasterCreate,              aml->GetSym(pGTASA, "_Z14RwRasterCreateiiii"));
            SET_TO(RwRasterLock,                aml->GetSym(pGTASA, "_Z12RwRasterLockP8RwRasterhi"));
            SET_TO(RwRasterUnlock,              aml->GetSym(pGTASA, "_Z14RwRasterUnlockP8RwRaster"));
            SET_TO(ImmediateModeRenderStatesStore, aml->GetSym(pGTASA, "_ZN12CPostEffects30ImmediateModeRenderStatesStoreEv"));
            SET_TO(ImmediateModeRenderStatesSet, aml->GetSym(pGTASA, "_ZN12CPostEffects28ImmediateModeRenderStatesSetEv"));
            SET_TO(ImmediateModeRenderStatesReStore, aml->GetSym(pGTASA, "_ZN12CPostEffects32ImmediateModeRenderStatesReStoreEv"));
            SET_TO(RwRenderStateSet,            aml->GetSym(pGTASA, "_Z16RwRenderStateSet13RwRenderStatePv"));
            SET_TO(DrawQuadSetUVs,              aml->GetSym(pGTASA, "_ZN12CPostEffects14DrawQuadSetUVsEffffffff"));
            SET_TO(PostEffectsDrawQuad,         aml->GetSym(pGTASA, "_ZN12CPostEffects8DrawQuadEffffhhhhP8RwRaster"));
            SET_TO(DrawQuadSetDefaultUVs,       aml->GetSym(pGTASA, "_ZN12CPostEffects21DrawQuadSetDefaultUVsEv"));
            SET_TO(CamNoRain,                   aml->GetSym(pGTASA, "_ZN10CCullZones9CamNoRainEv"));
            SET_TO(PlayerNoRain,                aml->GetSym(pGTASA, "_ZN10CCullZones12PlayerNoRainEv"));
            SET_TO(RsGlobal,                    aml->GetSym(pGTASA, "RsGlobal"));
            SET_TO(pnGrainStrength,             aml->GetSym(pGTASA, "_ZN12CPostEffects15m_grainStrengthE"));
            SET_TO(currArea,                    aml->GetSym(pGTASA, "_ZN5CGame8currAreaE"));
            SET_TO(pbGrainEnable,               aml->GetSym(pGTASA, "_ZN12CPostEffects14m_bGrainEnableE"));
            SET_TO(pbRainEnable,                aml->GetSym(pGTASA, "_ZN12CPostEffects13m_bRainEnableE"));
            SET_TO(pbInCutscene,                aml->GetSym(pGTASA, "_ZN12CPostEffects13m_bInCutsceneE"));
            SET_TO(pbNightVision,               aml->GetSym(pGTASA, "_ZN12CPostEffects14m_bNightVisionE"));
            SET_TO(pbInfraredVision,            aml->GetSym(pGTASA, "_ZN12CPostEffects17m_bInfraredVisionE"));
            SET_TO(pfWeatherRain,               aml->GetSym(pGTASA, "_ZN8CWeather4RainE"));
            SET_TO(pfWeatherUnderwaterness,     aml->GetSym(pGTASA, "_ZN8CWeather14UnderWaternessE"));
            SET_TO(effects_TheCamera,           aml->GetSym(pGTASA, "TheCamera"));
            if(sautils != NULL) sautils->AddSliderItem(Display, "Grain Intensity", pGrainIntensityPercent->GetInt(), 0, 100, GrainIntensityChanged);
        }
        if(pScreenFog->GetBool())
        {
            SET_TO(pbFog,                       aml->GetSym(pGTASA, "_ZN12CPostEffects6m_bFogE"));
            SET_TO(RenderScreenFogPostEffect,   aml->GetSym(pGTASA, "_ZN12CPostEffects3FogEv"));
        }
    }

    if(pWaterFog->GetBool() || pMovingFog->GetBool() || pVolumetricClouds->GetBool())
    {
        logger->Info("Effects patch is enabled!");
        pGTASAAddr_EffectsRender =              pGTASAAddr + 0x3F63A6 + 0x1;
        SET_TO(pdword_952880,                   pGTASAAddr + 0x952880);
        SET_TO(pg_fx,                           aml->GetSym(pGTASA, "g_fx"));
        SET_TO(RenderFx,                        aml->GetSym(pGTASA, "_ZN4Fx_c6RenderEP8RwCamerah"));
        SET_TO(RenderWaterCannons,              aml->GetSym(pGTASA, "_ZN13CWaterCannons6RenderEv"));
        Redirect(pGTASAAddr + 0x3F638C + 0x1,   (uintptr_t)EffectsRender_stub);

        if(pWaterFog->GetBool())
        {
            Redirect(pGTASAAddr + 0x599FB4 + 0x1, (uintptr_t)SetUpWaterFog_stub);
            void** ms_WaterFog_New = new void*[pWaterFogBlocksLimits->GetInt()];
            aml->Write(pGTASAAddr + 0x6779A0,   (uintptr_t)&ms_WaterFog_New, sizeof(void*));
            SET_TO(RenderWaterFog,              aml->GetSym(pGTASA, "_ZN11CWaterLevel14RenderWaterFogEv"));
        }
        if(pMovingFog->GetBool())
        {
            SET_TO(RenderMovingFog,             aml->GetSym(pGTASA, "_ZN7CClouds15MovingFogRenderEv"));
        }
        if(pVolumetricClouds->GetBool())
        {
            SET_TO(RenderVolumetricClouds,      aml->GetSym(pGTASA, "_ZN7CClouds22VolumetricCloudsRenderEv"));
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
        SET_TO(FileMgrSetDir,                   aml->GetSym(pGTASA, "_ZN8CFileMgr6SetDirEPKc"));
        SET_TO(FileMgrOpenFile,                 aml->GetSym(pGTASA, "_ZN8CFileMgr8OpenFileEPKcS1_"));
        SET_TO(FileMgrCloseFile,                aml->GetSym(pGTASA, "_ZN8CFileMgr9CloseFileEj"));
        SET_TO(FileLoaderLoadLine,              aml->GetSym(pGTASA, "_ZN11CFileLoader8LoadLineEj"));
        SET_TO(GetSurfaceIdFromName,            aml->GetSym(pGTASA, "_ZN14SurfaceInfos_c20GetSurfaceIdFromNameEPc"));
        SET_TO(m_SurfPropPtrTab,                aml->GetSym(pGTASA, "_ZN17CPlantSurfPropMgr16m_SurfPropPtrTabE"));
        SET_TO(m_countSurfPropsAllocated,       aml->GetSym(pGTASA, "_ZN17CPlantSurfPropMgr25m_countSurfPropsAllocatedE"));
        SET_TO(m_SurfPropTab,                   aml->GetSym(pGTASA, "_ZN17CPlantSurfPropMgr13m_SurfPropTabE"));
        HOOK(PlantSurfPropMgrInit,              aml->GetSym(pGTASA, "_ZN17CPlantSurfPropMgr10InitialiseEv"));

        if(pPlantsDatLoading->GetBool())
        {
            SET_TO(StreamingMakeSpaceFor,       aml->GetSym(pGTASA, "_ZN10CStreaming12MakeSpaceForEi"));

            SET_TO(LoadTextureDB,               aml->GetSym(pGTASA, "_ZN22TextureDatabaseRuntime4LoadEPKcb21TextureDatabaseFormat"));
            SET_TO(RegisterTextureDB,           aml->GetSym(pGTASA, "_ZN22TextureDatabaseRuntime8RegisterEPS_"));
            SET_TO(UnregisterTextureDB,         aml->GetSym(pGTASA, "_ZN22TextureDatabaseRuntime10UnregisterEPS_"));
            SET_TO(GetTextureFromTextureDB,     aml->GetSym(pGTASA, "_ZN22TextureDatabaseRuntime10GetTextureEPKc"));
            SET_TO(AddImageToList,              aml->GetSym(pGTASA, "_ZN10CStreaming14AddImageToListEPKcb"));
            SET_TO(SetPlantModelsTab,           aml->GetSym(pGTASA, "_ZN14CGrassRenderer17SetPlantModelsTabEjPP8RpAtomic"));
            SET_TO(SetCloseFarAlphaDist,        aml->GetSym(pGTASA, "_ZN14CGrassRenderer20SetCloseFarAlphaDistEff"));

            SET_TO(PC_PlantSlotTextureTab,      aml->GetSym(pGTASA, "_ZN9CPlantMgr22PC_PlantSlotTextureTabE"));
            SET_TO(PC_PlantTextureTab0,         aml->GetSym(pGTASA, "_ZN9CPlantMgr19PC_PlantTextureTab0E"));
            SET_TO(PC_PlantTextureTab1,         aml->GetSym(pGTASA, "_ZN9CPlantMgr19PC_PlantTextureTab1E"));
            SET_TO(PC_PlantModelSlotTab,        aml->GetSym(pGTASA, "_ZN9CPlantMgr20PC_PlantModelSlotTabE"));
            SET_TO(PC_PlantModelsTab0,          aml->GetSym(pGTASA, "_ZN9CPlantMgr18PC_PlantModelsTab0E"));
            SET_TO(PC_PlantModelsTab1,          aml->GetSym(pGTASA, "_ZN9CPlantMgr18PC_PlantModelsTab1E"));

            SET_TO(RwStreamOpen,                aml->GetSym(pGTASA, "_Z12RwStreamOpen12RwStreamType18RwStreamAccessTypePKv"));
            SET_TO(RwStreamFindChunk,           aml->GetSym(pGTASA, "_Z17RwStreamFindChunkP8RwStreamjPjS1_"));
            SET_TO(RpClumpStreamRead,           aml->GetSym(pGTASA, "_Z17RpClumpStreamReadP8RwStream"));
            SET_TO(RwStreamClose,               aml->GetSym(pGTASA, "_Z13RwStreamCloseP8RwStreamPv"));
            SET_TO(GetFirstAtomic,              aml->GetSym(pGTASA, "_Z14GetFirstAtomicP7RpClump"));
            SET_TO(SetFilterModeOnAtomicsTextures, aml->GetSym(pGTASA, "_Z30SetFilterModeOnAtomicsTexturesP8RpAtomic19RwTextureFilterMode"));
            SET_TO(RpGeometryLock,              aml->GetSym(pGTASA, "_Z14RpGeometryLockP10RpGeometryi"));
            SET_TO(RpGeometryUnlock,            aml->GetSym(pGTASA, "_Z16RpGeometryUnlockP10RpGeometry"));
            SET_TO(RpGeometryForAllMaterials,   aml->GetSym(pGTASA, "_Z25RpGeometryForAllMaterialsP10RpGeometryPFP10RpMaterialS2_PvES3_"));
            SET_TO(RpMaterialSetTexture,        aml->GetSym(pGTASA, "_Z20RpMaterialSetTextureP10RpMaterialP9RwTexture"));
            SET_TO(RpAtomicClone,               aml->GetSym(pGTASA, "_Z13RpAtomicCloneP8RpAtomic"));
            SET_TO(RpClumpDestroy,              aml->GetSym(pGTASA, "_Z14RpClumpDestroyP7RpClump"));
            SET_TO(RwFrameCreate,               aml->GetSym(pGTASA, "_Z13RwFrameCreatev"));
            SET_TO(RpAtomicSetFrame,            aml->GetSym(pGTASA, "_Z16RpAtomicSetFrameP8RpAtomicP7RwFrame"));

            SET_TO(PlantMgr_rwOpenGLSetRenderState, aml->GetSym(pGTASA, "_Z23_rwOpenGLSetRenderState13RwRenderStatePv"));
            SET_TO(IsSphereVisibleForCamera,    aml->GetSym(pGTASA, "_ZN7CCamera15IsSphereVisibleERK7CVectorf"));
            SET_TO(AddTriPlant,                 aml->GetSym(pGTASA, "_ZN14CGrassRenderer11AddTriPlantEP10PPTriPlantj"));
            SET_TO(PlantMgr_TheCamera,          aml->GetSym(pGTASA, "TheCamera"));

            HOOKPLT(PlantMgrInit,               pGTASAAddr + 0x673C90);
            HOOKPLT(PlantMgrRender,             pGTASAAddr + 0x6726D0);
            //HOOKPLT(PlantLocTriAdd,             pGTASAAddr + 0x675504);
        }
    }

    if(cfg->Bind("BumpShadowsLimit", false, "Shadows")->GetBool())
    {
        PatchShadows();
        RTShadows();
    }
}