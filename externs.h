#include <GTASA_STRUCTS.h>
#include <GLES2/gl2.h>
#include <shader.h>
#include <pipeline.h>
#include <effects.h>
#include <plantsurfprop.h>
#include <colorfilter.h>
#include <shading.h>
#include <shadows.h>

#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_SPECULAR 0x1202
#define GL_EMISSION 0x1600
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#define GL_COLOR_MATERIAL 0x0B57

#ifndef __EXTERNS
#define __EXTERNS

// Hooks
extern DECL_HOOKv(InitialisePostEffects);
extern uintptr_t pGTASA_MobileEffectsRender;
extern uintptr_t pGTASA_EffectsRender;

// Variables
extern RpLight **p_pDirect, **p_pAmbient;
extern RwRGBAReal *p_AmbientLightColourForFrame, *p_AmbientLightColourForFrame_PedsCarsAndObjects, *p_DirectionalLightColourForFrame, *p_DirectionalLightColourFromDay;
extern RsGlobalType* RsGlobal;
extern int *pnGrainStrength, *currArea;
extern bool *pbGrainEnable, *pbRainEnable, *pbInCutscene, *pbNightVision, *pbInfraredVision;
extern float *pfWeatherRain, *pfWeatherUnderwaterness;
extern CCamera *TheCamera;
extern uintptr_t pg_fx;
extern bool* pbCCTV;
extern bool* pbFog ;
extern float* m_fDNBalanceParam;
extern float* rwOpenGLOpaqueBlack;
extern int* rwOpenGLLightingEnabled;
extern int* rwOpenGLColorMaterialEnabled;
extern int* ms_envMapPluginOffset;
extern int* RasterExtOffset;
extern char* byte_70BF3C;
extern RwRaster *grainRaster ;
extern uintptr_t* pdword_952880;
extern float*  m_fDNBalanceParam;
extern CPlantSurfProp** m_SurfPropPtrTab;
extern uint32_t*      m_countSurfPropsAllocated;
extern CPlantSurfProp m_SurfPropTab[MAX_SURFACE_PROPS];
extern RwTexture*     tex_gras07Si;
extern RwTexture**    PC_PlantSlotTextureTab[4];
extern RwTexture*     PC_PlantTextureTab0[4];
extern RwTexture*     PC_PlantTextureTab1[4];
extern RpAtomic**     PC_PlantModelSlotTab[4];
extern RpAtomic*      PC_PlantModelsTab0[4];
extern RpAtomic*      PC_PlantModelsTab1[4];
extern CPlantLocTri** m_CloseLocTriListHead;
extern CPlantLocTri** m_UnusedLocTriListHead;
extern float *TimeCycShadowDispX, *TimeCycShadowDispY, *TimeCycShadowFrontX, *TimeCycShadowFrontY, *TimeCycShadowSideX, *TimeCycShadowSideY;
extern int *TimeCycCurrentStoredValue;
extern int *RTShadowsQuality;
extern RQCapabilities *RQCaps;
extern int *RQMaxBones;
extern int* deviceChip;
extern float *openglAmbientLight;
extern float _rwOpenGLOpaqueBlack[4];
extern RwInt32 *p_rwOpenGLColorMaterialEnabled;
extern CColourSet *p_CTimeCycle__m_CurrentColours;
extern CVector *p_CTimeCycle__m_vecDirnLightToSun;
extern float *p_gfLaRiotsLightMult;
extern float *p_CCoronas__LightsMult;
extern uint8_t *p_CWeather__LightningFlash;
extern float *skin_map;
extern int *skin_dirty;
extern int *skin_num;

