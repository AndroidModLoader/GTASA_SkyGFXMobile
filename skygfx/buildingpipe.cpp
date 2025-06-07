#include <externs.h>
#include "include/renderqueue.h"

/* Variables */


/* Configs */

/* Hooks */

/* Functions */
inline void RenderMeshes(RxOpenGLMeshInstanceData* meshData, bool dualPass)
{
    if(!dualPass) return DrawStoredMeshData(meshData);
    
    RwBool hasAlpha = rwIsAlphaBlendOn(); // ERQ_HasAlphaBlending();
    //int alphafunc, alpharef;
    int zwrite;

    GLenum alphaFunc = *currentAlphaFunc;
    float alphaRef = *currentAlphaFuncVal;

    _rwOpenGLGetRenderState(rwRENDERSTATEZWRITEENABLE, &zwrite);
    //_rwOpenGLGetRenderState(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);

    if(hasAlpha && zwrite)
    {
        //_rwOpenGLGetRenderState(rwRENDERSTATEALPHATESTFUNCTIONREF, &alpharef);
        //RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)128);
        //RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATEREQUAL);
        RQ_SetAlphaTest(GL_GEQUAL, 0.5f);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
        DrawStoredMeshData(meshData);
        //RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONLESS);
        RQ_SetAlphaTest(GL_LESS, 0.5f);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)false);
        DrawStoredMeshData(meshData);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)(intptr_t)zwrite);
        //RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)(intptr_t)alpharef);
        //RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)(intptr_t)alphafunc);
        RQ_SetAlphaTest(alphaFunc, alphaRef);
    }
    else if(!zwrite)
    {
        //RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
        RQ_SetAlphaTest(GL_ALWAYS, 1.0f);
        DrawStoredMeshData(meshData);
        //RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)(intptr_t)alphafunc);
        RQ_SetAlphaTest(alphaFunc, alphaRef);
    }
    else
    {
        DrawStoredMeshData(meshData);
    }
}

/* Main */
void StartBuildingPipeline()
{
    
}