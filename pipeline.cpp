#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <gtasa_things.h>
#include "GTASA_STRUCTS.h"

#include "pipeline.h"

void              (*_rxPipelineDestroy)(RxPipeline*);
RxPipeline*       (*RxPipelineCreate)();
RxLockedPipe*     (*RxPipelineLock)(RxPipeline*);
RxNodeDefinition* (*RxNodeDefinitionGetOpenGLAtomicAllInOne)();
void*             (*RxLockedPipeAddFragment)(RxLockedPipe*, int, RxNodeDefinition*, int);
RxLockedPipe*     (*RxLockedPipeUnlock)(RxLockedPipe*);
RxPipelineNode*   (*RxPipelineFindNodeByName)(RxPipeline*, const char*, int, int);
void              (*RxOpenGLAllInOneSetInstanceCallBack)(RxPipelineNode*, void*);
void              (*RxOpenGLAllInOneSetRenderCallBack)(RxPipelineNode*, void*);
void              (*_rwOpenGLSetRenderState)(RwRenderState, int); //
void              (*_rwOpenGLGetRenderState)(RwRenderState, void*); //
void              (*_rwOpenGLSetRenderStateNoExtras)(RwRenderState, void*); //
void              (*_rwOpenGLLightsSetMaterialPropertiesORG)(const RpMaterial *mat, RwUInt32 flags); //
void              (*SetNormalMatrix)(float, float, RwV2d); //
void              (*DrawStoredMeshData)(RxOpenGLMeshInstanceData*);


float* m_fDNBalanceParam;
float* rwOpenGLOpaqueBlack;
int* rwOpenGLLightingEnabled;
int* rwOpenGLColorMaterialEnabled;
int* ms_envMapPluginOffset;
int* ppline_RasterExtOffset;
char* byte_70BF3C;
// Start emu_*
void (*ppline_SetSecondVertexColor)(uint8_t, float);
void (*ppline_EnableAlphaModulate)(float);
void (*ppline_DisableAlphaModulate)();
void (*ppline_glDisable)(uint32_t);
void (*ppline_glColor4fv)(float*);
void (*ppline_SetEnvMap)(void*, float, int);
void (*ppline_ResetEnvMap)();
void (*ppline_glPopMatrix)();
void (*ppline_glMatrixMode)(uint32_t);
// End emu_*


int CCustomBuildingDNPipeline__CustomPipeInstanceCB_SkyGfx(void* object, RxOpenGLMeshInstanceData* meshData, int, int reinstance)
{
    // Still nothing because PC's Pipe and Mobile's are COMPLETELY different :(
    return 1;
}

