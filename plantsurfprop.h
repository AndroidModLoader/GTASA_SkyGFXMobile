#include <stdio.h>

struct Surface
{
    short pad0;
};

struct RwStream
{

};

extern void (*FileMgrSetDir)(const char* dir);
extern FILE* (*FileMgrOpenFile)(const char* dir, const char* ioflags);
extern void (*FileMgrCloseFile)(FILE*);
extern char* (*FileLoaderLoadLine)(FILE*);
extern unsigned short (*GetSurfaceIdFromName)(void*, const char* surfname);
extern Surface** m_SurfPropPtrTab;
extern int* m_countSurfPropsAllocated;
extern uintptr_t m_SurfPropTab;


// CPlantMgr:
extern CPool** pTxdPool; // _ZN9CTxdStore11ms_pTxdPoolE

extern void (*StreamingMakeSpaceFor)(int);
extern void (*ImGonnaUseStreamingMemory)();
extern int (*RwTexDictionaryGetCurrent)();
extern int (*FindTxdSlot)(const char*);
extern int (*AddTxdSlot)(const char*);
extern void (*TxdAddRef)(int);
extern void (*SetCurrentTxd)(int);
extern int* TexDictionaryLinkPluginOff; // A83F5C

extern RwStream* (*RwStreamOpen)(int type, int accessType, int data);
extern void (*RwStreamClose)(RwStream* stream, void* unk);
extern bool (*RwStreamFindChunk)(RwStream* stream, unsigned int, unsigned int, unsigned int);
extern int (*RwTexDictionaryGtaStreamRead)(RwStream* stream);