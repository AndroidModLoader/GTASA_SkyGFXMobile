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

enum ePipelineDualpassWay : uint8_t
{
    DPWay_Default = 0,
    DPWay_Alpha,
    DPWay_Everything,
    DPWay_LastMesh,

    DPWay_MaxWays,
};
extern ePipelineDualpassWay pipelineWay;

RxPipeline* CCustomBuildingDNPipeline_CreateCustomObjPipe_SkyGfx();

extern const char* pPipelineSettings[4];
void PipelineChanged(int oldVal, int newVal);

extern void              (*_rxPipelineDestroy)(RxPipeline*);
extern RxPipeline*       (*RxPipelineCreate)();
extern RxLockedPipe*     (*RxPipelineLock)(RxPipeline*);
extern RxNodeDefinition* (*RxNodeDefinitionGetOpenGLAtomicAllInOne)();
extern void*             (*RxLockedPipeAddFragment)(RxLockedPipe*, int, RxNodeDefinition*, int);
extern RxLockedPipe*     (*RxLockedPipeUnlock)(RxLockedPipe*);
extern RxPipelineNode*   (*RxPipelineFindNodeByName)(RxPipeline*, const char*, int, int);
extern void              (*RxOpenGLAllInOneSetInstanceCallBack)(RxPipelineNode*, void*);
extern void              (*RxOpenGLAllInOneSetRenderCallBack)(RxPipelineNode*, void*);
extern void              (*_rwOpenGLSetRenderState)(RwRenderState, int); //
extern void              (*_rwOpenGLGetRenderState)(RwRenderState, void*); //
extern void              (*_rwOpenGLSetRenderStateNoExtras)(RwRenderState, void*); //
extern void              (*_rwOpenGLLightsSetMaterialPropertiesORG)(const RpMaterial *mat, RwUInt32 flags); //
extern void              (*SetNormalMatrix)(float, float, RwV2d); //
extern void              (*DrawStoredMeshData)(RxOpenGLMeshInstanceData*);
extern void              (*ResetEnvMap)();

extern float* m_fDNBalanceParam;
extern float* rwOpenGLOpaqueBlack;
extern int*   rwOpenGLLightingEnabled;
extern int*   rwOpenGLColorMaterialEnabled;
extern int*   ms_envMapPluginOffset;
extern int*   ppline_RasterExtOffset;
extern char*  byte_70BF3C;
// Start emu_*
extern void (*ppline_SetSecondVertexColor)(uint8_t, float);
extern void (*ppline_EnableAlphaModulate)(float);
extern void (*ppline_DisableAlphaModulate)();
extern void (*ppline_glDisable)(uint32_t);
extern void (*ppline_glColor4fv)(float*);
extern void (*ppline_SetEnvMap)(void*, float, int);
// End emu_*