// Functions
extern RwFrame *(*RwFrameTransform)(RwFrame * frame, const RwMatrix * m, RwOpCombineType combine);
extern RpLight *(*RpLightSetColor)(RpLight *light, const RwRGBAReal *color);
extern void (*emu_glColor4fv)(const GLfloat *v);
extern void (*emu_glLightModelfv)(GLenum pname, const GLfloat *params);
extern void (*emu_glMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
extern void (*emu_glColorMaterial)(GLenum face, GLenum mode);
extern void (*emu_glEnable)(GLenum cap);
extern void (*emu_glDisable)(GLenum cap);
extern RwRaster* (*RwRasterCreate)(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags);
extern RwUInt8* (*RwRasterLock)(RwRaster*, unsigned char, int);
extern void (*RwRasterUnlock)(RwRaster*);
extern void (*ImmediateModeRenderStatesStore)();
extern void (*ImmediateModeRenderStatesSet)();
extern void (*ImmediateModeRenderStatesReStore)();
extern void (*RwRenderStateSet)(RwRenderState, void*);
extern void (*DrawQuadSetUVs)(float,float,float,float,float,float,float,float);
extern void (*PostEffectsDrawQuad)(float,float,float,float,uint8_t,uint8_t,uint8_t,uint8_t,RwRaster*);
extern void (*DrawQuadSetDefaultUVs)(void);
extern uint8_t (*CamNoRain)();
extern uint8_t (*PlayerNoRain)();
extern void (*RenderFx)(uintptr_t, uintptr_t, unsigned char) ;
extern void (*RenderWaterCannons)() ;
extern void (*RenderWaterFog)() ;
extern void (*RenderMovingFog)() ;
extern void (*RenderVolumetricClouds)() ;
extern void (*RenderScreenFogPostEffect)() ;
extern void (*RenderCCTVPostEffect)() ;
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
extern void (*SetSecondVertexColor)(uint8_t, float);
extern void (*EnableAlphaModulate)(float);
extern void (*DisableAlphaModulate)();
extern void (*SetEnvMap)(void*, float, int);
extern void           (*FileMgrSetDir)(const char* dir);
extern FILE*          (*FileMgrOpenFile)(const char* dir, const char* ioflags);
extern void           (*FileMgrCloseFile)(FILE*);
extern char*          (*FileLoaderLoadLine)(FILE*);
extern unsigned short (*GetSurfaceIdFromName)(void* trash, const char* surfname);
extern uintptr_t      (*LoadTextureDB)(const char* dbFile, bool fullLoad, int txdbFormat);
extern uintptr_t      (*GetTextureDB)(const char* dbFile);
extern void           (*RegisterTextureDB)(uintptr_t dbPtr);
extern void           (*UnregisterTextureDB)(uintptr_t dbPtr);
extern RwTexture*     (*GetTextureFromTextureDB)(const char* texture);
extern int            (*AddImageToList)(const char* imgName, bool isPlayerImg);
extern int            (*SetPlantModelsTab)(unsigned int, RpAtomic**);
extern int            (*SetCloseFarAlphaDist)(float, float);
extern void           (*FlushTriPlantBuffer)();
extern void           (*DeActivateDirectional)();
extern void           (*SetAmbientColours)(RwRGBAReal*);
extern bool (*IsSphereVisibleForCamera)(CCamera*, const CVector*, float);
extern void (*AddTriPlant)(PPTriPlant*, unsigned int);
extern void (*MoveLocTriToList)(CPlantLocTri*& oldList, CPlantLocTri*& newList, CPlantLocTri* triangle);
extern void (*StreamingMakeSpaceFor)(int);
extern RwStream* (*RwStreamOpen)(int, int, const char*);
extern bool (*RwStreamFindChunk)(RwStream*, int, int, int);
extern RpClump* (*RpClumpStreamRead)(RwStream*);
extern void (*RwStreamClose)(RwStream*, int);
extern RpAtomic* (*GetFirstAtomic)(RpClump*);
extern void (*SetFilterModeOnAtomicsTextures)(RpAtomic*, int);
extern void (*RpGeometryLock)(RpGeometry*, int);
extern void (*RpGeometryUnlock)(RpGeometry*);
extern void (*RpGeometryForAllMaterials)(RpGeometry*, RpMaterial* (*)(RpMaterial*, void*), void*);
extern void (*RpMaterialSetTexture)(RpMaterial*, RwTexture*);
extern RpAtomic* (*RpAtomicClone)(RpAtomic*);
extern void (*RpClumpDestroy)(RpClump*);
extern RwFrame* (*RwFrameCreate)();
extern void (*RpAtomicSetFrame)(RpAtomic*, RwFrame*);
extern void (*RenderAtomicWithAlpha)(RpAtomic*, int alphaVal);
extern RpGeometry* (*RpGeometryCreate)(int, int, unsigned int); //
extern RpMaterial* (*RpGeometryTriangleGetMaterial)(RpGeometry*, RpTriangle*); //
extern void (*RpGeometryTriangleSetMaterial)(RpGeometry*, RpTriangle*, RpMaterial*); //
extern void (*RpAtomicSetGeometry)(RpAtomic*, RpGeometry*, unsigned int);
extern RwCamera* (*CreateShadowCamera)(ShadowCameraStorage*, int size);
extern void (*DestroyShadowCamera)(CShadowCamera*);
extern void (*MakeGradientRaster)(ShadowCameraStorage*);
extern void (*_rwObjectHasFrameSetFrame)(void*, RwFrame*);
extern void (*RwFrameDestroy)(RwFrame*);
extern void (*RpLightDestroy)(RpLight*);
extern void (*CreateRTShadow)(CRealTimeShadow* shadow, int size, bool blur, int blurPasses, bool hasGradient);
extern void (*SetShadowedObject)(CRealTimeShadow* shadow, CPhysical* physical);
extern int (*CamDistComp)(const void*, const void*);
extern bool (*StoreRealTimeShadow)(CPhysical* physical, float shadowDispX, float shadowDispY, float shadowFrontX, float shadowFrontY, float shadowSideX, float shadowSideY);
extern void (*UpdateRTShadow)(CRealTimeShadow* shadow);
extern int (*GetMobileEffectSetting)();
extern void (*EntityPreRender)(CEntity* entity);

// Main
void ResolveExternals();

#endif // __EXTERNS