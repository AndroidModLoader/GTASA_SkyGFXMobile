#include <mod/amlmod.h>
#include <mod/logger.h>
#include <string.h>
#include <gtasa_things.h>
#include <plantsurfprop.h>

void           (*FileMgrSetDir)(const char* dir);
FILE*          (*FileMgrOpenFile)(const char* dir, const char* ioflags);
void           (*FileMgrCloseFile)(FILE*);
char*          (*FileLoaderLoadLine)(FILE*);
unsigned short (*GetSurfaceIdFromName)(void* trash, const char* surfname);
uintptr_t      (*LoadTextureDB)(const char* dbFile, bool fullLoad, int txdbFormat);
uintptr_t      (*GetTextureDB)(const char* dbFile);
void           (*RegisterTextureDB)(uintptr_t dbPtr);
void           (*UnregisterTextureDB)(uintptr_t dbPtr);
RwTexture*     (*GetTextureFromTextureDB)(const char* texture);
int            (*AddImageToList)(const char* imgName, bool isPlayerImg);
int            (*SetPlantModelsTab)(unsigned int, RpAtomic**);
int            (*SetCloseFarAlphaDist)(float, float);
void           (*FlushTriPlantBuffer)();
void           (*DeActivateDirectional)();
void           (*SetAmbientColours)(RwRGBAReal*);

extern float*  m_fDNBalanceParam;
CPlantSurfProp** m_SurfPropPtrTab;
uint32_t*      m_countSurfPropsAllocated;
CPlantSurfProp m_SurfPropTab[MAX_SURFACE_PROPS];
RwTexture*     tex_gras07Si;
RwTexture**    PC_PlantSlotTextureTab[4];
RwTexture*     PC_PlantTextureTab0[4];
RwTexture*     PC_PlantTextureTab1[4];
RpAtomic**     PC_PlantModelSlotTab[4];
RpAtomic*      PC_PlantModelsTab0[4];
RpAtomic*      PC_PlantModelsTab1[4];
CPool<ColDef>** ms_pColPool;


#define MAXLOCTRIS 4
void (*PlantMgr_rwOpenGLSetRenderState)(RwRenderState, int);
bool (*IsSphereVisibleForCamera)(CCamera*, const CVector*, float);
void (*AddTriPlant)(PPTriPlant*, unsigned int);
void (*MoveLocTriToList)(CPlantLocTri*& oldList, CPlantLocTri*& newList, CPlantLocTri* triangle);
CPlantLocTri** m_CloseLocTriListHead;
CPlantLocTri** m_UnusedLocTriListHead;
CCamera* PlantMgr_TheCamera;

CPlantSurfProp* GetSurfacePtr(unsigned short index)
{
    if(index >= MAX_SURFACES)
    {
        logger->Error("Surface index (%d) is wrong!", index);
        return NULL;
    }
    logger->Info("surf ptr for %d is 0x%X", (int)index, m_SurfPropPtrTab[index]);
    return m_SurfPropPtrTab[index];
}
CPlantSurfProp* AllocSurfacePtr(unsigned short index)
{
    if ( *m_countSurfPropsAllocated < MAX_SURFACE_PROPS )
    {
        m_SurfPropPtrTab[index] = &m_SurfPropTab[*m_countSurfPropsAllocated];
        ++(*m_countSurfPropsAllocated);
        return m_SurfPropPtrTab[index];
    }
    return NULL;
}



