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
void           (*RegisterTextureDB)(uintptr_t dbPtr);
void           (*UnregisterTextureDB)(uintptr_t dbPtr);
RwTexture*     (*GetTextureFromTextureDB)(const char* texture);
int            (*AddImageToList)(const char* imgName, bool isPlayerImg);
int            (*SetPlantModelsTab)(unsigned int, RpAtomic**);
int            (*SetCloseFarAlphaDist)(float, float);

Surface**      m_SurfPropPtrTab;
int*           m_countSurfPropsAllocated;
Surface        m_SurfPropTab[MAX_SURFACE_PROPS];
RwTexture*     tex_gras07Si;
RwTexture**    PC_PlantSlotTextureTab[4];
RwTexture*     PC_PlantTextureTab0[4];
RwTexture*     PC_PlantTextureTab1[4];
RpAtomic**     PC_PlantModelSlotTab[4];
RpAtomic*      PC_PlantModelsTab0[4];
RpAtomic*      PC_PlantModelsTab1[4];

Surface* GetSurfacePtr(unsigned short index)
{
    if(index < 0 && index >= MAX_SURFACES)
    {
        logger->Error("Surface index (%d) is wrong!", index);
        return NULL;
    }
    return m_SurfPropPtrTab[index];
}

void PlantSurfPropMgrLoadPlantsDat(const char* filename)
{
    logger->Info("Parsing plants.dat...");
    FileMgrSetDir("data");
    FILE* f = FileMgrOpenFile(filename, "r");
    FileMgrSetDir("");

    char* i, *token; int line = 0;
    unsigned short SurfaceIdFromName = 0, nPlantCoverDefIndex = 0;
    Surface* SurfacePtr;
    SurfaceProperty* ActiveSurface;

    while((i = FileLoaderLoadLine(f)) != NULL)
    {
        ++line;
        if(!strcmp(i, ";the end")) break;
        if(*i == 59) continue; // if ';'

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
                        if(SurfacePtr != NULL) break;
                        if(*m_countSurfPropsAllocated < MAX_SURFACE_PROPS)
                        {
                            SurfacePtr = &m_SurfPropTab[*m_countSurfPropsAllocated];
                            ++*m_countSurfPropsAllocated;
                            m_SurfPropPtrTab[SurfaceIdFromName] = SurfacePtr;
                            break;
                        }
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
                    if(nPlantCoverDefIndex >= MAX_SURFACE_DEFINDEXES || nPlantCoverDefIndex < 0) nPlantCoverDefIndex = 0;
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

RwStream* (*RwStreamOpen)(int, int, const char*);
bool (*RwStreamFindChunk)(RwStream*, int, int, int);
RpClump* (*RpClumpStreamRead)(RwStream*);
void (*RwStreamClose)(RwStreamMemory*, int);
RpAtomic* (*GetFirstAtomic)(RpClump*);
void (*SetFilterModeOnAtomicsTextures)(RpAtomic*, int);
void (*RpGeometryLock)(RpGeometry*, int);
void (*RpGeometryUnlock)(RpGeometry*);
void (*RpGeometryForAllMaterials)(RpGeometry*, RpMaterial* (*)(RpMaterial*, void*), void*);
void (*RpMaterialSetTexture)(RpMaterial*, RwTexture*);
RpAtomic* (*RpAtomicClone)(RpAtomic*);
void (*RpClumpDestroy)(RpClump*);
RwFrame* (*RwFrameCreate)();
void (*RpAtomicSetFrame)(RpAtomic*, RwFrame*);
void (*RenderAtomicWithAlpha)(RpAtomic*, int alphaVal);
RpMaterial* SetGrassMaterial(RpMaterial* material, void* rgba)
{
    material->color = *(RwRGBA*)rgba;
    RpMaterialSetTexture(material, tex_gras07Si);
    return material;
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
        //AtomicCreatePrelitIfNeeded(FirstAtomic);
        RpGeometry* geometry = FirstAtomic->geometry;
        RpGeometryLock(geometry, 4095);
        geometry->flags &= 0xFFFFFF8F | 0x40;
        RpGeometryUnlock(geometry);
        //v9 = (_DWORD *)CRGBA::CRGBA(&v17, 0xFFu, 0xFFu, 0xFFu, 0xFFu);
        //GeometrySetPrelitConstantColor(*v9);
        int pData = 0x32000000;
        RpGeometryForAllMaterials(geometry, SetGrassMaterial, &pData);
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
    logger->Info("Initializing Plant-Manager...");

    //int loadedImageId = AddImageToList("TEXDB\\ENVGRASS.IMG", false);
    //if(!loadedImageId)
    //{
    //    logger->Error("Failed to load ENVGRASS.IMG!");
    //}
    //else
    //{
    //    logger->Info("Successfully loaded ENVGRASS.IMG (id:%d)", loadedImageId);
    //}
    uintptr_t tdb = LoadTextureDB("envgrass", false, 5);
    if(tdb)
    {
        logger->Info("Successfully loaded ENVGRASS.TEXDB (ptr:0x%X)", tdb);
        RegisterTextureDB(tdb);
    }
    else
    {
        logger->Error("Failed to load ENVGRASS.TEXDB!");
    }

    StreamingMakeSpaceFor(0x8800); // Does nothing

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

    UnregisterTextureDB(tdb);
    logger->Info("Plant-Manager initialized!");
}

DECL_HOOKv(PlantSurfPropMgrInit)
{
    PlantSurfPropMgrInit();
    PlantSurfPropMgrLoadPlantsDat("plants.dat");
}
DECL_HOOKv(PlantMgrInit)
{
    logger->Info("CPlantMgr::ReloadConfig()");
    PlantMgrInit(); // Acts like a CPlantMgr::ReloadConfig()
    logger->Info("CPlantMgr::Initialise()");
    InitPlantManager();
}

#define DDDAFSF 4
void (*PlantMgr_rwOpenGLSetRenderState)(RwRenderState, int);
bool (*IsSphereVisibleForCamera)(CCamera*, const CVector*, float);
void (*AddTriPlant)(PPTriPlant*, unsigned int);
CPlantLocTri* m_CloseLocTriListHead[DDDAFSF];
CCamera* PlantMgr_TheCamera;

DECL_HOOKv(PlantMgrRender)
{
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEZWRITEENABLE, 0);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEZTESTENABLE, 1u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEVERTEXALPHAENABLE, 1u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATESRCBLEND, 5u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEDESTBLEND, 6u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEFOGENABLE, 1u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, 8u);

    RenderAtomicWithAlpha(PC_PlantModelsTab0[0], 128);

    for(int type = 0; type < DDDAFSF; ++type)
    {
        static PPTriPlant plant = {0}; 
        CPlantLocTri* plantTris = m_CloseLocTriListHead[type];
        RwTexture** grassTex = PC_PlantSlotTextureTab[type];
        logger->Info("Type: %d, PlantTris: 0x%X, grassTex: 0x%X", (int)type, plantTris, grassTex);

        plant.pos.x = 20.0f;
        plant.pos.y = 20.0f;
        plant.pos.z = 20.0f;
        plant.texture = grassTex[0];
        AddTriPlant(&plant, type);

        while(plantTris != NULL)
        {
            Surface* surface = GetSurfacePtr(plantTris->surfaceId);
            if(IsSphereVisibleForCamera(PlantMgr_TheCamera, &plantTris->pos, plantTris->radius))
            {
                SurfaceProperty& surfProp = surface->m_aSurfaceProperties[0];
                if(surfProp.m_nModelId != -1)
                {
                    
                }
            }
            plantTris = plantTris->nextPlant;
        }
    }

    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEZWRITEENABLE, 1u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEZTESTENABLE, 1u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTION, 7u);
    PlantMgr_rwOpenGLSetRenderState(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);
}