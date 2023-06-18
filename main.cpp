#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <string> // memset

#include <gtasa_things.h>

// SAUtils
#if __has_include(<isautils.h>)
    #include <isautils.h>
    ISAUtils* sautils = NULL;
#endif

#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))
MYMODCFG(net.rusjj.skygfxmobile, SkyGfx Mobile Beta, 0.2, aap & TheOfficialFloW & RusJJ)
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

uintptr_t GrassMaterialApplying_BackTo;
RpMaterial* SetGrassModelProperties(RpMaterial* material, void* data)
{
    PPTriPlant* plant = (PPTriPlant*)data;
    material->texture = plant->texture_ptr;
    material->color = plant->color;
    //RpMaterialSetTexture(material, plant->texture_ptr); // Deletes previous texture, SUS BEHAVIOUR
    return material;
}
extern "C" void GrassMaterialApplying(RpGeometry* geometry, PPTriPlant* plant)
{
    RpGeometryForAllMaterials(geometry, SetGrassModelProperties, plant);
}
__attribute__((optnone)) __attribute__((naked)) void GrassMaterialApplying_Patch(void)
{
    asm volatile(
        "PUSH {R0-R9}\n"
        "LDR R0, [SP, #0]\n"
        "MOV R1, R10\n"
        "BL GrassMaterialApplying\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0-R9}\n"
        "BX R12\n"
    :: "r" (GrassMaterialApplying_BackTo));
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
    hGTASA = aml->GetLibHandle("libGTASA.so");
    pGTASA = aml->GetLib("libGTASA.so");
    sautils = (ISAUtils*)GetInterface("SAUtils");
    ResolveExternals();
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
    // Patches: Shading
    if(pPS2Shading->GetBool())
    {
        logger->Info("PS2 Shading enabled!");
        aml->PlaceNOP(pGTASA + 0x1C1382);
        aml->PlaceNOP(pGTASA + 0x1C13BA);
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

        //SET_TO(_rwOpenGLSetRenderState,         aml->GetSym(hGTASA, "_Z23_rwOpenGLSetRenderState13RwRenderStatePv"));
        //SET_TO(_rwOpenGLGetRenderState,         aml->GetSym(hGTASA, "_Z23_rwOpenGLGetRenderState13RwRenderStatePv"));
        if(sautils != NULL) sautils->AddClickableItem(SetType_Display, "Building DualPass", pipelineWay, 0, sizeofA(pPipelineSettings)-1, pPipelineSettings, PipelineChanged, NULL);

        aml->Redirect(aml->GetSym(hGTASA, "_ZN25CCustomBuildingDNPipeline19CreateCustomObjPipeEv"), (uintptr_t)CCustomBuildingDNPipeline_CreateCustomObjPipe_SkyGfx);

    }

    if(pScreenFog->GetBool() || pGrainEffect->GetBool())
    {
        logger->Info("Post-Effects patch is enabled!");
        pGTASA_MobileEffectsRender =        pGTASA + 0x5B6790 + 0x1;
        aml->Redirect(pGTASA + 0x5B677E + 0x1,   (uintptr_t)MobileEffectsRender_stub);

        if(pGrainEffect->GetBool())
        {
            HOOKPLT(InitialisePostEffects,      pGTASA + 0x672200);
            if(sautils != NULL) sautils->AddSliderItem(SetType_Display, "Grain Intensity", pGrainIntensityPercent->GetInt(), 0, 100, GrainIntensityChanged, NULL, NULL);
        }
    }

    if(pWaterFog->GetBool() || pMovingFog->GetBool() || pVolumetricClouds->GetBool())
    {
        logger->Info("Effects patch is enabled!");
        pGTASA_EffectsRender =              pGTASA + 0x3F63A6 + 0x1;
        aml->Redirect(pGTASA + 0x3F638C + 0x1,   (uintptr_t)EffectsRender_stub);

        if(pWaterFog->GetBool())
        {
            aml->Redirect(pGTASA + 0x599FB4 + 0x1, (uintptr_t)SetUpWaterFog_stub);
            void** ms_WaterFog_New = new void*[pWaterFogBlocksLimits->GetInt()];
            aml->Write(pGTASA + 0x6779A0,   (uintptr_t)&ms_WaterFog_New, sizeof(void*));
        }
        if(pMovingFog->GetBool())
        {
        }
        if(pVolumetricClouds->GetBool())
        {
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
        HOOK(PlantSurfPropMgrInit,              aml->GetSym(hGTASA, "_ZN17CPlantSurfPropMgr10InitialiseEv"));

        if(pPlantsDatLoading->GetBool())
        {
            HOOKPLT(PlantMgrInit,               pGTASA + 0x673C90);
            HOOKPLT(PlantMgrRender,             pGTASA + 0x6726D0);
            //HOOKPLT(PlantLocTriAdd,             pGTASA + 0x675504); // I spent my life on useless reversed fn :(
            
            aml->Redirect(pGTASA + 0x2CD42A + 0x1, (uintptr_t)GrassMaterialApplying_Patch);
            GrassMaterialApplying_BackTo = pGTASA + 0x2CD434 + 0x1;
            fTweakGrassAlpha = cfg->GetFloat("GrassAlphaMultiplier", 1.0f, "Features");
        }
    }

    //if(cfg->Bind("BumpShadowsLimit", false, "Shadows")->GetBool())
    //{
    //    RTShadows();
    //}
}
