#include <externs.h>
#include "include/renderqueue.h"

/* Others */
enum
{
    DPASS_DISABLED = 0,
    DPASS_ENABLED,
    DPASS_EXTRA,

    DPASS_MAX
};

/* Variables */
int g_nLastBuildingDistSq = 99999999;
int g_nBuildingDualPassDistSq = 50 * 50; // "CMP int,int" gives more performance (not much but still)
int g_nDualPass = DPASS_DISABLED;
float g_fDualPassThreshold = 0.5f;
const char* aDualPassSwitch[DPASS_MAX] = 
{
    "Disabled",
    "Enabled (optimised)",
    "Extra Distance (-FPS)",
};

/* Configs */
ConfigEntry* pCFGDualPass;

/* Functions */
void DualPassSettingChanged(int oldVal, int newVal, void* data)
{
    if(oldVal == newVal) return;

    pCFGDualPass->SetInt(newVal);
    pCFGDualPass->Clamp(0, DPASS_MAX - 1);
    g_nDualPass = pCFGDualPass->GetInt();

    if(g_nDualPass == DPASS_EXTRA) g_nBuildingDualPassDistSq = 120 * 120;
    else                           g_nBuildingDualPassDistSq = 50 * 50;

    cfg->Save();
}
inline void RenderMeshes(RxOpenGLMeshInstanceData* meshData, bool dualPass)
{
    if(!dualPass) return DrawStoredMeshData(meshData);

    int zwrite = 0;
    GLenum alphaFunc = *currentAlphaFunc;
    float alphaRef = *currentAlphaFuncVal;
    _rwOpenGLGetRenderState(rwRENDERSTATEZWRITEENABLE, &zwrite);
    if(zwrite)
    {
        RQ_SetAlphaTest(GL_GEQUAL, g_fDualPassThreshold);
        DrawStoredMeshData(meshData);
        RQ_SetAlphaTest(GL_LESS, g_fDualPassThreshold);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)false);
        DrawStoredMeshData(meshData);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
        RQ_SetAlphaTest(alphaFunc, alphaRef);
    }
    else
    {
        RQ_SetAlphaTest(GL_ALWAYS, 1.0f);
        DrawStoredMeshData(meshData);
        RQ_SetAlphaTest(alphaFunc, alphaRef);
    }

    /*int zwrite = 0;
    RwBool hasAlpha = rwIsAlphaBlendOn();

    GLenum alphaFunc = *currentAlphaFunc;
    float alphaRef = *currentAlphaFuncVal;

    _rwOpenGLGetRenderState(rwRENDERSTATEZWRITEENABLE, &zwrite);
    if(hasAlpha && zwrite)
    {
        RQ_SetAlphaTest(GL_GEQUAL, 0.5f);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
        DrawStoredMeshData(meshData);
        RQ_SetAlphaTest(GL_LESS, 0.5f);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)false);
        DrawStoredMeshData(meshData);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)(intptr_t)zwrite);
        RQ_SetAlphaTest(alphaFunc, alphaRef);
    }
    else if(!zwrite)
    {
        RQ_SetAlphaTest(GL_ALWAYS, 1.0f);
        DrawStoredMeshData(meshData);
        RQ_SetAlphaTest(alphaFunc, alphaRef);
    }
    else
    {
        DrawStoredMeshData(meshData);
    }*/
}

