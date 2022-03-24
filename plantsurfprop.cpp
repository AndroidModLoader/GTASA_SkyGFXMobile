#include <mod/logger.h>

#include <string.h>
#include <gtasa_things.h>
#include "GTASA_STRUCTS.h"
#include <plantsurfprop.h>

void           (*FileMgrSetDir)(const char* dir);
FILE*          (*FileMgrOpenFile)(const char* dir, const char* ioflags);
void           (*FileMgrCloseFile)(FILE*);
char*          (*FileLoaderLoadLine)(FILE*);
unsigned short (*GetSurfaceIdFromName)(void* trash, const char* surfname);
uintptr_t      (*LoadTextureDB)(const char* dbFile, bool fullLoad, int txdbFormat);
void           (*RegisterTextureDB)(uintptr_t dbPtr);
RwTexture*     (*GetTextureFromTextureDB)(const char* texture);
int            (*AddImageToList)(const char* imgName, bool isPlayerImg);

Surface**  m_SurfPropPtrTab;
int*       m_countSurfPropsAllocated;
Surface    m_SurfPropTab[MAX_SURFACE_PROPS];
RwTexture* PC_PlantTextureTab0[4];
RwTexture* PC_PlantTextureTab1[4];

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
                        //if(SurfacePtr != NULL || *m_countSurfPropsAllocated < MAX_SURFACE_PROPS && (TempSurface = &m_SurfPropTab[*m_countSurfPropsAllocated],
                        //        ++*m_countSurfPropsAllocated, m_SurfPropPtrTab[SurfaceIdFromName] = TempSurface, (SurfacePtr = TempSurface) != 0)) break;
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

void InitPlantManager()
{
    logger->Info("InitPlantManager");

    AddImageToList("TEXDB\\ENVGRASS.IMG", false);
    uintptr_t tdb = LoadTextureDB("ENVGRASS", false, 5);
    if(tdb) RegisterTextureDB(tdb);

    StreamingMakeSpaceFor(0x8800);

    PC_PlantTextureTab0[0] = GetTextureFromTextureDB("txgrass0_0");
    PC_PlantTextureTab0[1] = GetTextureFromTextureDB("txgrass0_1");
    PC_PlantTextureTab0[2] = GetTextureFromTextureDB("txgrass0_2");
    PC_PlantTextureTab0[3] = GetTextureFromTextureDB("txgrass0_3");

    PC_PlantTextureTab1[0] = GetTextureFromTextureDB("txgrass1_0");
    PC_PlantTextureTab1[1] = GetTextureFromTextureDB("txgrass1_1");
    PC_PlantTextureTab1[2] = GetTextureFromTextureDB("txgrass1_2");
    PC_PlantTextureTab1[3] = GetTextureFromTextureDB("txgrass1_3");


}