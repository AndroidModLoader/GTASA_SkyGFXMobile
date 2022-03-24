#include <stdio.h>

extern void           (*FileMgrSetDir)(const char* dir);
extern FILE*          (*FileMgrOpenFile)(const char* dir, const char* ioflags);
extern void           (*FileMgrCloseFile)(FILE*);
extern char*          (*FileLoaderLoadLine)(FILE*);
extern unsigned short (*GetSurfaceIdFromName)(void*, const char* surfname);
extern uintptr_t      (*LoadTextureDB)(const char* dbFile, bool fullLoad, int txdbFormat);
extern void           (*RegisterTextureDB)(uintptr_t dbPtr);
extern RwTexture*     (*GetTextureFromTextureDB)(const char* texture);
extern int            (*AddImageToList)(const char* imgName, bool isPlayerImg);

extern Surface**      m_SurfPropPtrTab;
extern int*           m_countSurfPropsAllocated;
extern Surface        m_SurfPropTab[MAX_SURFACE_PROPS];
extern RwTexture*     PC_PlantTextureTab0[4];
extern RwTexture*     PC_PlantTextureTab1[4];

// CPlantMgr:
extern void (*StreamingMakeSpaceFor)(int);