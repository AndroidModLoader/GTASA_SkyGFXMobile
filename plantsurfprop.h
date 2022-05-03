#include <stdio.h>

#define GRASS_MODELS_TAB 4

extern void           (*FileMgrSetDir)(const char* dir);
extern FILE*          (*FileMgrOpenFile)(const char* dir, const char* ioflags);
extern void           (*FileMgrCloseFile)(FILE*);
extern char*          (*FileLoaderLoadLine)(FILE*);
extern unsigned short (*GetSurfaceIdFromName)(void*, const char* surfname);
extern uintptr_t      (*LoadTextureDB)(const char* dbFile, bool fullLoad, int txdbFormat);
extern void           (*RegisterTextureDB)(uintptr_t dbPtr);
extern void           (*UnregisterTextureDB)(uintptr_t dbPtr);
extern RwTexture*     (*GetTextureFromTextureDB)(const char* texture);
extern int            (*AddImageToList)(const char* imgName, bool isPlayerImg);
extern int            (*SetPlantModelsTab)(unsigned int, RpAtomic**);
extern int            (*SetCloseFarAlphaDist)(float, float);

extern Surface**      m_SurfPropPtrTab;
extern int*           m_countSurfPropsAllocated;
extern Surface        m_SurfPropTab[MAX_SURFACE_PROPS];
extern RwTexture**    PC_PlantSlotTextureTab[4];
extern RwTexture*     PC_PlantTextureTab0[4];
extern RwTexture*     PC_PlantTextureTab1[4];
extern RpAtomic**     PC_PlantModelSlotTab[4];
extern RpAtomic*      PC_PlantModelsTab0[4];
extern RpAtomic*      PC_PlantModelsTab1[4];

// CPlantMgr:
extern void (*StreamingMakeSpaceFor)(int);


extern RwStream* (*RwStreamOpen)(int, int, const char*);
extern bool (*RwStreamFindChunk)(RwStream*, int, int, int);
extern RpClump* (*RpClumpStreamRead)(RwStream*);
extern void (*RwStreamClose)(RwStreamMemory*, int);
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

extern void (*PlantMgr_rwOpenGLSetRenderState)(RwRenderState, int);
extern bool (*IsSphereVisibleForCamera)(CCamera*, const CVector*, float);
extern void (*AddTriPlant)(PPTriPlant*, unsigned int);
extern CCamera* PlantMgr_TheCamera;