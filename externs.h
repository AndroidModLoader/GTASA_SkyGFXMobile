#ifndef __EXTERNS
#define __EXTERNS

#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <list>
#include <skygfx/include/fastcat.h>

#include <GLES2/gl2.h>

#ifdef AML32
    #include "GTASA_STRUCTS.h"
    #define BYVER(__for32, __for64) (__for32)
#else
    #include "GTASA_STRUCTS_210.h"
    #define BYVER(__for32, __for64) (__for64)
#endif
#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))



extern uintptr_t pGTASA;
extern void* hGTASA;

template <typename T>
struct BasicEvent
{
    std::list<T> listeners;

    inline void Call()
    {
        auto end = listeners.end();
        for(auto it = listeners.begin(); it != end; ++it) (*it)();
    }
    inline void operator+=(T fn) { listeners.push_back(fn); }
};

typedef void (*SimpleVoidFn)();
extern BasicEvent<SimpleVoidFn> shadercreation;

// Shaders
#define FLAG_ALPHA_TEST           0x01
#define FLAG_LIGHTING             0x02
#define FLAG_ALPHA_MODULATE       0x04
#define FLAG_COLOR_EMISSIVE       0x08
#define FLAG_COLOR                0x10
#define FLAG_TEX0                 0x20
#define FLAG_ENVMAP               0x40          // normal envmap
#define FLAG_BONE3                0x80
#define FLAG_BONE4                0x100
#define FLAG_CAMERA_BASED_NORMALS 0x200
#define FLAG_FOG                  0x400
#define FLAG_TEXBIAS              0x800
#define FLAG_BACKLIGHT            0x1000
#define FLAG_LIGHT1               0x2000
#define FLAG_LIGHT2               0x4000
#define FLAG_LIGHT3               0x8000
#define FLAG_DETAILMAP            0x10000
#define FLAG_COMPRESSED_TEXCOORD  0x20000
#define FLAG_PROJECT_TEXCOORD     0x40000
#define FLAG_WATER                0x80000
#define FLAG_COLOR2               0x100000
#define FLAG_SPHERE_XFORM         0x800000      // this renders the scene as a sphere map for vehicle reflections
#define FLAG_SPHERE_ENVMAP        0x1000000     // spherical real-time envmap
#define FLAG_TEXMATRIX            0x2000000
#define FLAG_GAMMA                0x4000000

#define PXL_EMIT(__v) t = concatf(t, __v "\n", sizeof(__v)+1)
#define VTX_EMIT(__v) t = concatf(t, __v "\n", sizeof(__v)+1)

#define PXL_EMIT_V(...)                      \
  do {                                       \
    snprintf(tmp, sizeof(tmp), __VA_ARGS__); \
    t=concatf(t, tmp, strlen(tmp)+1);        \
    t=concatf(t, "\n", 2);                   \
  } while (0)


#define VTX_EMIT_V(...)                      \
  do {                                       \
    snprintf(tmp, sizeof(tmp), __VA_ARGS__); \
    t=concatf(t, tmp, strlen(tmp)+1);        \
    t=concatf(t, "\n", 2);                   \
  } while (0)

extern ES2Shader* pForcedShader;
ES2Shader* CreateCustomShaderAlloc(uint32_t flags, const char* pxlsrc, const char* vtxsrc, size_t pxllen = 0, size_t vtxlen = 0);
ES2Shader* CreateCustomShader(uint32_t flags, const char* pxlsrc, const char* vtxsrc);
inline void ForceCustomShader(ES2Shader* shader)
{
    pForcedShader = shader;
}

#if __has_include(<isautils.h>)
    #include <isautils.h>
    extern ISAUtils* sautils;
    extern eTypeOfSettings skygfxSettingsTab;
    inline void AddSetting(const char* name, int initVal, int minVal, int maxVal, const char** switchesArray = NULL, OnSettingChangedFn fnOnValueChange = NULL, void* data = NULL)
    {
        if(!sautils) return;
        sautils->AddClickableItem(skygfxSettingsTab, name, initVal, minVal, maxVal, switchesArray, fnOnValueChange, data);
    }
#else
    inline void AddSetting(...)
    {
        /* No SAUtils */
    }
#endif

#define NAKEDAT __attribute__((optnone)) __attribute__((naked))
#define rpPDS_MAKEPIPEID(vendorID, pipeID)              \
    (((vendorID & 0xFFFF) << 16) | (pipeID & 0xFFFF))

enum RockstarPipelineIDs
{
    VEND_ROCKSTAR     = 0x0253F2,

