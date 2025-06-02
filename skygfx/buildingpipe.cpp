#include <externs.h>

/* Variables */
RwRGBAReal buildingAmbient;
int g_nBuildingPipeline = BUILDING_MOBILE;
bool g_bDualPass = false;
const char* aPipelineNames[NUMBUILDINGPIPES] = 
{
    "Default (Mobile)",
    "PS2",
    "Xbox",
};
const char* aDualPassSwitch[2] = 
{
    "FEM_OFF",
    "FEM_ON",
};

/* Configs */
ConfigEntry* pCFGBuildingPipeline;
ConfigEntry* pCFGLightningIlluminatesWorld;
ConfigEntry* pCFGExplicitBuildingPipe;
ConfigEntry* pCFGDualRenderPass;

/* Shaders */
ES2Shader* g_aCustomBuildingShaderPS2[128] = {NULL};
int g_nCustomBuildingShadersPS2 = 0;

void BuildPixelSource_PS2Pipeline(char** buf, int flags); static char* pxl_buf;
void BuildVertexSource_PS2Pipeline(char** buf, int flags); static char* vtx_buf;
inline ES2Shader* GetFlaggedBuildingShader_PS2(int flags)
{
    ES2Shader* shad;
    int instantiateAt = -1;
    for(int i = 0; i < g_nCustomBuildingShadersPS2; ++i)
    {
        shad = g_aCustomBuildingShaderPS2[i];
        if(!shad) // HOW
        {
            logger->Info("HOW.");
            instantiateAt = i;
            break;
        }
        if(shad->flags == flags) return shad;
    }
    
    pxl_buf = new char[4096]; pxl_buf[0] = 0; BuildPixelSource_PS2Pipeline(&pxl_buf, flags);
    vtx_buf = new char[4096]; vtx_buf[0] = 0; BuildVertexSource_PS2Pipeline(&vtx_buf, flags);

    logger->Info("Shader new PRE: num %d", g_nCustomBuildingShadersPS2);
    logger->Info("Shader pxl:\n%s", pxl_buf);
    logger->Info("Shader vtx:\n%s", vtx_buf);
    shad = RQCreateShader(pxl_buf, vtx_buf, flags);
    logger->Info("Shader new: 0x%08X num %d", shad, g_nCustomBuildingShadersPS2);

    if(instantiateAt == -1)
    {
        g_aCustomBuildingShaderPS2[g_nCustomBuildingShadersPS2] = shad;
        ++g_nCustomBuildingShadersPS2;
    } else g_aCustomBuildingShaderPS2[instantiateAt] = shad;

    return shad;
}

/* Hooks */
DECL_HOOKv(CustomBuildingPipelineUpdate)
{
    CustomBuildingPipelineUpdate();

    if(pCFGLightningIlluminatesWorld->GetBool() && *LightningFlash && !IsVisionFXActive())
    {
        buildingAmbient = {1.0f, 1.0f, 1.0f, 0.0f};
    }
    else
    {
        float lightsMult      = *p_CCoronas__LightsMult;
        buildingAmbient.red   = lightsMult * TimeCycGetAmbientRed();
        buildingAmbient.green = lightsMult * TimeCycGetAmbientGreen();
        buildingAmbient.blue  = lightsMult * TimeCycGetAmbientBlue();
    }
}

DECL_HOOK(bool, IsCBPCPipelineAttached, RpAtomic *atomic)
{
    uint32_t pipeID = GetPipelineID(atomic);
    if(pipeID == RSPIPE_PC_CustomBuilding_PipeID || pipeID == RSPIPE_PC_CustomBuildingDN_PipeID) return true;
    if(pCFGExplicitBuildingPipe->GetInt() > 0) return false;

    RpGeometry *geo = atomic ? atomic->geometry : NULL;
    RxPipeline *pipe = atomic ? atomic->pipeline : NULL;
    return (pipe == NULL && GetExtraVertColourPtr(geo) && geo->preLitLum);
}

/* Functions */
inline bool RenderMeshes(RxOpenGLMeshInstanceData* meshData)
{
    if(g_bDualPass)
    {
        RwBool hasAlpha = rwIsAlphaBlendOn();
        int alphafunc, alpharef;
        int zwrite;

        _rwOpenGLGetRenderState(rwRENDERSTATEZWRITEENABLE, &zwrite);
        _rwOpenGLGetRenderState(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);

        if(hasAlpha && zwrite)
        {
            _rwOpenGLGetRenderState(rwRENDERSTATEALPHATESTFUNCTIONREF, &alpharef);
            RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)128);
            RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATEREQUAL);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
            DrawStoredMeshData(meshData);
            RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONLESS);
		    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)false);
            DrawStoredMeshData(meshData);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)(intptr_t)zwrite);
            RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)(intptr_t)alpharef);
            RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)(intptr_t)alphafunc);
        }
        else if(!zwrite)
        {
            RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
            DrawStoredMeshData(meshData);
            RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)(intptr_t)alphafunc);
        }
        else DrawStoredMeshData(meshData);
    }
    else
    {
        DrawStoredMeshData(meshData);
    }
    return g_bDualPass;
}
void PipelineSettingChanged(int oldVal, int newVal, void* data)
{
    pCFGBuildingPipeline->SetInt(newVal);
    pCFGBuildingPipeline->Clamp(0, NUMBUILDINGPIPES - 1);
    g_nBuildingPipeline = pCFGBuildingPipeline->GetInt();
    cfg->Save();
}
void DualPassSettingChanged(int oldVal, int newVal, void* data)
{
    pCFGDualRenderPass->SetBool(newVal != 0);
    g_bDualPass = pCFGDualRenderPass->GetBool();
    cfg->Save();
}

