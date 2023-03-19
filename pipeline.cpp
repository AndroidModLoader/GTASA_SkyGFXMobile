#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <gtasa_things.h>

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
void              (*ResetEnvMap)();

extern ConfigEntry* pPS2PipelineRenderWay;
const char* pPipelineSettings[4] = 
{
    "Disabled",
    "For Alpha",
    "For Everything",
    "For Last Meshes",
};
void PipelineChanged(int oldVal, int newVal, void* data)
{
    pPS2PipelineRenderWay->SetInt(newVal);
    pipelineWay = (ePipelineDualpassWay)newVal;
    cfg->Save();
}

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
// End emu_*


ePipelineDualpassWay pipelineWay = DPWay_Default;

int CCustomBuildingDNPipeline__CustomPipeInstanceCB_SkyGfx(void* object, RxOpenGLMeshInstanceData* meshData, int, int reinstance)
{
    // Still nothing because PC's Pipe and Mobile's are COMPLETELY different :(
    return 1;
}

void CCustomBuildingDNPipeline__CustomPipeRenderCB_SkyGfx(RwResEntry* entry, void* obj, unsigned char type, unsigned int flags)
{
    int numMeshes = entry->meshesNum;
    RxOpenGLMeshInstanceData* meshData = entry->meshes;

    ppline_SetSecondVertexColor(1, *m_fDNBalanceParam);
    while(--numMeshes >= 0)
    {
        RpMaterial* material = meshData->m_pMaterial;
        uint8_t alpha = material->color.alpha;
        int vertexColor = meshData->m_nVertexColor, alphafunc;
        bool hasAlphaVertexEnabled = vertexColor || alpha != 255;

        if(hasAlphaVertexEnabled && alpha == 0) continue; // Fully invisible

        if(hasAlphaVertexEnabled) ppline_EnableAlphaModulate(0.00392156862f * alpha);
        _rwOpenGLSetRenderState(rwRENDERSTATEVERTEXALPHAENABLE, hasAlphaVertexEnabled);
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

        bool hasEnvMap = *(int*)&material->surfaceProps.specular & 1;
        //bool hasEnvMap = material->surfaceProps.specular != 0;
        if(hasEnvMap)
        {
            int v15 = *(int*)((int)material + *ms_envMapPluginOffset);
            RwTexture* v17 = *(RwTexture**)(v15 + 8);
            v17->filterMode2 = 17;
            ppline_SetEnvMap(*(void **)(*(int*)v17 + *ppline_RasterExtOffset), *(uint8_t*)(v15 + 4) * 0.0058824f, 0);
            SetNormalMatrix(*(char*)(v15 + 0) * 0.125f, *(char*)(v15 + 1) * 0.125f, RwV2d(0, 0));
            *byte_70BF3C = 0;
        }

        if(!(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED)) || material->texture == NULL)
        {
            _rwOpenGLSetRenderState(rwRENDERSTATETEXTURERASTER, false);
            DrawStoredMeshData(meshData);
            if (hasAlphaVertexEnabled) ppline_DisableAlphaModulate();
            if (hasEnvMap) ResetEnvMap();
        }
        else if(material->texture->raster->originalStride << 31)
        {
            if (hasAlphaVertexEnabled) ppline_DisableAlphaModulate();
        }
        else
        {
            _rwOpenGLSetRenderStateNoExtras(rwRENDERSTATETEXTURERASTER, (void*)material->texture->raster);
            _rwOpenGLSetRenderState(rwRENDERSTATETEXTUREFILTER, material->texture->filterMode);
            int zwrite;
            _rwOpenGLGetRenderState(rwRENDERSTATEZWRITEENABLE, &zwrite);
            _rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTIONREF, 1);
            switch(pipelineWay)
            {
                default:
                    DrawStoredMeshData(meshData);
                    break;

                case DPWay_Alpha:
                    if(!hasAlphaVertexEnabled)
                    {
                        DrawStoredMeshData(meshData);
                    }
                    else
                    {
                        _rwOpenGLGetRenderState(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
                        _rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, rwALPHATESTFUNCTIONGREATEREQUAL);
                        //DrawStoredMeshData(meshData);
                        _rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, rwALPHATESTFUNCTIONLESS);
                        _rwOpenGLSetRenderState(rwRENDERSTATEZWRITEENABLE, false);
                        DrawStoredMeshData(meshData);
                        _rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, alphafunc);
                        _rwOpenGLSetRenderState(rwRENDERSTATEZWRITEENABLE, true);
                    }
                    break;

                case DPWay_Everything:
                    _rwOpenGLGetRenderState(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
                    _rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, rwALPHATESTFUNCTIONGREATEREQUAL);
                    //DrawStoredMeshData(meshData);
                    _rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, rwALPHATESTFUNCTIONLESS);
                    _rwOpenGLSetRenderState(rwRENDERSTATEZWRITEENABLE, false);
                    DrawStoredMeshData(meshData);
                    _rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, alphafunc);
                    _rwOpenGLSetRenderState(rwRENDERSTATEZWRITEENABLE, true);
                    break;

                case DPWay_LastMesh:
                    if(numMeshes > 1)
                    {
                        DrawStoredMeshData(meshData);
                        break;
                    }
                    _rwOpenGLGetRenderState(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
                    _rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, rwALPHATESTFUNCTIONGREATEREQUAL);
                    //DrawStoredMeshData(meshData);
                    _rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, rwALPHATESTFUNCTIONLESS);
                    _rwOpenGLSetRenderState(rwRENDERSTATEZWRITEENABLE, false);
                    DrawStoredMeshData(meshData);
                    _rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, alphafunc);
                    _rwOpenGLSetRenderState(rwRENDERSTATEZWRITEENABLE, true);
                    break;
            }
            if (hasAlphaVertexEnabled) ppline_DisableAlphaModulate();
            if (hasEnvMap) ResetEnvMap();
        }

        ++meshData;
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

    RxOpenGLAllInOneSetInstanceCallBack(NodeByName, (void*)aml->GetSym(hGTASA, "_ZN25CCustomBuildingDNPipeline20CustomPipeInstanceCBEPvP24RxOpenGLMeshInstanceDataii")); // ORG
    //RxOpenGLAllInOneSetRenderCallBack(NodeByName, (void*)aml->GetSym(hGTASA, "_ZN25CCustomBuildingDNPipeline18CustomPipeRenderCBEP10RwResEntryPvhj")); // ORG

    //RxOpenGLAllInOneSetInstanceCallBack(NodeByName, (void*)CCustomBuildingDNPipeline__CustomPipeInstanceCB_SkyGfx); // SkyGfx // Vertex
    RxOpenGLAllInOneSetRenderCallBack(NodeByName, (void*)CCustomBuildingDNPipeline__CustomPipeRenderCB_SkyGfx); // SkyGfx // Fragment

    pipeline->pluginId = 0x53F20098;
    pipeline->pluginData = 0x53F20098;
    return pipeline;
}
