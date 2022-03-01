#include <mod/amlmod.h>
#include <mod/config.h>
#include <gtasa_things.h>
#include "GTASA_STRUCTS.h"

#include "pipeline.h"

void (*_rxPipelineDestroy)(RxPipeline*);
RxPipeline* (*RxPipelineCreate)();
RxLockedPipe* (*RxPipelineLock)(RxPipeline*);
RxNodeDefinition* (*RxNodeDefinitionGetOpenGLAtomicAllInOne)();
void* (*RxLockedPipeAddFragment)(RxLockedPipe*, int, RxNodeDefinition*, int);
RxLockedPipe* (*RxLockedPipeUnlock)(RxLockedPipe*);
RxPipelineNode* (*RxPipelineFindNodeByName)(RxPipeline*, const char*, int, int);
void (*RxOpenGLAllInOneSetInstanceCallBack)(RxPipelineNode*, void*);
void (*RxOpenGLAllInOneSetRenderCallBack)(RxPipelineNode*, void*);

//bool CCustomBuildingDNPipeline__CustomPipeInstanceCB_SkyGfx(uintptr_t object, uintptr_t meshData, int, bool reinstance)
//{
//
//}

//int CCustomBuildingDNPipeline__CustomPipeRenderCB_SkyGfx(uintptr_t entry, void*, unsigned char, unsigned int flags)
//{
//    // Still nothing because PC's Pipe and Mobile's are COMPLETELY different :(
//}

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
    //RxOpenGLAllInOneSetInstanceCallBack(NodeByName, (void*)CCustomBuildingDNPipeline__CustomPipeInstanceCB_SkyGfx); // SkyGfx (nothing for instance is changed actually. for yet?)
    RxOpenGLAllInOneSetInstanceCallBack(NodeByName, (void*)aml->GetSym(pGTASA, "_ZN25CCustomBuildingDNPipeline20CustomPipeInstanceCBEPvP24RxOpenGLMeshInstanceDataii")); // ORG
    //RxOpenGLAllInOneSetRenderCallBack(NodeByName, (void*)CCustomBuildingDNPipeline__CustomPipeRenderCB_SkyGfx); // SkyGfx
    RxOpenGLAllInOneSetRenderCallBack(NodeByName, (void*)aml->GetSym(pGTASA, "_ZN25CCustomBuildingDNPipeline18CustomPipeRenderCBEP10RwResEntryPvhj")); // ORG

    *(int*)((uintptr_t)pipeline + 44) = 0x53F20098;
    *(int*)((uintptr_t)pipeline + 48) = 0x53F20098;
    return pipeline;
}