inline void RenderMeshes(int meshData, unsigned int flags)
{
    RpMaterial* material = *(RpMaterial**)(meshData + 48);
    int alpha = material->color.alpha;
    bool hasAlphaVertexEnabled = !(*(int*)(meshData + 52)==0 && alpha==255),
         hasEnvMap = false;
    if(hasAlphaVertexEnabled && alpha == 0) return;

    if(hasAlphaVertexEnabled) ppline_EnableAlphaModulate((float)alpha/255.0f);
    _rwOpenGLSetRenderState(rwRENDERSTATEVERTEXALPHAENABLE, hasAlphaVertexEnabled?1:0);
    if(*rwOpenGLLightingEnabled)
    {
        _rwOpenGLLightsSetMaterialPropertiesORG(material, flags);
    }
    else
    {
        if(*rwOpenGLColorMaterialEnabled)
        {
            ppline_glDisable(0xB57u);
            *rwOpenGLColorMaterialEnabled = 0;
        }
        if(!(flags & rxGEOMETRY_PRELIT)) ppline_glColor4fv(rwOpenGLOpaqueBlack);
    }

    hasEnvMap = (*(int*)((int)material + 16) & 1)!=0;
    if(hasEnvMap)
    {
        int v15 = *(int*)((int)material + *ms_envMapPluginOffset);
        int v17 = *(int*)(v15 + 8);
        *(char*)(v17 + 81) = 17;
        ppline_SetEnvMap(*(void **)(*(int*)v17 + *ppline_RasterExtOffset), *(uint8_t*)(v15 + 4) * 0.0039216f * 1.5f, 0);
        SetNormalMatrix(*(char*)(v15 + 0) * 0.125f, *(char*)(v15 + 1) * 0.125f, RwV2d(0, 0));
        *byte_70BF3C = 0;
    }

    if(!(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED)) || material->texture == NULL)
    {
        _rwOpenGLSetRenderState(rwRENDERSTATETEXTURERASTER, false);
        DrawStoredMeshData((RxOpenGLMeshInstanceData*)meshData);
        if (hasAlphaVertexEnabled) ppline_DisableAlphaModulate();
        if (hasEnvMap)
        {
            ppline_ResetEnvMap();
            if ( !*byte_70BF3C )
            {
                ppline_glPopMatrix();
                ppline_glMatrixMode(0x1700u);
            }
        }
    }
    else if(material->texture->raster->originalStride << 31)
    {
        if (hasAlphaVertexEnabled) ppline_DisableAlphaModulate();
    }
    else
    {
        _rwOpenGLSetRenderStateNoExtras(rwRENDERSTATETEXTURERASTER, (void*)material->texture->raster);
        _rwOpenGLSetRenderState(rwRENDERSTATETEXTUREFILTER, RwTextureGetFilterModeMacro(material->texture));
        DrawStoredMeshData((RxOpenGLMeshInstanceData*)meshData);
        if (hasAlphaVertexEnabled) ppline_DisableAlphaModulate();
        if (hasEnvMap)
        {
            ppline_ResetEnvMap();
            if ( !*byte_70BF3C )
            {
                ppline_glPopMatrix();
                ppline_glMatrixMode(0x1700u);
            }
        }
    }
}

void CCustomBuildingDNPipeline__CustomPipeRenderCB_SkyGfx(RwResEntry* entry, void* obj, unsigned char type, unsigned int flags)
{
    uint16_t numMeshes = *(uint16_t*)((int)entry + 26);
    int meshData = (int)entry + 28;

    ppline_SetSecondVertexColor(1, *m_fDNBalanceParam);
    while(--numMeshes > 0)
    {
        RenderMeshes(meshData, flags);
        meshData += 56;
    }
    ppline_SetSecondVertexColor(0, 0.0f);
}

RxPipeline* CCustomBuildingDNPipeline_CreateCustomObjPipe_SkyGfx()
{
    RxPipeline* pipeline = RxPipelineCreate();
    RxNodeDefinition* OpenGLAtomicAllInOne = RxNodeDefinitionGetOpenGLAtomicAllInOne();
    if(pipeline == NULL) return NULL;

    RxLockedPipe* lockedPipeline = RxPipelineLock(pipeline);
    if(lockedPipeline == NULL || RxLockedPipeAddFragment(lockedPipeline, 0, OpenGLAtomicAllInOne, 0) == NULL || RxLockedPipeUnlock(lockedPipeline) == NULL)
    {
        _rxPipelineDestroy(pipeline);
        return NULL;
    }
    RxPipelineNode* NodeByName = RxPipelineFindNodeByName(pipeline, OpenGLAtomicAllInOne->name, 0, 0);

    RxOpenGLAllInOneSetInstanceCallBack(NodeByName, (void*)aml->GetSym(pGTASA, "_ZN25CCustomBuildingDNPipeline20CustomPipeInstanceCBEPvP24RxOpenGLMeshInstanceDataii")); // ORG
    RxOpenGLAllInOneSetRenderCallBack(NodeByName, (void*)aml->GetSym(pGTASA, "_ZN25CCustomBuildingDNPipeline18CustomPipeRenderCBEP10RwResEntryPvhj")); // ORG

    //RxOpenGLAllInOneSetInstanceCallBack(NodeByName, (void*)CCustomBuildingDNPipeline__CustomPipeInstanceCB_SkyGfx); // SkyGfx // Vertex
    //RxOpenGLAllInOneSetRenderCallBack(NodeByName, (void*)CCustomBuildingDNPipeline__CustomPipeRenderCB_SkyGfx); // SkyGfx // Fragment

    pipeline->pluginId = 0x53F20098;
    pipeline->pluginData = 0x53F20098;
    return pipeline;
}