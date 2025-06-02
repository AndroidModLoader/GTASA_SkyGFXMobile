#include <externs.h>

// =====================================================================================
// =====================================================================================
// =====================================================================================
// ============================ This Was Taken From My SAMP ============================
// =====================================================================================
// =====================================================================================
// =====================================================================================

#define RQUEUE_QUEUE(__cmd)                                 \
{                                                           \
    renderQueue->curQueueingCommand = __cmd;                \
    *(RQCommand*)(renderQueue->mainWorkPointer) = __cmd;    \
    renderQueue->mainWorkPointer += 4;                      \
}

// bytes count should be aligned to 4 or 8 !!!!
#define RQUEUE_WRITEBYTE(__v)                           \
{                                                       \
    *(uint8_t*)(renderQueue->mainWorkPointer) = (__v);  \
    renderQueue->mainWorkPointer += 4;                  \
}

#define RQUEUE_WRITEINT(__v)                        \
{                                                   \
    *(int*)(renderQueue->mainWorkPointer) = (__v);  \
    renderQueue->mainWorkPointer += 4;              \
}

#define RQUEUE_WRITEFLOAT(__v)                          \
{                                                       \
    *(float*)(renderQueue->mainWorkPointer) = (__v);    \
    renderQueue->mainWorkPointer += 4;                  \
}

#define RQUEUE_WRITEPTR(__v)                                \
{                                                           \
    *(void**)(renderQueue->mainWorkPointer) = (void*)(__v); \
    renderQueue->mainWorkPointer += sizeof(void*);          \
}

/* This Might Be Incorrect... */
#define RQUEUE_CLOSE()                                                                                              \
{                                                                                                                   \
    if(renderQueue->useMutex) OS_MutexObtain(renderQueue->commandMutex);                                            \
    uintptr_t expected, workAtomicLength = (uintptr_t)(renderQueue->mainWorkPointer - renderQueue->mainPointer);    \
    uintptr_t* mainPtr = (uintptr_t*)&renderQueue->mainPointer;                                                     \
    std::atomic<uintptr_t> workAtomic((uintptr_t)renderQueue->mainPointer);                                         \
    do {                                                                                                            \
        expected = workAtomic + workAtomicLength;                                                                   \
    } while(workAtomic.compare_exchange_weak(expected, *mainPtr));                                                  \
    if(renderQueue->useMutex) OS_MutexRelease(renderQueue->commandMutex);                                           \
    if(!renderQueue->multiThread) RQ_Process(renderQueue);                                                          \
    if(renderQueue->mainPointer + 1024 > renderQueue->queueEnd) RQ_Flush(renderQueue);                              \
}

enum ExtendedRQCommand : __int32
{
    EXRQC_START = 0x2000, // Because i want to.

    erqColorMask = 0,
    erqBlendFuncSeparate,
    erqBlendFunc,
    erqEnable,
    erqDisable,

    EXRQC_END
};

struct ExtendedRQ
{

};
extern ExtendedRQ extRQ;