void PlantSurfPropMgrLoadPlantsDat(const char* filename)
{
    logger->Info("Parsing plants.dat...");
    FileMgrSetDir("data");
    FILE* f = FileMgrOpenFile(filename, "r");
    FileMgrSetDir("");

    char* i, *token; int line = 0;
    unsigned short SurfaceIdFromName = 0, nPlantCoverDefIndex = 0;
    CPlantSurfProp* SurfacePtr;
    CPlantSurfPropPlantData* ActiveSurface;

    while((i = FileLoaderLoadLine(f)) != NULL)
    {
        ++line;
        if(!strcmp(i, ";the end")) break;
        if(*i == ';') continue; // if ';'

        int proceeded = 0;
        token = strtok(i, " \t");
        while(token != NULL && proceeded < 18)
        {
            switch(proceeded)
            {
                case 0:
                    SurfaceIdFromName = GetSurfaceIdFromName(NULL, token);
                    if(SurfaceIdFromName != 0 && SurfaceIdFromName != MAX_SURFACES) // 0 is "DEFAULT" or unknown. MAX_SURFACES is when token is NULL (idk how actually)
                    {
                        SurfacePtr = GetSurfacePtr(SurfaceIdFromName);
                        if(SurfacePtr != NULL ||
                          (SurfacePtr = AllocSurfacePtr(SurfaceIdFromName)) != NULL) break;
                        logger->Error("SurfacePtr (%s) (%d, line %d) is NULL!", token, SurfaceIdFromName, line);
                    }
                    else
                    {
                        logger->Error("Unknown surface name '%s' in 'plants.dat' (line %d)! See Andrzej to fix this.", token, line);
                        continue; // Just let it pass all surfaces (why do we need to stop it actually?)
                    }
                    goto CloseFile;

                case 1:
                    nPlantCoverDefIndex = atoi(token); // 0 or 1 or 2 (cant be any other)
                    if(nPlantCoverDefIndex >= MAX_SURFACE_DEFINDEXES) nPlantCoverDefIndex = 0;
                    ActiveSurface = &SurfacePtr->m_aSurfaceProperties[nPlantCoverDefIndex];
                    break;

                case 2:
                    SurfacePtr->m_nSlotID = atoi(token);
                    break;

                case 3:
                    ActiveSurface->m_nModelId = atoi(token);
                    break;

                case 4:
                    ActiveSurface->m_nUVOffset = atoi(token);
                    break;

                case 5:
                    ActiveSurface->m_Color.r = atoi(token);
                    break;

                case 6:
                    ActiveSurface->m_Color.g = atoi(token);
                    break;

                case 7:
                    ActiveSurface->m_Color.b = atoi(token);
                    break;

                case 8:
                    ActiveSurface->m_Intensity = atoi(token);
                    break;

                case 9:
                    ActiveSurface->m_IntensityVariation = atoi(token);
                    break;

                case 10:
                    ActiveSurface->m_Color.a = atoi(token);
                    break;

                case 11:
                    ActiveSurface->m_fSizeScaleXY = atof(token);
                    break;

                case 12:
                    ActiveSurface->m_fSizeScaleZ = atof(token);
                    break;

                case 13:
                    ActiveSurface->m_fSizeScaleXYVariation = atof(token);
                    break;

                case 14:
                    ActiveSurface->m_fSizeScaleZVariation = atof(token);
                    break;

                case 15:
                    ActiveSurface->m_fWindBendingScale = atof(token);
                    break;

                case 16:
                    ActiveSurface->m_fWindBendingVariation = atof(token);
                    break;

                case 17:
                    ActiveSurface->m_fDensity = atof(token);
                    break;
            }
            ++proceeded;
            token = strtok(NULL, " \t");
        }
    }
  CloseFile:
    FileMgrCloseFile(f);
}


/* EVERYTHING BELOW IS NOT WORKING! */
/* EVERYTHING BELOW IS NOT WORKING! */
/* EVERYTHING BELOW IS NOT WORKING! */

void (*StreamingMakeSpaceFor)(int);

const char* grassMdls1[GRASS_MODELS_TAB] =
{
    "models\\grass\\grass0_1.dff",
    "models\\grass\\grass0_2.dff",
    "models\\grass\\grass0_3.dff",
    "models\\grass\\grass0_4.dff",
};

const char* grassMdls2[GRASS_MODELS_TAB] =
{
    "models\\grass\\grass1_1.dff",
    "models\\grass\\grass1_2.dff",
    "models\\grass\\grass1_3.dff",
    "models\\grass\\grass1_4.dff",
};