/* Building Pipelines */
RwBool CustomPipeInstanceCB_PS2(void *object, RxOpenGLMeshInstanceData *instanceData, RwBool instanceDLandVA, RwBool reinstance)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    int flags = geometry->flags;
    int numTexCoordSets = geometry->numTexCoordSets;

    instanceData->m_nVertexDesc = flags;
    if(reinstance && (geometry->lockedSinceLastInst & 0xFFF) == 0) return true; // Do not draw - Atomic is locked

    if ( (geometry->flags & rpGEOMETRYNATIVE) != 0 )
    {
        instanceData->m_StoredIndexedMeshes = geometry->vertexBuffer;
        return true;
    }

    unsigned int vertexDataSize;
    RwRGBA* vertexColor1 = (RwRGBA*)((char*)&geometry->object.parent + *ms_extraVertColourPluginOffset);
    RwRGBA* vertexColor2 = (RwRGBA*)((char*)&geometry->object.parent + *ms_extraVertColourPluginOffset - 4); // Cant understand
    if(reinstance)
    {
        ArrayState* emuArrayRef = instanceData->m_StoredMeshes;
        if(emuArrayRef) emu_ArraysDelete(emuArrayRef);

        vertexDataSize = instanceData->m_nVertexDataSize;
    }
    else
    {
        int extraVertexStride = 12;
        if(flags & rpGEOMETRYNORMALS) extraVertexStride = 24;
        if(flags & rpGEOMETRYPRELIT) extraVertexStride += 8;

        int shiftSize = (*RwHackNoCompressedTexCoords) ? 3 : 2;
        instanceData->m_nVertexStride = (numTexCoordSets << shiftSize) + extraVertexStride;
        vertexDataSize = instanceData->m_nVertexStride * instanceData->m_nVertexCount;
        instanceData->m_nVertexDataSize = vertexDataSize;
    }

    RwUInt8* newVertexData = new RwUInt8[vertexDataSize];
    _rxOpenGLAllInOneAtomicInstanceVertexArray(instanceData, atomic, geometry, (RpGeometryFlag)flags, numTexCoordSets, reinstance, newVertexData, vertexColor1, vertexColor2);

    emu_ArraysReset();
    uint16_t* indexData = instanceData->m_pIndices;
    if(indexData) emu_ArraysIndices(indexData, 0x1403, instanceData->m_nIndicesCount);

    emu_ArraysVertex(indexData, instanceData->m_nVertexDataSize, instanceData->m_nVertexCount, instanceData->m_nVertexStride);
    emu_ArraysVertexAttrib(0, 3, 0x1406, 0, 0);

    int offset = 12;
    if((flags & rpGEOMETRYNORMALS) != 0)
    {
        emu_ArraysVertexAttrib(2, 3, 0x1406, 0, 12);
        offset = 24;
    }
    if((flags & rpGEOMETRYPRELIT) != 0)
    {
        emu_ArraysVertexAttrib(3, 4, 0x1401, 1, offset);
        emu_ArraysVertexAttrib(6, 4, 0x1401, 1, offset + 4);
        offset += 8;
    }

    if (numTexCoordSets > 0) emu_ArraysVertexAttrib(1, 2, *RwHackNoCompressedTexCoords ? 0x1406 : 0x1403, 0, offset);
    instanceData->m_StoredMeshes = emu_ArraysStore(reinstance != 0, 1u);
    return true;
}
RwBool CustomPipeInstanceCB_Mobile(void *object, RxOpenGLMeshInstanceData *instanceData, RwBool instanceDLandVA, RwBool reinstance)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    int flags = geometry->flags;
    int numTexCoordSets = geometry->numTexCoordSets;

    instanceData->m_nVertexDesc = flags;
    if(reinstance && (geometry->lockedSinceLastInst & 0xFFF) == 0) return true; // Do not draw - Atomic is locked

    if ( (geometry->flags & rpGEOMETRYNATIVE) != 0 )
    {
        instanceData->m_StoredIndexedMeshes = geometry->vertexBuffer;
        return true;
    }

    unsigned int vertexDataSize;
    RwRGBA* vertexColor1 = (RwRGBA*)((char*)&geometry->object.parent + *ms_extraVertColourPluginOffset);
    RwRGBA* vertexColor2 = (RwRGBA*)((char*)&geometry->object.parent + *ms_extraVertColourPluginOffset - 4); // Cant understand
    if(reinstance)
    {
        ArrayState* emuArrayRef = instanceData->m_StoredMeshes;
        if(emuArrayRef) emu_ArraysDelete(emuArrayRef);

        vertexDataSize = instanceData->m_nVertexDataSize;
    }
    else
    {
        int extraVertexStride = 12;
        if(flags & rpGEOMETRYNORMALS) extraVertexStride = 24;
        if(flags & rpGEOMETRYPRELIT) extraVertexStride += 8;

        int shiftSize = (*RwHackNoCompressedTexCoords) ? 3 : 2;
        instanceData->m_nVertexStride = (numTexCoordSets << shiftSize) + extraVertexStride;
        vertexDataSize = instanceData->m_nVertexStride * instanceData->m_nVertexCount;
        instanceData->m_nVertexDataSize = vertexDataSize;
    }

    RwUInt8* newVertexData = new RwUInt8[vertexDataSize];
    _rxOpenGLAllInOneAtomicInstanceVertexArray(instanceData, atomic, geometry, (RpGeometryFlag)flags, numTexCoordSets, reinstance, newVertexData, vertexColor1, vertexColor2);

    emu_ArraysReset();
    uint16_t* indexData = instanceData->m_pIndices;
    if(indexData) emu_ArraysIndices(indexData, 0x1403, instanceData->m_nIndicesCount);

    emu_ArraysVertex(indexData, instanceData->m_nVertexDataSize, instanceData->m_nVertexCount, instanceData->m_nVertexStride);
    emu_ArraysVertexAttrib(0, 3, 0x1406, 0, 0);

    int offset = 12;
    if((flags & rpGEOMETRYNORMALS) != 0)
    {
        emu_ArraysVertexAttrib(2, 3, 0x1406, 0, 12);
        offset = 24;
    }
    if((flags & rpGEOMETRYPRELIT) != 0)
    {
        emu_ArraysVertexAttrib(3, 4, 0x1401, 1, offset);
        emu_ArraysVertexAttrib(6, 4, 0x1401, 1, offset + 4);
        offset += 8;
    }

    if (numTexCoordSets > 0) emu_ArraysVertexAttrib(1, 2, *RwHackNoCompressedTexCoords ? 0x1406 : 0x1403, 0, offset);
    instanceData->m_StoredMeshes = emu_ArraysStore(reinstance != 0, 1u);
    return true;
}
RwBool CustomPipeInstanceCB_Switch(void *object, RxOpenGLMeshInstanceData *instanceData, RwBool instanceDLandVA, RwBool reinstance)
{
    switch(g_nBuildingPipeline)
    {
        case BUILDING_PS2:
        case BUILDING_XBOX:
            return CustomPipeInstanceCB_PS2(object, instanceData, instanceDLandVA, reinstance);

        default: return CustomPipeInstanceCB_Mobile(object, instanceData, instanceDLandVA, reinstance);
    }
}

