#include <externs.h>

/* Variables */
RwRGBAReal buildingAmbient;

/* Configs */
ConfigEntry* pCFGBuildingPipeline;
ConfigEntry* pCFGLightningIlluminatesWorld;
ConfigEntry* pCFGExplicitBuildingPipe;

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
        float lightsMult = *p_CCoronas__LightsMult;
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
        instanceData->m_StoredIndexedMeshes = (ArrayState*)geometry->vertexBuffer;
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
        instanceData->m_StoredIndexedMeshes = (ArrayState*)geometry->vertexBuffer;
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
            CustomEnvMapPipeMaterialData* envData = *(CustomEnvMapPipeMaterialData**)((int)material + *ms_envMapPluginOffset);
            RwTexture* envTexture = envData->pEnvTexture;
            envTexture->filterMode2 = 17; // What is this? Special ENV texture mode?
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
            DrawStoredMeshData(meshData); // Dual Pass here?
            if (hasAlphaVertexEnabled) DisableAlphaModulate();
            if (hasEnvMap) ResetEnvMap();
        }
        ++meshData;
    }
    SetSecondVertexColor(0, 0.0f);
}
void CustomPipeRenderCB_PS2(RwResEntry *entry, void *obj, RwUInt8 type, RwUInt32 flags)
{

}
void CustomPipeRenderCB_Xbox(RwResEntry *entry, void *obj, RwUInt8 type, RwUInt32 flags)
{

}
void CustomPipeRenderCB_Switch(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
    switch(pCFGBuildingPipeline->GetInt())
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

    RxOpenGLAllInOneSetInstanceCallBack(NodeByName, (void*)CustomPipeInstanceCB_Mobile);
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

    RxOpenGLAllInOneSetInstanceCallBack(NodeByName, (void*)CustomPipeInstanceCB_Mobile);
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

    HOOKPLT(CustomBuildingPipelineUpdate, pGTASA + 0x67526C);
    if(pCFGExplicitBuildingPipe->GetInt() >= 0)
    {
        HOOKPLT(IsCBPCPipelineAttached, pGTASA + 0x6742F8);
    }
    HOOKPLT(CreateCustomObjPipe, pGTASA + 0x674AC0);
    HOOKPLT(CreateCustomObjPipeDN, pGTASA + 0x6727E4);
}