#include <externs.h>
#include "include/renderqueue.h"

/* Variables */

/* Configs */

/* Functions */
inline void RenderMeshes(RxOpenGLMeshInstanceData* meshData, bool dualPass)
{
    if(!dualPass) return DrawStoredMeshData(meshData);
    
    int zwrite = 0;
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
    }
}

/* Hooks */
DECL_HOOKv(DrawStored_BuildingPipe, RxOpenGLMeshInstanceData* meshData)
{
    RenderMeshes(meshData, true);
}

/* Main */
void StartBuildingPipeline()
{
    //HOOKBL(DrawStored_BuildingPipe, pGTASA + 0x38BEC4); // test
    //HOOKBL(DrawStored_BuildingPipe, pGTASA + 0x38CAAC); // test
}