void CustomPipeRenderCB_Mobile(RwResEntry *entry, void *obj, RwUInt8 type, RwUInt32 flags)
{
    RxOpenGLResEntryHeader* header = ResEntryHeader(entry);
    RxOpenGLMeshInstanceData* meshData = ResEntryInstanceData(entry);
    int numMeshes = header->numMeshes;

    SetSecondVertexColor(1, *m_fDNBalanceParam);
    while(--numMeshes >= 0)
    {
        RpMaterial* material = meshData->m_pMaterial;
        RwTexture* texture = material->texture;

        uint8_t alpha = material->color.alpha;
        bool hasAlphaVertexEnabled = meshData->m_nVertexAlpha || alpha != 255;

        if(hasAlphaVertexEnabled && alpha == 0) // Fully invisible?
        {
            ++meshData;
            continue;
        }

        if(hasAlphaVertexEnabled) EnableAlphaModulate(alpha / 255.0f);
        _rwOpenGLSetRenderState(rwRENDERSTATEVERTEXALPHAENABLE, hasAlphaVertexEnabled);
        if(*rwOpenGLLightingEnabled)
        {
            _rwOpenGLLightsSetMaterialPropertiesORG(material, flags);
        }
        else
        {
            if(*rwOpenGLColorMaterialEnabled)
            {
                emu_glDisable(0x0B57);
                *rwOpenGLColorMaterialEnabled = 0;
            }
            if((flags & rxGEOMETRY_PRELIT) == 0)
            {
                emu_glColor4fv(rwOpenGLOpaqueBlack);
            }
        }

        bool hasEnvMap = (*(int*)&material->surfaceProps.specular & 0x1) != 0; // This part is VERY WEIRD. Optimizer?
        if(hasEnvMap)
        {
            CustomEnvMapPipeMaterialData* envData = *(CustomEnvMapPipeMaterialData**)((uintptr_t)material + *ms_envMapPluginOffset);
            RwTexture* envTexture = envData->pEnvTexture;
            envTexture->filterMode = 17; // What is this? Special ENV texture mode?
            SetEnvMap(*(void **)((char*)&envTexture->raster->parent + *RasterExtOffset), envData->GetShininess() * 1.5f, 0);
            SetNormalMatrix(envData->GetScaleX(), envData->GetScaleY(), RwV2d(0, 0));
            *doPop = 0;
        }

        if(!(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED)) || texture == NULL)
        {
            _rwOpenGLSetRenderState(rwRENDERSTATETEXTURERASTER, false);
            DrawStoredMeshData(meshData);
            if (hasAlphaVertexEnabled) DisableAlphaModulate();
            if (hasEnvMap) ResetEnvMap();
        }
        else if((texture->raster->privateFlags & 0x1) != 0)
        {
            if (hasAlphaVertexEnabled) DisableAlphaModulate();
        }
        else
        {
            _rwOpenGLSetRenderStateNoExtras(rwRENDERSTATETEXTURERASTER, (void*)texture->raster);
            _rwOpenGLSetRenderState(rwRENDERSTATETEXTUREFILTER, texture->filterMode);
            DrawStoredMeshData(meshData); // Dual Pass here? Or shaders?!
            if (hasAlphaVertexEnabled) DisableAlphaModulate();
            if (hasEnvMap) ResetEnvMap();
        }
        ++meshData;
    }
    SetSecondVertexColor(0, 0.0f);
}
void CustomPipeRenderCB_PS2(RwResEntry *entry, void *obj, RwUInt8 type, RwUInt32 flags)
{
    ForceCustomShader(GetFlaggedBuildingShader_PS2(flags));
    CustomPipeRenderCB_Mobile(entry, obj, type, flags);
    ForceCustomShader(NULL); // Allow other shaders to be used
}
void CustomPipeRenderCB_Xbox(RwResEntry *entry, void *obj, RwUInt8 type, RwUInt32 flags)
{

}
void CustomPipeRenderCB_Switch(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
    switch(g_nBuildingPipeline)
    {
        case BUILDING_PS2:
        {
            CustomPipeRenderCB_PS2(repEntry, object, type, flags);
            break;
        }
        case BUILDING_XBOX:
        {
            CustomPipeRenderCB_Xbox(repEntry, object, type, flags);
            break;
        }
        default: // BUILDING_MOBILE
        {
            CustomPipeRenderCB_Mobile(repEntry, object, type, flags);
            break;
        }
    }
    // We dont have SAMP that breaks the rendering
    // so "fixSAMP" part is not going to be anywhere
}
DECL_HOOK(RxPipeline*, CreateCustomObjPipe)
{
    RxPipeline* pipeline = RxPipelineCreate();
    if(pipeline == NULL) return NULL;

    RxNodeDefinition* OpenGLAtomicAllInOne = RxNodeDefinitionGetOpenGLAtomicAllInOne();
    RxLockedPipe* lockedPipeline = RxPipelineLock(pipeline);
    if(lockedPipeline == NULL || RxLockedPipeAddFragment(lockedPipeline, 0, OpenGLAtomicAllInOne, 0) == NULL || RxLockedPipeUnlock(lockedPipeline) == NULL)
    {
        _rxPipelineDestroy(pipeline);
        return NULL;
    }
    RxPipelineNode* NodeByName = RxPipelineFindNodeByName(pipeline, OpenGLAtomicAllInOne->name, 0, 0);

    RxOpenGLAllInOneSetInstanceCallBack(NodeByName, (void*)CustomPipeInstanceCB_Switch);
    RxOpenGLAllInOneSetRenderCallBack(NodeByName, (void*)CustomPipeRenderCB_Switch);

    pipeline->pluginId = RSPIPE_PC_CustomBuilding_PipeID;
    pipeline->pluginData = RSPIPE_PC_CustomBuilding_PipeID;
    return pipeline;
}
DECL_HOOK(RxPipeline*, CreateCustomObjPipeDN)
{
    RxPipeline* pipeline = RxPipelineCreate();
    if(pipeline == NULL) return NULL;

    RxNodeDefinition* OpenGLAtomicAllInOne = RxNodeDefinitionGetOpenGLAtomicAllInOne();
    RxLockedPipe* lockedPipeline = RxPipelineLock(pipeline);
    if(lockedPipeline == NULL || RxLockedPipeAddFragment(lockedPipeline, 0, OpenGLAtomicAllInOne, 0) == NULL || RxLockedPipeUnlock(lockedPipeline) == NULL)
    {
        _rxPipelineDestroy(pipeline);
        return NULL;
    }
    RxPipelineNode* NodeByName = RxPipelineFindNodeByName(pipeline, OpenGLAtomicAllInOne->name, 0, 0);

    RxOpenGLAllInOneSetInstanceCallBack(NodeByName, (void*)CustomPipeInstanceCB_Switch);
    RxOpenGLAllInOneSetRenderCallBack(NodeByName, (void*)CustomPipeRenderCB_Switch);

    pipeline->pluginId = RSPIPE_PC_CustomBuildingDN_PipeID;
    pipeline->pluginData = RSPIPE_PC_CustomBuildingDN_PipeID;

    return pipeline;
}