    // PS2 pipes
    // building
    PDS_PS2_CustomBuilding_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x80),
    PDS_PS2_CustomBuilding_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x81),
    PDS_PS2_CustomBuildingDN_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x82),
    PDS_PS2_CustomBuildingDN_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x83),
    PDS_PS2_CustomBuildingEnvMap_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8C),
    PDS_PS2_CustomBuildingEnvMap_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8D),
    PDS_PS2_CustomBuildingDNEnvMap_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8E),
    PDS_PS2_CustomBuildingDNEnvMap_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8F),
    PDS_PS2_CustomBuildingUVA_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x90),
    PDS_PS2_CustomBuildingUVA_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x91),
    PDS_PS2_CustomBuildingDNUVA_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x92),
    PDS_PS2_CustomBuildingDNUVA_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x93),
    // car
    PDS_PS2_CustomCar_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x84),
    PDS_PS2_CustomCar_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x85),
    PDS_PS2_CustomCarEnvMap_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x86),
    PDS_PS2_CustomCarEnvMap_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x87),
//    PDS_PS2_CustomCarEnvMapUV2_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8A),    // this does not exist
    PDS_PS2_CustomCarEnvMapUV2_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8B),
    // skin
    PDS_PS2_CustomSkinPed_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x88),
    PDS_PS2_CustomSkinPed_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x89),

    // PC pipes
    RSPIPE_PC_CustomBuilding_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x9C),
    RSPIPE_PC_CustomBuildingDN_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x98),
    RSPIPE_PC_CustomCarEnvMap_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x9A),    // same as XBOX!

    // Xbox pipes
    RSPIPE_XBOX_CustomBuilding_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x9E),
    RSPIPE_XBOX_CustomBuildingDN_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x96),
    RSPIPE_XBOX_CustomBuildingEnvMap_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0xA0),
    RSPIPE_XBOX_CustomBuildingDNEnvMap_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0xA2),
    RSPIPE_XBOX_CustomCarEnvMap_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x9A),    // same as PC!
};

enum BuildingPipeline
{
    BUILDING_MOBILE,
    BUILDING_PS2,
    BUILDING_XBOX,

    NUMBUILDINGPIPES
};

#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_SPECULAR 0x1202
#define GL_EMISSION 0x1600
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#define GL_COLOR_MATERIAL 0x0B57

// Structures
struct ShadowCameraStorage
{
    CShadowCamera* m_pShadowCamera;
    RwTexture*     m_pShadowTexture;
};

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
extern bool* pbFog;
extern float* m_fDNBalanceParam;
extern float* rwOpenGLOpaqueBlack;
extern int* rwOpenGLLightingEnabled;
extern int* rwOpenGLColorMaterialEnabled;
extern int* ms_envMapPluginOffset;
extern int* RasterExtOffset;
extern char* doPop;
extern RwRaster *grainRaster;
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
extern bool *LightningFlash;
extern int *ms_extraVertColourPluginOffset;
extern bool *RwHackNoCompressedTexCoords;
extern RQShader **curSelectedShader;
extern float *AmbientLightColor;
extern bool *AmbientLightDirty;
extern float *ms_fFarClip;
extern RenderQueue** renderQueue;
extern RwRaster** pRasterFrontBuffer;
extern int *curActiveTexture;
extern int *boundTextures;
extern GlobalSceneTag* Scene;
extern GLenum* currentAlphaFunc;
extern float* currentAlphaFuncVal;