void (*InitColStore)();
RwStream* (*RwStreamOpen)(int, int, const char*);
bool (*RwStreamFindChunk)(RwStream*, int, int, int);
RpClump* (*RpClumpStreamRead)(RwStream*);
void (*RwStreamClose)(RwStreamMemory*, int);
RpAtomic* (*GetFirstAtomic)(RpClump*);
void (*SetFilterModeOnAtomicsTextures)(RpAtomic*, int);
void (*RpGeometryLock)(RpGeometry*, int);
void (*RpGeometryUnlock)(RpGeometry*);
void (*RpGeometryForAllMaterials)(RpGeometry*, RpMaterial* (*)(RpMaterial*, RwRGBA&), RwRGBA&);
void (*RpMaterialSetTexture)(RpMaterial*, RwTexture*);
RpAtomic* (*RpAtomicClone)(RpAtomic*);
void (*RpClumpDestroy)(RpClump*);
RwFrame* (*RwFrameCreate)();
void (*RpAtomicSetFrame)(RpAtomic*, RwFrame*);
void (*RenderAtomicWithAlpha)(RpAtomic*, int alphaVal);
RpGeometry* (*RpGeometryCreate)(int, int, unsigned int); //
RpMaterial* (*RpGeometryTriangleGetMaterial)(RpGeometry*, RpTriangle*); //
void (*RpGeometryTriangleSetMaterial)(RpGeometry*, RpTriangle*, RpMaterial*); //
void (*RpAtomicSetGeometry)(RpAtomic*, RpGeometry*, unsigned int);

RpMaterial* SetGrassMaterial(RpMaterial* material, RwRGBA& rgba)
{
    material->color = rgba;
    RpMaterialSetTexture(material, tex_gras07Si);
    return material;
}

// Reversed!
void AtomicCreatePrelitIfNeeded(RpAtomic* atomic)
{
    RpGeometry* geometry = atomic->geometry;
    if((geometry->flags & rpGEOMETRYPRELIT) != 0) return;
    RwInt32 numTriangles = geometry->numTriangles;
    
    RpGeometry* newometry = RpGeometryCreate(geometry->numVertices, geometry->numTriangles, rpGEOMETRYPRELIT | rpGEOMETRYTEXTURED | rpGEOMETRYTRISTRIP);
    memcpy(newometry->morphTarget->verts, geometry->morphTarget->verts, sizeof(RwV3d) * geometry->numVertices);
    memcpy(newometry->texCoords[0], geometry->texCoords[0], sizeof(RwTexCoords) * geometry->numVertices);
    memcpy(newometry->triangles, geometry->triangles, sizeof(RpTriangle) * numTriangles);
    
    for(int i = 0; i < numTriangles; ++i)
    {
        RpGeometryTriangleSetMaterial(newometry, &newometry->triangles[i], 
                                      RpGeometryTriangleGetMaterial(geometry, &geometry->triangles[i]));
    }
    RpGeometryUnlock(newometry);
    newometry->flags |= rpGEOMETRYPOSITIONS; // why?
    RpAtomicSetGeometry(atomic, newometry, 1);
}

// Reversed!
bool GeometrySetPrelitConstantColor(RpGeometry* geometry, CRGBA clr)
{
    if((geometry->flags & rpGEOMETRYPRELIT) == 0) return false;
    RpGeometryLock(geometry, 4095);
    
    RwRGBA* prelitClrPtr = geometry->preLitLum;
    if(prelitClrPtr)
    {
        RwInt32 numPrelit = geometry->numVertices;
        if(numPrelit)
        {
            memset(prelitClrPtr, clr.val, numPrelit * sizeof(RwRGBA));
        }
    }
    
    RpGeometryUnlock(geometry);
    return true;
}

float GetPlantDensity(CPlantLocTri* plant)
{
    // The magnitude of the cross product of 2 vectors give the area of a paralellogram they span.
    return (plant->m_V2 - plant->m_V1).Cross(plant->m_V3 - plant->m_V1).Magnitude() / 2.f;
}

bool LoadGrassModels(const char** grassModelsNames, RpAtomic** ret)
{
    for(int i = 0; i < GRASS_MODELS_TAB; ++i)
    {
        RpClump* clump = NULL;
        RwStream* stream = RwStreamOpen(2, 1, grassModelsNames[i]);
        if(stream && RwStreamFindChunk(stream, 16, 0, 0)) clump = RpClumpStreamRead(stream);
        RwStreamClose((RwStreamMemory*)stream, 0);
        RpAtomic* FirstAtomic = GetFirstAtomic(clump);
        SetFilterModeOnAtomicsTextures(FirstAtomic, 4);
        
        AtomicCreatePrelitIfNeeded(FirstAtomic);
        RpGeometry* geometry = FirstAtomic->geometry;
        RpGeometryLock(geometry, 4095);
        geometry->flags = (geometry->flags & 0xFFFFFF8F) | 0x40;
        RpGeometryUnlock(geometry);
        
        GeometrySetPrelitConstantColor(geometry, rgbaWhite);
        
        RwRGBA defClr {0, 0, 0, 50};
        RpGeometryForAllMaterials(geometry, SetGrassMaterial, defClr);
        RpAtomic* CloneAtomic = RpAtomicClone(FirstAtomic);
        ret[i] = CloneAtomic;
        RpClumpDestroy(clump);
        SetFilterModeOnAtomicsTextures(CloneAtomic, 2);
        RpAtomicSetFrame(CloneAtomic, RwFrameCreate());
    }
    return true; // idk
}