/* Main */
void StartBuildingPipeline()
{
    pCFGBuildingPipeline = cfg->Bind("BuildingPipeline", BUILDING_MOBILE, "Pipeline");
    pCFGBuildingPipeline->Clamp(BUILDING_MOBILE, NUMBUILDINGPIPES - 1);
    pCFGLightningIlluminatesWorld = cfg->Bind("LightningIlluminatesWorld", false, "Pipeline");
    pCFGExplicitBuildingPipe = cfg->Bind("ExplicitBuildingPipe", -1, "Pipeline");
    pCFGDualRenderPass = cfg->Bind("DualRenderPass", false, "Pipeline");

    HOOKPLT(CustomBuildingPipelineUpdate, pGTASA + 0x67526C);
    if(pCFGExplicitBuildingPipe->GetInt() >= 0)
    {
        HOOKPLT(IsCBPCPipelineAttached, pGTASA + 0x6742F8);
    }
    HOOKPLT(CreateCustomObjPipe, pGTASA + 0x674AC0);
    HOOKPLT(CreateCustomObjPipeDN, pGTASA + 0x6727E4);

    g_bDualPass = pCFGDualRenderPass->GetBool();
    g_nBuildingPipeline = pCFGBuildingPipeline->GetInt();
    AddSetting("Building Pipeline", g_nBuildingPipeline, 0, sizeofA(aPipelineNames)-1, aPipelineNames, PipelineSettingChanged, NULL);
    //AddSetting("Dual Pass Render", g_bDualPass, 0, sizeofA(aDualPassSwitch)-1, aDualPassSwitch, DualPassSettingChanged, NULL);
}

