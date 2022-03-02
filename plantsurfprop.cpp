#include <mod/logger.h>

#include <string.h>
#include <gtasa_things.h>
#include "GTASA_STRUCTS.h"
#include <plantsurfprop.h>

void (*FileMgrSetDir)(const char* dir);
FILE* (*FileMgrOpenFile)(const char* dir, const char* ioflags);
void (*FileMgrCloseFile)(FILE*);
char* (*FileLoaderLoadLine)(FILE*);
unsigned short (*GetSurfaceIdFromName)(void* trash, const char* surfname);
Surface** m_SurfPropPtrTab;
int* m_countSurfPropsAllocated;
uintptr_t m_SurfPropTab;

Surface* GetSurfacePtr(unsigned short index)
{
    if(index < 0 && index >= 178)
    {
        logger->Error("Surface index (%d) is too high!", index);
        return NULL;
    }
    //return *(Surface**)((uintptr_t)m_SurfPropPtrTab + 4 * index);
    return m_SurfPropPtrTab[index];
}

void PlantSurfPropMgrLoadPlantsDat(const char* filename)
{
    //logger->Info("CPlantSurfPropMgr::LoadPlantsDat");
    FileMgrSetDir("data");
    FILE* f = FileMgrOpenFile(filename, "r");
    FileMgrSetDir("");

    char* i, *token; int line = 0;
    unsigned short SurfaceIdFromName = 0, unknownVar10 = 0;
    Surface* SurfacePtr, *ActiveSurface, *TempSurface;
    while((i = FileLoaderLoadLine(f)) != NULL)
    {
        ++line;
        if(!strcmp(i, ";the end")) break;
        if(*i == 59) continue;

        int proceeded = 0;
        token = strtok(i, " \t");
        while(token != NULL && proceeded < 18)
        {
            switch(proceeded)
            {
                case 0:
                {
                    SurfaceIdFromName = GetSurfaceIdFromName((void*)token, token);
                    if(SurfaceIdFromName != 0)
                    {
                        SurfacePtr = GetSurfacePtr(SurfaceIdFromName);
                        if(SurfacePtr != NULL || *m_countSurfPropsAllocated < 0x39 && (TempSurface = (Surface*)(m_SurfPropTab + 124 * *m_countSurfPropsAllocated),
                                ++*m_countSurfPropsAllocated, m_SurfPropPtrTab[SurfaceIdFromName] = TempSurface, (SurfacePtr = TempSurface) != 0))
                        {
                            //logger->Info("Loaded surface %s (%d) (0x%X)", token, SurfaceIdFromName, SurfacePtr);
                            break;
                        }
                        logger->Error("SurfacePtr (%s) (%d, line %d) is NULL!", token, SurfaceIdFromName, line);
                    }
                    else
                    {
                        logger->Error("Unknown surface name '%s' in 'plants.dat' (line %d)! See Andrzej to fix this.", token, line);
                    }
                    goto CloseFile;
                }

                case 1:
                {
                    unknownVar10 = atoi(token);
                    if(unknownVar10 > 2) unknownVar10 = 0;
                    ActiveSurface = &SurfacePtr[20 * unknownVar10 + 2];
                    break;
                }

                case 2:
                {
                    SurfacePtr->pad0 = atoi(token);
                    break;
                }

                case 3:
                {
                    *(short*)((int)ActiveSurface + 0) = atoi(token);
                    break;
                }

                case 4:
                {
                    *(short*)((int)ActiveSurface + 2) = atoi(token);
                    break;
                }

                case 5:
                {
                    *(char*)((int)ActiveSurface + 4) = atoi(token);
                    break;
                }

                case 6:
                {
                    *(char*)((int)ActiveSurface + 5) = atoi(token);
                    break;
                }

                case 7:
                {
                    *(char*)((int)ActiveSurface + 6) = atoi(token);
                    break;
                }

                case 8:
                {
                    *(char*)((int)ActiveSurface + 8) = atoi(token);
                    break;
                }

                case 9:
                {
                    *(char*)((int)ActiveSurface + 9) = atoi(token);
                    break;
                }

                case 10:
                {
                    *(char*)((int)ActiveSurface + 7) = atoi(token);
                    break;
                }

                case 11:
                {
                    *(float*)((int)ActiveSurface + 12) = atof(token);
                    break;
                }

                case 12:
                {
                    *(float*)((int)ActiveSurface + 16) = atof(token);
                    break;
                }

                case 13:
                {
                    *(float*)((int)ActiveSurface + 20) = atof(token);
                    break;
                }

                case 14:
                {
                    *(float*)((int)ActiveSurface + 24) = atof(token);
                    break;
                }

                case 15:
                {
                    *(float*)((int)ActiveSurface + 32) = atof(token);
                    break;
                }

                case 16:
                {
                    *(float*)((int)ActiveSurface + 36) = atof(token);
                    break;
                }

                case 17:
                {
                    *(float*)((int)ActiveSurface + 28) = atof(token);
                    break;
                }
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


CPool** pTxdPool; // _ZN9CTxdStore11ms_pTxdPoolE

void (*StreamingMakeSpaceFor)(int);
void (*ImGonnaUseStreamingMemory)();
int (*RwTexDictionaryGetCurrent)();
int (*FindTxdSlot)(const char*);
int (*AddTxdSlot)(const char*);
void (*TxdAddRef)(int);
void (*SetCurrentTxd)(int);
int* TexDictionaryLinkPluginOff; // A83F5C

RwStream* (*RwStreamOpen)(int type, int accessType, int data);
void (*RwStreamClose)(RwStream* stream, void* unk);
bool (*RwStreamFindChunk)(RwStream* stream, unsigned int, unsigned int, unsigned int);
int (*RwTexDictionaryGtaStreamRead)(RwStream* stream);

void txdInitParent(int index)
{
    unsigned char* m_byteMap = (*pTxdPool)->m_byteMap;
    char* v2;
    short v3;
    int* v4;
    int v5;

    if (!(m_byteMap[index] & 0x80u)) v2 = (char *)(*pTxdPool)->m_pObjects + 12 * index;
    else v2 = 0;
    v3 = *((short*)v2 + 3);

    if (v3 != -1)
    {
        if(!(m_byteMap[v3] & 0x80u)) v4 = (int*)((char*)(*pTxdPool)->m_pObjects + 12 * v3);
        else v4 = 0;
        *(int*)(*TexDictionaryLinkPluginOff + *(int*)v2) = *v4;
        v5 = *((short*)v2 + 3);
        if(!((*pTxdPool)->m_byteMap[v5] & 0x80u)) ++*((short*)(*pTxdPool)->m_pObjects + 6 * v5 + 2);
    }
}

int TxdStoreLoadTxd(int index, RwStream *stream)
{
    int* v2 = (int*)((char *)(*pTxdPool)->m_pObjects + 12 * index);
    if (RwStreamFindChunk(stream, 22, 0, 0))
    {
        int v4 = RwTexDictionaryGtaStreamRead(stream);
        *v2 = v4;
        if (v4) txdInitParent(index);
        return *v2 != 0;
    }
    return false;
}

int LoadTxd(int txdSlot, const char* filename)
{
    char path[256];
    snprintf(path, sizeof(path), "%s", filename);
    RwStream* stream;
    while((stream = RwStreamOpen(2, 1, (int)path)) == NULL);
    int Txd = TxdStoreLoadTxd(txdSlot, stream);
    RwStreamClose(stream, NULL);
    return Txd;
}

void InitPlantManager()
{
    logger->Info("InitPlantManager");

    StreamingMakeSpaceFor(0x8800);
    ImGonnaUseStreamingMemory();
    RwTexDictionaryGetCurrent();
    int TxdSlot = FindTxdSlot("grass_pc");
    if(TxdSlot == -1) TxdSlot = AddTxdSlot("grass_pc");
    LoadTxd(TxdSlot, "models/grass/plant1.txd");
    TxdAddRef(TxdSlot);
    SetCurrentTxd(TxdSlot);

    
}