void InitPlantManager()
{
    //uintptr_t tdb = LoadTextureDB("envgrass", false, DF_Default);
    //if(tdb)
    //{
    //    logger->Info("Successfully loaded ENVGRASS.TEXDB (ptr:0x%X)", tdb);
    //    RegisterTextureDB(tdb);
    //}
    //else
    //{
    //    logger->Error("Failed to load ENVGRASS.TEXDB!");
    //}

    //StreamingMakeSpaceFor(0x8800); // Does nothing

    PC_PlantTextureTab0[0] = GetTextureFromTextureDB("txgrass0_0");
    PC_PlantTextureTab0[1] = GetTextureFromTextureDB("txgrass0_1");
    PC_PlantTextureTab0[2] = GetTextureFromTextureDB("txgrass0_2");
    PC_PlantTextureTab0[3] = GetTextureFromTextureDB("txgrass0_3");

    PC_PlantTextureTab1[0] = GetTextureFromTextureDB("txgrass1_0");
    PC_PlantTextureTab1[1] = GetTextureFromTextureDB("txgrass1_1");
    PC_PlantTextureTab1[2] = GetTextureFromTextureDB("txgrass1_2");
    PC_PlantTextureTab1[3] = GetTextureFromTextureDB("txgrass1_3");
    
    tex_gras07Si = GetTextureFromTextureDB("gras07Si");

    PC_PlantSlotTextureTab[0] = PC_PlantTextureTab0;
    PC_PlantSlotTextureTab[1] = PC_PlantTextureTab1;
    PC_PlantSlotTextureTab[2] = PC_PlantTextureTab0;
    PC_PlantSlotTextureTab[3] = PC_PlantTextureTab1;

    if(LoadGrassModels(grassMdls1, PC_PlantModelsTab0) && LoadGrassModels(grassMdls2, PC_PlantModelsTab1))
    {
        PC_PlantModelSlotTab[0] = PC_PlantModelsTab0;
        PC_PlantModelSlotTab[1] = PC_PlantModelsTab1;
        PC_PlantModelSlotTab[2] = PC_PlantModelsTab0;
        PC_PlantModelSlotTab[3] = PC_PlantModelsTab1;

        SetPlantModelsTab(0, PC_PlantModelsTab0);
        SetPlantModelsTab(1, PC_PlantModelSlotTab[0]);
        SetPlantModelsTab(2, PC_PlantModelSlotTab[0]);
        SetPlantModelsTab(3, PC_PlantModelSlotTab[0]);
        SetCloseFarAlphaDist(3.0f, 60.0f);
    }

    //UnregisterTextureDB(tdb);
}

DECL_HOOKv(PlantSurfPropMgrInit)
{
    PlantSurfPropMgrInit();
    PlantSurfPropMgrLoadPlantsDat("plants.dat");
}
DECL_HOOKv(PlantMgrInit)
{
    PlantMgrInit(); // Acts like a CPlantMgr::ReloadConfig()
    InitPlantManager();
}

