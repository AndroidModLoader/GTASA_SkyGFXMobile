#include <stdio.h>

#define GRASS_MODELS_TAB 4

extern void           (*FileMgrSetDir)(const char* dir);
extern FILE*          (*FileMgrOpenFile)(const char* dir, const char* ioflags);
extern void           (*FileMgrCloseFile)(FILE*);
extern char*          (*FileLoaderLoadLine)(FILE*);
extern unsigned short (*GetSurfaceIdFromName)(void*, const char* surfname);
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

extern CPlantSurfProp** m_SurfPropPtrTab;
extern uint32_t*      m_countSurfPropsAllocated;
extern CPlantSurfProp m_SurfPropTab[MAX_SURFACE_PROPS];
extern RwTexture**    PC_PlantSlotTextureTab[4];
extern RwTexture*     PC_PlantTextureTab0[4];
extern RwTexture*     PC_PlantTextureTab1[4];
extern RpAtomic**     PC_PlantModelSlotTab[4];
extern RpAtomic*      PC_PlantModelsTab0[4];
extern RpAtomic*      PC_PlantModelsTab1[4];
extern CPool<ColDef>** ms_pColPool;

// CPlantMgr:
extern void (*StreamingMakeSpaceFor)(int);


extern void (*InitColStore)();
extern RwStream* (*RwStreamOpen)(int, int, const char*);
extern bool (*RwStreamFindChunk)(RwStream*, int, int, int);
extern RpClump* (*RpClumpStreamRead)(RwStream*);
extern void (*RwStreamClose)(RwStreamMemory*, int);
extern RpAtomic* (*GetFirstAtomic)(RpClump*);
extern void (*SetFilterModeOnAtomicsTextures)(RpAtomic*, int);
extern void (*RpGeometryLock)(RpGeometry*, int);
extern void (*RpGeometryUnlock)(RpGeometry*);
extern void (*RpGeometryForAllMaterials)(RpGeometry*, RpMaterial* (*)(RpMaterial*, RwRGBA&), RwRGBA&);
extern void (*RpMaterialSetTexture)(RpMaterial*, RwTexture*);
extern RpAtomic* (*RpAtomicClone)(RpAtomic*);
extern void (*RpClumpDestroy)(RpClump*);
extern RwFrame* (*RwFrameCreate)();
extern void (*RpAtomicSetFrame)(RpAtomic*, RwFrame*);
extern void (*RenderAtomicWithAlpha)(RpAtomic*, int alphaVal);
extern RpGeometry* (*RpGeometryCreate)(int, int, unsigned int);
extern RpMaterial* (*RpGeometryTriangleGetMaterial)(RpGeometry*, RpTriangle*);
extern void (*RpGeometryTriangleSetMaterial)(RpGeometry*, RpTriangle*, RpMaterial*);
extern void (*RpAtomicSetGeometry)(RpAtomic*, RpGeometry*, unsigned int);

extern void (*PlantMgr_rwOpenGLSetRenderState)(RwRenderState, int);
extern bool (*IsSphereVisibleForCamera)(CCamera*, const CVector*, float);
extern void (*AddTriPlant)(PPTriPlant*, unsigned int);
extern void (*MoveLocTriToList)(CPlantLocTri*& oldList, CPlantLocTri*& newList, CPlantLocTri* triangle);
extern CCamera* PlantMgr_TheCamera;
extern CPlantLocTri** m_CloseLocTriListHead;
extern CPlantLocTri** m_UnusedLocTriListHead;
