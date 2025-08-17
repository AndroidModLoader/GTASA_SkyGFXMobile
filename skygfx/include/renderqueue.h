#include <externs.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

// =====================================================================================
// =====================================================================================
// =====================================================================================
// ============================ This Was Taken From My SAMP ============================
// =====================================================================================
// =====================================================================================
// =====================================================================================

#define RQUEUE_QUEUE(__cmd)                                 \
{                                                           \
    (*renderQueue)->curQueueingCommand = __cmd;                \
    *(RQCommand*)((*renderQueue)->mainWorkPointer) = __cmd;    \
    (*renderQueue)->mainWorkPointer += 4;                      \
}

// bytes count should be aligned to 4 or 8 !!!!
#define RQUEUE_WRITEBYTE(__v)                               \
{                                                           \
    *(uint8_t*)((*renderQueue)->mainWorkPointer) = (__v);   \
    (*renderQueue)->mainWorkPointer += sizeof(int);         \
}
#define RQUEUE_READBYTE(__data)                             \
    *(uint8_t*)*__data;                                     \
    *__data += sizeof(int);
#define RQUEUE_PEAKBYTE(__data)                             \
    *(uint8_t*)*__data;

#define RQUEUE_WRITEINT(__v)                                \
{                                                           \
    *(int*)((*renderQueue)->mainWorkPointer) = (int)(__v);  \
    (*renderQueue)->mainWorkPointer += sizeof(int);         \
}
#define RQUEUE_READINT(__data)                              \
    *(int*)*__data;                                         \
    *__data += sizeof(int);
#define RQUEUE_PEAKINT(__data)                              \
    *(int*)*__data;

#define RQUEUE_WRITEFLOAT(__v)                              \
{                                                           \
    *(float*)((*renderQueue)->mainWorkPointer) = (__v);     \
    (*renderQueue)->mainWorkPointer += sizeof(float);       \
}
#define RQUEUE_READFLOAT(__data)                            \
    *(float*)*__data;                                       \
    *__data += sizeof(float);
#define RQUEUE_PEAKFLOAT(__data)                            \
    *(float*)*__data;

#define RQUEUE_WRITEPTR(__v)                                   \
{                                                              \
    *(void**)((*renderQueue)->mainWorkPointer) = (void*)(__v); \
    (*renderQueue)->mainWorkPointer += sizeof(void*);          \
}
#define RQUEUE_READPTR(__data)                              \
    *(void**)*__data;                                       \
    *__data += sizeof(void*);
#define RQUEUE_PEAKPTR(__data)                              \
    *(void**)*__data;

/* This Might Be Incorrect... */
#define RQUEUE_CLOSE()                                                                                              \
{                                                                                                                   \
    if((*renderQueue)->useMutex) OS_MutexObtain((*renderQueue)->commandMutex);                                            \
    uintptr_t expected, workAtomicLength = (uintptr_t)((*renderQueue)->mainWorkPointer - (*renderQueue)->mainPointer);    \
    uintptr_t* mainPtr = (uintptr_t*)&(*renderQueue)->mainPointer;                                                     \
    std::atomic<uintptr_t> workAtomic((uintptr_t)(*renderQueue)->mainPointer);                                         \
    do {                                                                                                            \
        expected = workAtomic + workAtomicLength;                                                                   \
    } while(workAtomic.compare_exchange_weak(expected, *mainPtr));                                                  \
    if((*renderQueue)->useMutex) OS_MutexRelease((*renderQueue)->commandMutex);                                           \
    if(!(*renderQueue)->multiThread) RQ_Process((*renderQueue));                                                          \
    if((*renderQueue)->mainPointer + 1024 > (*renderQueue)->queueEnd) RQ_Flush((*renderQueue));                              \
}

enum ExtendedRQCommand : __int32
{
    EXRQC_START = 0x2000, // Because i want to.

    erqColorMask,
    erqBlendFuncSeparate,
    erqBlendFunc,
    erqEnable,
    erqDisable,
    erqAlphaBlendStatus,
    erqGrabFramebuffer,
    erqGrabFramebufferPost,
    erqSetActiveTexture,
    erqRenderTextureIntoTexture,

    EXRQC_END
};

struct ExtendedRQ
{
    ExtendedRQ() { memset(this, 0, sizeof(*this)); }

    GLint m_aViewportBackup[4] { -1, -1, -1, -1 };
    GLint m_nPrevTex = -1, m_nPrevActiveTex = -1, m_nPrevBuffer = -1;
    GLuint m_NormalMapBuffer { 0 };

    bool m_bAlphaBlending = false;
};
extern ExtendedRQ extRQ;

inline void ERQ_ColorMask(bool r, bool g, bool b, bool a)
{
    RQUEUE_QUEUE(rqDebugMarker);
    
    RQUEUE_WRITEINT(erqColorMask);
    RQUEUE_WRITEINT((r | (g << 1) | (b << 2) | (a << 3)));

    RQUEUE_CLOSE();
}
inline void ERQ_BlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
    RQUEUE_QUEUE(rqDebugMarker);
    
    RQUEUE_WRITEINT(erqBlendFuncSeparate);
    RQUEUE_WRITEINT(sfactorRGB);
    RQUEUE_WRITEINT(dfactorRGB);
    RQUEUE_WRITEINT(sfactorAlpha);
    RQUEUE_WRITEINT(dfactorAlpha);

    RQUEUE_CLOSE();
}
inline bool ERQ_HasAlphaBlending()
{
    RQUEUE_QUEUE(rqDebugMarker);
    RQUEUE_WRITEINT(erqAlphaBlendStatus);
    RQUEUE_CLOSE();

    return extRQ.m_bAlphaBlending;
}
inline void RQ_SetAlphaTest(GLenum func, GLclampf ref)
{
    RQUEUE_QUEUE(rqSetAlphaTest);
    RQUEUE_WRITEINT(func - GL_NEVER);
    RQUEUE_WRITEFLOAT(ref);
    RQUEUE_CLOSE();
}
inline void ERQ_GrabFramebuffer(RwRaster* raster)
{
    RQUEUE_QUEUE(rqDebugMarker);
    RQUEUE_WRITEINT(erqGrabFramebuffer);
    RQUEUE_WRITEPTR(GetES2Raster(raster));
    RQUEUE_CLOSE();
}
inline void ERQ_GrabFramebufferPost()
{
    RQUEUE_QUEUE(rqDebugMarker);
    RQUEUE_WRITEINT(erqGrabFramebufferPost);
    RQUEUE_CLOSE();
}
inline void ERQ_SetActiveTexture(int texNum, GLuint texId)
{
    RQUEUE_QUEUE(rqDebugMarker);
    RQUEUE_WRITEINT(erqSetActiveTexture);
    RQUEUE_WRITEINT(texNum);
    RQUEUE_WRITEINT(texId);
    RQUEUE_CLOSE();
}
inline void ERQ_RenderTextureIntoTexture(RwRaster* src, RwRaster* dst)
{
    RQUEUE_QUEUE(rqDebugMarker);
    RQUEUE_WRITEINT(erqRenderTextureIntoTexture);
    RQUEUE_WRITEPTR(GetES2Raster(src));
    RQUEUE_WRITEPTR(GetES2Raster(dst));
    RQUEUE_CLOSE();
}