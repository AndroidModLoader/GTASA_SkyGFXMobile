class RxPipeline
{

};

class RxLockedPipe
{

};

class RxNodeDefinition
{
public:
    const char* name;
};

class RxPipelineNode
{

};

RxPipeline* CCustomBuildingDNPipeline_CreateCustomObjPipe_SkyGfx();

extern void (*_rxPipelineDestroy)(RxPipeline*);
extern RxPipeline* (*RxPipelineCreate)();
extern RxLockedPipe* (*RxPipelineLock)(RxPipeline*);
extern RxNodeDefinition* (*RxNodeDefinitionGetOpenGLAtomicAllInOne)();
extern void* (*RxLockedPipeAddFragment)(RxLockedPipe*, int, RxNodeDefinition*, int);
extern RxLockedPipe* (*RxLockedPipeUnlock)(RxLockedPipe*);
extern RxPipelineNode* (*RxPipelineFindNodeByName)(RxPipeline*, const char*, int, int);
extern void (*RxOpenGLAllInOneSetInstanceCallBack)(RxPipelineNode*, void*);
extern void (*RxOpenGLAllInOneSetRenderCallBack)(RxPipelineNode*, void*);