// Functions
extern RwFrame*            (*RwFrameTransform)(RwFrame * frame, const RwMatrix * m, RwOpCombineType combine);
extern RpLight*            (*RpLightSetColor)(RpLight *light, const RwRGBAReal *color);
extern void                (*emu_glColor4fv)(const GLfloat *v);
extern void                (*emu_glLightModelfv)(GLenum pname, const GLfloat *params);
extern void                (*emu_glMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
extern void                (*emu_glColorMaterial)(GLenum face, GLenum mode);
extern void                (*emu_glEnable)(GLenum cap);
extern void                (*emu_glDisable)(GLenum cap);
extern RwRaster*           (*RwRasterCreate)(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags);
extern RwUInt8*            (*RwRasterLock)(RwRaster*, unsigned char, int);
extern void                (*RwRasterUnlock)(RwRaster*);
extern void                (*ImmediateModeRenderStatesStore)();
extern void                (*ImmediateModeRenderStatesSet)();
extern void                (*ImmediateModeRenderStatesReStore)();
extern void                (*RwRenderStateSet)(RwRenderState, void*);
extern void                (*DrawQuadSetUVs)(float,float,float,float,float,float,float,float);
extern void                (*PostEffectsDrawQuad)(float,float,float,float,uint8_t,uint8_t,uint8_t,uint8_t,RwRaster*);
extern void                (*DrawQuadSetDefaultUVs)(void);
extern uint8_t             (*CamNoRain)();
extern uint8_t             (*PlayerNoRain)();
extern void                (*RenderFx)(uintptr_t, uintptr_t, unsigned char);
extern void                (*RenderWaterCannons)();
extern void                (*RenderWaterFog)();
extern void                (*RenderMovingFog)();
extern void                (*RenderVolumetricClouds)();
extern void                (*RenderScreenFogPostEffect)();
extern void                (*RenderCCTVPostEffect)();
extern void                (*_rxPipelineDestroy)(RxPipeline*);
extern RxPipeline*         (*RxPipelineCreate)();
extern RxLockedPipe*       (*RxPipelineLock)(RxPipeline*);
extern RxNodeDefinition*   (*RxNodeDefinitionGetOpenGLAtomicAllInOne)();
extern void*               (*RxLockedPipeAddFragment)(RxLockedPipe*, int, RxNodeDefinition*, int);
extern RxLockedPipe*       (*RxLockedPipeUnlock)(RxLockedPipe*);
extern RxPipelineNode*     (*RxPipelineFindNodeByName)(RxPipeline*, const char*, int, int);
extern void                (*RxOpenGLAllInOneSetInstanceCallBack)(RxPipelineNode*, void*);
extern void                (*RxOpenGLAllInOneSetRenderCallBack)(RxPipelineNode*, void*);
extern void                (*_rwOpenGLSetRenderState)(RwRenderState, int);
extern void                (*_rwOpenGLGetRenderState)(RwRenderState, void*);
extern void                (*_rwOpenGLSetRenderStateNoExtras)(RwRenderState, void*);
extern void                (*_rwOpenGLLightsSetMaterialPropertiesORG)(const RpMaterial *mat, RwUInt32 flags);
extern void                (*SetNormalMatrix)(float, float, RwV2d);
extern void                (*DrawStoredMeshData)(RxOpenGLMeshInstanceData*);
extern void                (*ResetEnvMap)();
extern void                (*SetSecondVertexColor)(uint8_t, float);
extern void                (*EnableAlphaModulate)(float);
extern void                (*DisableAlphaModulate)();
extern void                (*SetEnvMap)(void*, float, int);
extern void                (*FileMgrSetDir)(const char* dir);
extern FILE*               (*FileMgrOpenFile)(const char* dir, const char* ioflags);
extern void                (*FileMgrCloseFile)(FILE*);
extern char*               (*FileLoaderLoadLine)(FILE*);
extern unsigned short      (*GetSurfaceIdFromName)(void* trash, const char* surfname);
extern uintptr_t           (*LoadTextureDB)(const char* dbFile, bool fullLoad, int txdbFormat);
extern uintptr_t           (*GetTextureDB)(const char* dbFile);
extern void                (*RegisterTextureDB)(uintptr_t dbPtr);
extern void                (*UnregisterTextureDB)(uintptr_t dbPtr);
extern RwTexture*          (*GetTextureFromTextureDB)(const char* texture);
extern int                 (*AddImageToList)(const char* imgName, bool isPlayerImg);
extern int                 (*SetPlantModelsTab)(unsigned int, RpAtomic**);
extern int                 (*SetCloseFarAlphaDist)(float, float);
extern void                (*FlushTriPlantBuffer)();
extern void                (*DeActivateDirectional)();
extern void                (*SetAmbientColours)(RwRGBAReal*);
extern bool                (*IsSphereVisibleForCamera)(CCamera*, const CVector*, float);
extern void                (*AddTriPlant)(PPTriPlant*, unsigned int);
extern void                (*MoveLocTriToList)(CPlantLocTri*& oldList, CPlantLocTri*& newList, CPlantLocTri* triangle);
extern void                (*StreamingMakeSpaceFor)(int);
extern RwStream*           (*RwStreamOpen)(int, int, const char*);
extern bool                (*RwStreamFindChunk)(RwStream*, int, int, int);
extern RpClump*            (*RpClumpStreamRead)(RwStream*);
extern void                (*RwStreamClose)(RwStream*, int);
extern RpAtomic*           (*GetFirstAtomic)(RpClump*);
extern void                (*SetFilterModeOnAtomicsTextures)(RpAtomic*, int);
extern void                (*RpGeometryLock)(RpGeometry*, int);
extern void                (*RpGeometryUnlock)(RpGeometry*);
extern void                (*RpGeometryForAllMaterials)(RpGeometry*, RpMaterial* (*)(RpMaterial*, void*), void*);
extern void                (*RpMaterialSetTexture)(RpMaterial*, RwTexture*);
extern RpAtomic*           (*RpAtomicClone)(RpAtomic*);
extern void                (*RpClumpDestroy)(RpClump*);
extern RwFrame*            (*RwFrameCreate)();
extern void                (*RpAtomicSetFrame)(RpAtomic*, RwFrame*);
extern void                (*RenderAtomicWithAlpha)(RpAtomic*, int alphaVal);
extern RpGeometry*         (*RpGeometryCreate)(int, int, unsigned int); //
extern RpMaterial*         (*RpGeometryTriangleGetMaterial)(RpGeometry*, RpTriangle*); //
extern void                (*RpGeometryTriangleSetMaterial)(RpGeometry*, RpTriangle*, RpMaterial*); //
extern void                (*RpAtomicSetGeometry)(RpAtomic*, RpGeometry*, unsigned int);
extern RwCamera*           (*CreateShadowCamera)(ShadowCameraStorage*, int size);
extern void                (*DestroyShadowCamera)(CShadowCamera*);
extern void                (*MakeGradientRaster)(ShadowCameraStorage*);
extern void                (*_rwObjectHasFrameSetFrame)(void*, RwFrame*);
extern void                (*RwFrameDestroy)(RwFrame*);
extern void                (*RpLightDestroy)(RpLight*);
extern void                (*CreateRTShadow)(CRealTimeShadow* shadow, int size, bool blur, int blurPasses, bool hasGradient);
extern void                (*SetShadowedObject)(CRealTimeShadow* shadow, CPhysical* physical);
extern int                 (*CamDistComp)(const void*, const void*);
extern bool                (*StoreRealTimeShadow)(CPhysical* physical, float shadowDispX, float shadowDispY, float shadowFrontX, float shadowFrontY, float shadowSideX, float shadowSideY);
extern void                (*UpdateRTShadow)(CRealTimeShadow* shadow);
extern int                 (*GetMobileEffectSetting)();
extern void                (*EntityPreRender)(CEntity* entity);
extern void                (*Radiosity)(int32_t intensityLimit, int32_t filterPasses, int32_t renderPasses, int32_t intensity);
extern void                (*HeatHazeFX)(float, bool);
extern bool                (*IsVisionFXActive)();
extern float               (*TimeCycGetAmbientRed)();
extern float               (*TimeCycGetAmbientGreen)();
extern float               (*TimeCycGetAmbientBlue)();
extern uint32_t            (*GetPipelineID)(RpAtomic*);
extern RwRGBA*             (*GetExtraVertColourPtr)(RpGeometry*);
extern void                (*emu_ArraysDelete)(ArrayState*);
extern void                (*_rxOpenGLAllInOneAtomicInstanceVertexArray)(RxOpenGLMeshInstanceData *, RpAtomic const*, RpGeometry const*, RpGeometryFlag, int, int, unsigned char *, RwRGBA *, RwRGBA *);
extern void                (*emu_ArraysReset)();
extern void                (*emu_ArraysIndices)(void*, unsigned int, unsigned int);
extern void                (*emu_ArraysVertex)(void*, unsigned int, unsigned int, unsigned int);
extern void                (*emu_ArraysVertexAttrib)(unsigned int, int, unsigned int, unsigned char, unsigned int);
extern ArrayState*         (*emu_ArraysStore)(unsigned char, unsigned char);
extern bool                (*rwIsAlphaBlendOn)();
extern ES2Shader*          (*RQCreateShader)(const char* pixel, const char* vertex, uint32_t flags);
extern void                (*OS_ThreadMakeCurrent)();
extern void                (*OS_ThreadUnmakeCurrent)();
extern void                (*SelectEmuShader)(EmuShader*, bool isNewSelection);
extern void                (*RenderCoronas)();
extern void                (*RenderSkyPolys)();
extern void                (*RenderPlants)();
extern void                (*RenderClouds)();
extern void                (*OS_MutexObtain)(OSMutex);
extern void                (*OS_MutexRelease)(OSMutex);
extern void                (*RQ_Process)(RenderQueue*);
extern void                (*RQ_Flush)(RenderQueue*);
extern void                (*RwCameraEndUpdate)(RwCamera*);
extern void                (*RsCameraBeginUpdate)(RwCamera*);
extern void                (*SpeedFX)(float);
extern CVector*            (*FindPlayerSpeed)(int);
extern CPlayerPed*         (*FindPlayerPed)(int);

// Main
void ResolveExternals();
void ForceCustomShader(ES2Shader* shader = NULL);

// Other
static const char* aYesNo[2] = 
{
    "FEM_OFF",
    "FEM_ON",
};
inline int OS_SystemChip() { return *deviceChip; }

#endif // __EXTERNS