DECL_HOOKv(PlantMgrRender)
{
    PPTriPlant plant;
    
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEZWRITEENABLE, 0);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEZTESTENABLE, 1u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEVERTEXALPHAENABLE, 1u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATESRCBLEND, 5u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEDESTBLEND, 6u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEFOGENABLE, 1u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, 8u);

    DeActivateDirectional();

    for(int type = 0; type < MAXLOCTRIS; ++type)
    {
        CPlantLocTri* plantTris = m_CloseLocTriListHead[type];
        RwTexture** grassTex = PC_PlantSlotTextureTab[type];

        while(plantTris != NULL)
        {
            CPlantSurfProp* surface = GetSurfacePtr(plantTris->m_nSurfaceType);
            if(surface && IsSphereVisibleForCamera(PlantMgr_TheCamera, &plantTris->m_Center, plantTris->m_SphereRadius))
            {
                CPlantSurfPropPlantData& surfProp = surface->m_aSurfaceProperties[0];
                //if(surfProp.m_nModelId != -1)
                {
                    plant.V1 = plantTris->m_V1;
                    plant.V2 = plantTris->m_V2;
                    plant.V3 = plantTris->m_V3;
                    plant.center = plantTris->m_Center;
                    plant.model_id = surfProp.m_nModelId;
                    plant.num_plants = ((plantTris->m_nMaxNumPlants[0] + 8) & 0xFFF8);
                    plant.scale.x = surfProp.m_fSizeScaleXY;
                    plant.scale.y = surfProp.m_fSizeScaleZ;
                    plant.texture_ptr = grassTex[surfProp.m_nUVOffset];
                    plant.color = *(RwRGBA*)&surfProp.m_Color;
                    plant.intensity = surfProp.m_Intensity;
                    plant.intensity_var = surfProp.m_IntensityVariation;
                    plant.seed = plantTris->m_Seed[0];
                    plant.scale_var_xy = surfProp.m_fSizeScaleXYVariation;
                    plant.scale_var_z = surfProp.m_fSizeScaleZVariation;
                    plant.wind_bend_scale = surfProp.m_fWindBendingScale;
                    plant.wind_bend_var = surfProp.m_fWindBendingVariation;
    
                    float intens = (((plantTris->m_ColLighting.night * (*m_fDNBalanceParam)) + (plantTris->m_ColLighting.day * (1.0f - *m_fDNBalanceParam))) / 30.0f) / 256.0f;
                    plant.color.red *= intens;
                    plant.color.green *= intens;
                    plant.color.blue *= intens;
                    
                    AddTriPlant(&plant, type);

                    logger->Info("Tex for grass is 0x%X (raster 0x%X)", plant.texture_ptr, plant.texture_ptr->raster);
                }
            }
            
            plantTris = plantTris->m_pNextTri;
        }
        
        FlushTriPlantBuffer();
    }

    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEZWRITEENABLE, 1u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEZTESTENABLE, 1u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, 7u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);
}

DECL_HOOK(CPlantLocTri*, PlantLocTriAdd, CPlantLocTri* self, CVector& p1, CVector& p2, CVector& p3, uint8_t surface, tColLighting lightning, bool createsPlants, bool createsObjects)
{
    self->m_V1 = p1;
    self->m_V2 = p2;
    self->m_V3 = p3;
    self->m_nLighting = lightning.as_uint8;
    self->m_nSurfaceType = surface;
    self->m_createsObjects = createsObjects;
    self->m_createsPlants = createsPlants;
    self->m_createdObjects = false;
    self->m_Center = (p1 + p2 + p3) / 3.0f;
    
    self->m_SphereRadius = DistanceBetweenPoints(self->m_Center, self->m_V1) * 1.75f;
    if (self->m_createsObjects && !self->m_createsPlants)
    {
        MoveLocTriToList(*m_UnusedLocTriListHead, m_CloseLocTriListHead[3], self);
        return NULL;
    }

    CPlantSurfProp* surfPtr = GetSurfacePtr(surface);
    auto densityMult = GetPlantDensity(self);
    auto density1 = (uint16_t)(densityMult * surfPtr->m_aSurfaceProperties[0].m_fDensity);
    auto density2 = (uint16_t)(densityMult * surfPtr->m_aSurfaceProperties[1].m_fDensity);
    auto density3 = (uint16_t)(densityMult * surfPtr->m_aSurfaceProperties[2].m_fDensity);
    if (density1 + density2 + density3 > 0)
    {
        self->m_Seed[0] = (float)rand() / (float)RAND_MAX;
        self->m_Seed[1] = (float)rand() / (float)RAND_MAX;
        self->m_Seed[2] = (float)rand() / (float)RAND_MAX;
        self->m_nMaxNumPlants[0] = density1;
        self->m_nMaxNumPlants[1] = density2;
        self->m_nMaxNumPlants[2] = density3;
        MoveLocTriToList(*m_UnusedLocTriListHead, m_CloseLocTriListHead[surfPtr->m_nSlotID], self);
        return self;
    }

    if (self->m_createsObjects)
    {
        self->m_createsObjects = false;

        MoveLocTriToList(*m_UnusedLocTriListHead, m_CloseLocTriListHead[3], self);
        return self;
    }
    
    return NULL;
}