/* Hooks */
static RwV2d nullVec2 = RwV2d(0, 0);
DECL_HOOKv(CCustomBuildingDNPipeline_CustomPipeRenderCB, RwResEntry* entry, void* obj, unsigned char type, unsigned int flags)
{
    int numMeshes = *(uint16_t*)( (uintptr_t)entry + BYBIT(26, 50) );
    if(!numMeshes) return;

    RxOpenGLMeshInstanceData* meshData = *(RxOpenGLMeshInstanceData**)( (uintptr_t)entry + BYBIT(28, 52) );
    SetSecondVertexColor(1, *m_fDNBalanceParam);
    while(--numMeshes >= 0)
    {
        RpMaterial* material = meshData->m_pMaterial;
        uint8_t alpha = material->color.alpha;
        int vertexColor = meshData->m_nVertexAlpha;
        bool hasAlphaVertexEnabled = (vertexColor || alpha != 255);

        if(hasAlphaVertexEnabled && alpha == 0) continue; // Fully invisible

        RwTexture* matTex = material->texture;

        if(hasAlphaVertexEnabled) EnableAlphaModulate(0.00392156862f * alpha);
        _rwOpenGLSetRenderState(rwRENDERSTATEVERTEXALPHAENABLE, hasAlphaVertexEnabled);
        if(*rwOpenGLLightingEnabled)
        {
            _rwOpenGLLightsSetMaterialPropertiesORG(material, flags);
        }
        else
        {
            if(*rwOpenGLColorMaterialEnabled)
            {
                emu_glDisable(0x0B57); // GL_COLOR_MATERIAL
                *rwOpenGLColorMaterialEnabled = 0;
            }
            if(!(flags & rxGEOMETRY_PRELIT)) emu_glColor4fv(rwOpenGLOpaqueBlack);
        }

        bool hasEnvMap = *(int*)&material->surfaceProps.specular & 1;
        if(hasEnvMap)
        {
            uintptr_t v15 = *(uintptr_t*)((uintptr_t)material + *ms_envMapPluginOffset);
            RwTexture* v17 = *(RwTexture**)(v15 + 8);
            v17->filterMode2 = 17;
            SetEnvMap(*(void **)(*(uintptr_t*)v17 + *RasterExtOffset), *(uint8_t*)(v15 + 4) * 0.0058824f, 0);
            SetNormalMatrix(*(char*)(v15) * 0.125f, *(char*)(v15 + 1) * 0.125f, nullVec2);
            *doPop = 0;
        }

        if(!(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED)) || matTex == NULL)
        {
            _rwOpenGLSetRenderState(rwRENDERSTATETEXTURERASTER, false);
            DrawStoredMeshData(meshData);
            if (hasAlphaVertexEnabled) DisableAlphaModulate();
            if (hasEnvMap) ResetEnvMap();
        }
        else if((matTex->raster->privateFlags & 0x1) != 0)
        {
            if (hasAlphaVertexEnabled) DisableAlphaModulate();
        }
        else
        {
            _rwOpenGLSetRenderStateNoExtras(rwRENDERSTATETEXTURERASTER, (void*)matTex->raster);
            _rwOpenGLSetRenderState(rwRENDERSTATETEXTUREFILTER, matTex->filterMode);
            
            RenderMeshes(meshData, g_nLastBuildingDistSq < g_nBuildingDualPassDistSq);

            if (hasAlphaVertexEnabled) DisableAlphaModulate();
            if (hasEnvMap) ResetEnvMap();
        }
        ++meshData;
    }
    SetSecondVertexColor(0, 0.0f);
}
DECL_HOOKv(RenderBuilding, CBuilding* self)
{
    if(g_nDualPass == DPASS_DISABLED) return RenderBuilding(self);

    g_nLastBuildingDistSq = (int) (FindPlayerPed(-1)->GetPosition() - self->GetPosition()).MagnitudeSqr2D();
    RenderBuilding(self);
    g_nLastBuildingDistSq = 99999999;
}
DECL_HOOKv(RenderDNBuildingMesh, RxOpenGLMeshInstanceData* meshData)
{
    bool doDual = (g_nDualPass != DPASS_DISABLED && g_nLastBuildingDistSq < g_nBuildingDualPassDistSq);
    if(doDual)
    {
        RwRaster* curRaster = RenderState->texRaster[RenderState->activeTexUnit];
        doDual = curRaster && (!curRaster->dbEntry || curRaster->dbEntry->alphaFormat > 1);
    }
    RenderMeshes(meshData, doDual);
}

/* Main */
void StartBuildingPipeline()
{
    // TODO: cant release it yet, some textures are broken again with dualpass

    pCFGDualPass = cfg->Bind("BuildingDualPass", DPASS_DISABLED, "Pipeline");
    DualPassSettingChanged(DPASS_DISABLED, pCFGDualPass->GetInt(), NULL);
    AddSetting("Building DualPass", g_nDualPass, 0, sizeofA(aDualPassSwitch)-1, aDualPassSwitch, DualPassSettingChanged, NULL);

    g_fDualPassThreshold = cfg->GetFloat("DualPassThreshold", g_fDualPassThreshold, "Pipeline");
    
    //HOOKPLT(CCustomBuildingDNPipeline_CustomPipeRenderCB, pGTASA + BYBIT(0x677CB4, 0x84D988)); // mesh->material crash??
    HOOKPLT(RenderBuilding, pGTASA + BYBIT(0x662014, 0x824F48));
    HOOKBLX(RenderDNBuildingMesh, pGTASA + BYBIT(0x2CA983, 0x38BEC4)); // doing this temporarily (reason: upper)
}