/* Shaders */
void BuildPixelSource_PS2Pipeline(char** buf, int flags)
{
    char tmp[512];
    char* t = *buf;
    int ped_spec = (FLAG_BONE3 | FLAG_BONE4);
    
    PXL_EMIT("#version 100");
    PXL_EMIT("precision mediump float;");
    if(flags & FLAG_TEX0)
        PXL_EMIT("uniform sampler2D Diffuse;\nvarying mediump vec2 Out_Tex0;");
    if(flags & (FLAG_SPHERE_ENVMAP | FLAG_ENVMAP))
    {
        PXL_EMIT("uniform sampler2D EnvMap;\nuniform lowp float EnvMapCoefficient;");
        if(flags & FLAG_ENVMAP)
            PXL_EMIT("varying mediump vec2 Out_Tex1;");
        else
            PXL_EMIT("varying mediump vec3 Out_Refl;");
    }
    else if(flags & FLAG_DETAILMAP)
    {
        PXL_EMIT("uniform sampler2D EnvMap;\nuniform float DetailTiling;");
    }
    
    if(flags & FLAG_FOG)
        PXL_EMIT("varying mediump float Out_FogAmt;\nuniform lowp vec3 FogColor;");
        
    if(flags & (FLAG_LIGHTING | FLAG_COLOR))
        PXL_EMIT("varying lowp vec4 Out_Color;");
        
    if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
        PXL_EMIT("varying lowp vec3 Out_Spec;");

    if(flags & FLAG_ALPHA_MODULATE)
        PXL_EMIT("uniform lowp float AlphaModulate;");

    if(flags & FLAG_WATER)
        PXL_EMIT("varying mediump vec2 Out_WaterDetail;\nvarying mediump vec2 Out_WaterDetail2;\nvarying mediump float Out_WaterAlphaBlend;");

    ///////////////////////////////////////
    //////////////// START ////////////////
    ///////////////////////////////////////
    PXL_EMIT("void main() {");

    PXL_EMIT("\tlowp vec4 fcolor;");
    if(flags & FLAG_TEX0)
    {
        if(flags & FLAG_TEXBIAS)
        {
            PXL_EMIT("\tlowp vec4 diffuseColor = texture2D(Diffuse, Out_Tex0, -1.5);");
        }
        else if(RQCaps->isSlowGPU != 0)
        {
            PXL_EMIT("\tlowp vec4 diffuseColor = texture2D(Diffuse, Out_Tex0);");
        }
        else
        {
            PXL_EMIT("\tlowp vec4 diffuseColor = texture2D(Diffuse, Out_Tex0, -0.5);");
        }
        PXL_EMIT("\tfcolor = diffuseColor;");

        if(flags & (FLAG_COLOR | FLAG_LIGHTING))
        {
            if(flags & FLAG_DETAILMAP)
            {
                if(!(flags & FLAG_WATER))
                {
                    PXL_EMIT("\tfcolor *= vec4(Out_Color.xyz * texture2D(EnvMap, Out_Tex0.xy * DetailTiling, -0.5).xyz * 2.0, Out_Color.w);");
                    if(flags & FLAG_ENVMAP)
                    {
                        PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
                    }
                    goto LABEL_45; // CHECK
                }
                PXL_EMIT("\tfloat waterDetail = texture2D(EnvMap, Out_WaterDetail, -1.0).x + texture2D(EnvMap, Out_WaterDetail2, -1.0).x;");
                PXL_EMIT("\tfcolor *= vec4(Out_Color.xyz * waterDetail * 1.1, Out_Color.w);");
                PXL_EMIT("\tfcolor.a += Out_WaterAlphaBlend;");
                if(flags & FLAG_ENVMAP)
                {
                    PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
                }
                goto LABEL_45; // CHECK
            }
            PXL_EMIT("\tfcolor *= Out_Color;");
        }
        if(!(flags & FLAG_WATER))
        {
            if(flags & FLAG_ENVMAP)
            {
                PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
            }
            goto LABEL_45; // CHECK
        }
        PXL_EMIT("\tfcolor.a += Out_WaterAlphaBlend;");
        if(flags & FLAG_ENVMAP)
        {
            PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
        }
        goto LABEL_45; // CHECK
    }

    if(flags & (FLAG_COLOR | FLAG_LIGHTING))
    {
        PXL_EMIT("\tfcolor = Out_Color;");
    }
    else
    {
        PXL_EMIT("\tfcolor = 0.0;");
    }
    if(flags & FLAG_ENVMAP)
    {
        PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
    }

LABEL_45:
    if(flags & FLAG_SPHERE_ENVMAP)
    {
        PXL_EMIT("\tvec2 ReflPos = normalize(Out_Refl.xy) * (Out_Refl.z * 0.5 + 0.5);");
        PXL_EMIT("\tReflPos = (ReflPos * vec2(0.5,0.5)) + vec2(0.5,0.5);");
        PXL_EMIT("\tlowp vec4 ReflTexture = texture2D(EnvMap, ReflPos);");
        PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz,ReflTexture.xyz, EnvMapCoefficient);");
        PXL_EMIT("\tfcolor.w += ReflTexture.b * 0.125;");
    }

    if(!RQCaps->unk_08)
    {
        if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
        {
            PXL_EMIT("\tfcolor.xyz += Out_Spec;");
        }
        if(flags & FLAG_FOG)
            PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, FogColor, Out_FogAmt);");
    }

    if(flags & FLAG_GAMMA)
    {
        PXL_EMIT("\tfcolor.xyz += fcolor.xyz * 0.5;");
    }

    PXL_EMIT("\tgl_FragColor = fcolor;");
    if(flags & FLAG_ALPHA_TEST)
    {
        PXL_EMIT("\t/*ATBEGIN*/");
        if ((OS_SystemChip() == 13) && (flags & FLAG_TEX0))
        {
            if (flags & FLAG_TEXBIAS)
            {
                PXL_EMIT("\tif (diffuseColor.a < 0.8) { discard; }");
            }
            else
            {
                if (!(flags & FLAG_CAMERA_BASED_NORMALS))
                {
                    PXL_EMIT("\tif (diffuseColor.a < 0.2) { discard; }");
                }
                else
                {
                    PXL_EMIT("\tgl_FragColor.a = Out_Color.a;");
                    PXL_EMIT("\tif (diffuseColor.a < 0.5) { discard; }");
                }
            }
        }
        else
        {
            if (flags & FLAG_TEXBIAS)
            {
                PXL_EMIT("\tif (diffuseColor.a < 0.8) { discard; }");
            }
            else
            {
                if (!(flags & FLAG_CAMERA_BASED_NORMALS))
                {
                    PXL_EMIT("\tif (diffuseColor.a < 0.2) { discard; }");
                }
                else
                {
                    PXL_EMIT("\tif (diffuseColor.a < 0.5) { discard; }");
                    PXL_EMIT("\tgl_FragColor.a = Out_Color.a;");
                }
            }
        }
        PXL_EMIT("\t/*ATEND*/");
    }

    if(flags & FLAG_ALPHA_MODULATE)
        PXL_EMIT("\tgl_FragColor.a *= AlphaModulate;");

    PXL_EMIT("}");
    ///////////////////////////////////////
    ////////////////  END  ////////////////
    ///////////////////////////////////////
}
void BuildVertexSource_PS2Pipeline(char** buf, int flags)
{
    char tmp[512];
    char* t = *buf;
    int ped_spec = (FLAG_BONE3 | FLAG_BONE4);
    
    VTX_EMIT("#version 100");
    VTX_EMIT("precision highp float;\nuniform mat4 ProjMatrix;\nuniform mat4 ViewMatrix;\nuniform mat4 ObjMatrix;");
    if(flags & FLAG_LIGHTING)
    {
        VTX_EMIT("uniform lowp vec3 AmbientLightColor;\nuniform lowp vec4 MaterialEmissive;");
        VTX_EMIT("uniform lowp vec4 MaterialAmbient;\nuniform lowp vec4 MaterialDiffuse;");
        if(flags & FLAG_LIGHT1)
        {
            VTX_EMIT("uniform lowp vec3 DirLightDiffuseColor;\nuniform vec3 DirLightDirection;");
            if (GetMobileEffectSetting() == 3 && (flags & (FLAG_BACKLIGHT | FLAG_BONE3 | FLAG_BONE4)))
            {
                VTX_EMIT("uniform vec3 DirBackLightDirection;");
            }
        }
        if(flags & FLAG_LIGHT2)
        {
            VTX_EMIT("uniform lowp vec3 DirLight2DiffuseColor;\nuniform vec3 DirLight2Direction;");
        }
        if(flags & FLAG_LIGHT3)
        {
            VTX_EMIT("uniform lowp vec3 DirLight3DiffuseColor;\nuniform vec3 DirLight3Direction;");
        }
    }
    
    VTX_EMIT("attribute vec3 Position;\nattribute vec3 Normal;");
    if(flags & FLAG_TEX0)
    {
        if(flags & FLAG_PROJECT_TEXCOORD)
            VTX_EMIT("attribute vec4 TexCoord0;");
        else
            VTX_EMIT("attribute vec2 TexCoord0;");
    }

    VTX_EMIT("attribute vec4 GlobalColor;");
    
    if(flags & (FLAG_BONE3 | FLAG_BONE4))
    {
        VTX_EMIT("attribute vec4 BoneWeight;\nattribute vec4 BoneIndices;");
        VTX_EMIT_V("uniform highp vec4 Bones[%d];", 3 * *RQMaxBones);
    }
    
    if(flags & FLAG_TEX0)
    {
        VTX_EMIT("varying mediump vec2 Out_Tex0;");
    }
    
    if(flags & FLAG_TEXMATRIX)
    {
        VTX_EMIT("uniform mat3 NormalMatrix;");
    }

    if(flags & (FLAG_SPHERE_ENVMAP | FLAG_ENVMAP))
    {
        if(flags & FLAG_ENVMAP)
            VTX_EMIT("varying mediump vec2 Out_Tex1;");
        else
            VTX_EMIT("varying mediump vec3 Out_Refl;");
        VTX_EMIT("uniform lowp float EnvMapCoefficient;");
    }

    if(flags & (FLAG_SPHERE_ENVMAP | FLAG_SPHERE_XFORM | FLAG_WATER | FLAG_FOG | FLAG_CAMERA_BASED_NORMALS | FLAG_ENVMAP | ped_spec))
    {
        VTX_EMIT("uniform vec3 CameraPosition;");
    }

    if(flags & FLAG_FOG)
    {
        VTX_EMIT("varying mediump float Out_FogAmt;");
        VTX_EMIT("uniform vec3 FogDistances;");
    }

    if(flags & FLAG_WATER)
    {
        VTX_EMIT("uniform vec3 WaterSpecs;");
        VTX_EMIT("varying mediump vec2 Out_WaterDetail;");
        VTX_EMIT("varying mediump vec2 Out_WaterDetail2;");
        VTX_EMIT("varying mediump float Out_WaterAlphaBlend;");
    }

    if(flags & FLAG_COLOR2)
    {
        VTX_EMIT("attribute vec4 Color2;");
        VTX_EMIT("uniform lowp float ColorInterp;");
    }

    if(flags & (FLAG_COLOR | FLAG_LIGHTING))
    {
        VTX_EMIT("varying lowp vec4 Out_Color;");
    }

    if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
    {
        VTX_EMIT("varying lowp vec3 Out_Spec;");
    }

    ///////////////////////////////////////
    //////////////// START ////////////////
    ///////////////////////////////////////
    VTX_EMIT("void main() {");

    if(flags & (FLAG_BONE4 | FLAG_BONE3))
    {
        VTX_EMIT("\tivec4 BlendIndexArray = ivec4(BoneIndices);");
        VTX_EMIT("\tmat4 BoneToLocal;");
        VTX_EMIT("\tBoneToLocal[0] = Bones[BlendIndexArray.x*3] * BoneWeight.x;");
        VTX_EMIT("\tBoneToLocal[1] = Bones[BlendIndexArray.x*3+1] * BoneWeight.x;");
        VTX_EMIT("\tBoneToLocal[2] = Bones[BlendIndexArray.x*3+2] * BoneWeight.x;");
        VTX_EMIT("\tBoneToLocal[3] = vec4(0.0,0.0,0.0,1.0);");
        VTX_EMIT("\tBoneToLocal[0] += Bones[BlendIndexArray.y*3] * BoneWeight.y;");
        VTX_EMIT("\tBoneToLocal[1] += Bones[BlendIndexArray.y*3+1] * BoneWeight.y;");
        VTX_EMIT("\tBoneToLocal[2] += Bones[BlendIndexArray.y*3+2] * BoneWeight.y;");
        VTX_EMIT("\tBoneToLocal[0] += Bones[BlendIndexArray.z*3] * BoneWeight.z;");
        VTX_EMIT("\tBoneToLocal[1] += Bones[BlendIndexArray.z*3+1] * BoneWeight.z;");
        VTX_EMIT("\tBoneToLocal[2] += Bones[BlendIndexArray.z*3+2] * BoneWeight.z;");

        if(flags & FLAG_BONE4)
        {
            VTX_EMIT("\tBoneToLocal[0] += Bones[BlendIndexArray.w*3] * BoneWeight.w;");
            VTX_EMIT("\tBoneToLocal[1] += Bones[BlendIndexArray.w*3+1] * BoneWeight.w;");
            VTX_EMIT("\tBoneToLocal[2] += Bones[BlendIndexArray.w*3+2] * BoneWeight.w;");
        }

        VTX_EMIT("\tvec4 WorldPos = ObjMatrix * (vec4(Position,1.0) * BoneToLocal);");
    }
    else
    {
        VTX_EMIT("\tvec4 WorldPos = ObjMatrix * vec4(Position,1.0);");
    }

    if(flags & FLAG_SPHERE_XFORM)
    {
        VTX_EMIT("\tvec3 ReflVector = WorldPos.xyz - CameraPosition.xyz;");
        VTX_EMIT("\tvec3 ReflPos = normalize(ReflVector);");
        VTX_EMIT("\tReflPos.xy = normalize(ReflPos.xy) * (ReflPos.z * 0.5 + 0.5);");
        VTX_EMIT("\tgl_Position = vec4(ReflPos.xy, length(ReflVector) * 0.002, 1.0);");
    }
    else
    {
        VTX_EMIT("\tvec4 ViewPos = ViewMatrix * WorldPos;");
        VTX_EMIT("\tgl_Position = ProjMatrix * ViewPos;");
    }

    if(flags & FLAG_LIGHTING)
    {
        if((flags & (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST)) == (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST) && (flags & (FLAG_LIGHT3 | FLAG_LIGHT2 | FLAG_LIGHT1)))
        {
            VTX_EMIT("\tvec3 WorldNormal = normalize(vec3(WorldPos.xy - CameraPosition.xy, 0.0001)) * 0.85;");
        }
        else if(flags & (FLAG_BONE4 | FLAG_BONE3))
        {
            VTX_EMIT("\tvec3 WorldNormal = mat3(ObjMatrix) * (Normal * mat3(BoneToLocal));");
        }
        else
        {
            VTX_EMIT("\tvec3 WorldNormal = (ObjMatrix * vec4(Normal, 0.0)).xyz;");
        }
    }
    else
    {
        if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
            VTX_EMIT("\tvec3 WorldNormal = vec3(0.0, 0.0, 0.0);");
    }

    if(!RQCaps->unk_08 && (flags & FLAG_FOG))
    {
        VTX_EMIT("\tOut_FogAmt = clamp((length(WorldPos.xyz - CameraPosition.xyz) - FogDistances.x) * FogDistances.z, 0.0, 0.90);");
    }

    const char* _tmp;
    if(flags & FLAG_TEX0)
    {
        if(flags & FLAG_PROJECT_TEXCOORD)
        {
            _tmp = "TexCoord0.xy / TexCoord0.w";
        }
        else if(flags & FLAG_COMPRESSED_TEXCOORD)
        {
            _tmp = "TexCoord0 / 512.0";
        }
        else
        {
            _tmp = "TexCoord0";
        }

        if(flags & FLAG_TEXMATRIX)
        {
            VTX_EMIT_V("\tOut_Tex0 = (NormalMatrix * vec3(%s, 1.0)).xy;", _tmp);
        }
        else
        {
            VTX_EMIT_V("\tOut_Tex0 = %s;", _tmp);
        }
    }

    if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
    {
        VTX_EMIT("\tvec3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
        VTX_EMIT("\treflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
        if(flags & FLAG_SPHERE_ENVMAP)
        {
            VTX_EMIT("\tOut_Refl = reflVector;");
        }
        else
        {
            VTX_EMIT("\tOut_Tex1 = vec2(length(reflVector.xy), (reflVector.z * 0.5) + 0.25);");
        }
    }

    if(flags & FLAG_COLOR2)
    {
        VTX_EMIT("\tlowp vec4 InterpColor = mix(GlobalColor, Color2, ColorInterp);");
        _tmp = "InterpColor";
    }
    else
    {
        _tmp = "GlobalColor";
    }

    if(flags & FLAG_LIGHTING)
    {
        VTX_EMIT("\tvec3 Out_LightingColor;");
        if(flags & FLAG_COLOR_EMISSIVE)
        {
            if(flags & FLAG_CAMERA_BASED_NORMALS)
            {
                VTX_EMIT("\tOut_LightingColor = AmbientLightColor * MaterialAmbient.xyz * 1.5;");
            }
            else
            {
                VTX_EMIT_V("\tOut_LightingColor = AmbientLightColor * MaterialAmbient.xyz + %s.xyz;", _tmp);
            }
        }
        else
        {
            VTX_EMIT("\tOut_LightingColor = AmbientLightColor * MaterialAmbient.xyz + MaterialEmissive.xyz;");
        }

        if(flags & (FLAG_LIGHT3 | FLAG_LIGHT2 | FLAG_LIGHT1))
        {
            if(flags & FLAG_LIGHT1)
            {
                if(GetMobileEffectSetting() == 3 && (flags & (FLAG_BACKLIGHT | FLAG_BONE4 | FLAG_BONE3)))
                {
                    VTX_EMIT("\tOut_LightingColor += (max(dot(DirLightDirection, WorldNormal), 0.0) + max(dot(DirBackLightDirection, WorldNormal), 0.0)) * DirLightDiffuseColor;");
                }
                else
                {
                    VTX_EMIT("\tOut_LightingColor += max(dot(DirLightDirection, WorldNormal), 0.0) * DirLightDiffuseColor;");
                }

                if(!(flags & FLAG_LIGHT2))
                {
                    if(flags & FLAG_LIGHT3)
                        VTX_EMIT("\tOut_LightingColor += max(dot(DirLight3Direction, WorldNormal), 0.0) * DirLight3DiffuseColor;");
                    goto LABEL_89;
                }
            }
            else if(!(flags & FLAG_LIGHT2))
            {
                if(flags & FLAG_LIGHT3)
                    VTX_EMIT("\tOut_LightingColor += max(dot(DirLight3Direction, WorldNormal), 0.0) * DirLight3DiffuseColor;");
                goto LABEL_89;
            }
            VTX_EMIT("\tOut_LightingColor += max(dot(DirLight2Direction, WorldNormal), 0.0) * DirLight2DiffuseColor;");
        }
    }

// Finalize
LABEL_89:
    if(flags & (FLAG_COLOR | FLAG_LIGHTING))
    {
        if(flags & FLAG_LIGHTING)
        {
            if(flags & FLAG_COLOR)
                VTX_EMIT_V("\tOut_Color = vec4((Out_LightingColor.xyz + %s.xyz * 1.5) * MaterialDiffuse.xyz, (MaterialAmbient.w) * %s.w);", _tmp, _tmp);
            else
                VTX_EMIT_V("\tOut_Color = vec4(Out_LightingColor * MaterialDiffuse.xyz, MaterialAmbient.w * %s.w);", _tmp);
            VTX_EMIT("\tOut_Color = clamp(Out_Color, 0.0, 1.0);");
        }
        else
        {
            VTX_EMIT_V("\tOut_Color = %s;", _tmp);
        }
    }

    if(!RQCaps->unk_08 && (flags & FLAG_LIGHT1))
    {
        if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
        {
            VTX_EMIT_V("\tfloat specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * EnvMapCoefficient * 2.0;", RQCaps->isMaliChip ? 9.0f : 10.0f);
        }
        else
        {
            if(!(flags & ped_spec)) goto LABEL_107;
            VTX_EMIT("\tvec3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
            VTX_EMIT("\treflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
            VTX_EMIT_V("\tfloat specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * 0.125;", RQCaps->isMaliChip ? 5.0f : 4.0f);
        }
        VTX_EMIT("\tOut_Spec = specAmt * DirLightDiffuseColor;");
    }

LABEL_107:
    if(flags & FLAG_WATER)
    {
        VTX_EMIT("\tOut_WaterDetail = (Out_Tex0 * 4.0) + vec2(WaterSpecs.x * -0.3, WaterSpecs.x * 0.21);");
        VTX_EMIT("\tOut_WaterDetail2 = (Out_Tex0 * -8.0) + vec2(WaterSpecs.x * 0.12, WaterSpecs.x * -0.05);");
        VTX_EMIT("\tOut_WaterAlphaBlend = distance(WorldPos.xy, CameraPosition.xy) * WaterSpecs.y;");
    }

    VTX_EMIT("}");
    ///////////////////////////////////////
    ////////////////  END  ////////////////
    ///////////////////////////////////////
}