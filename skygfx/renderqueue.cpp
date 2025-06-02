#include <externs.h>
#include "include/renderqueue.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// =====================================================================================
// =====================================================================================
// =====================================================================================
// ============================ This Was Taken From My SAMP ============================
// =====================================================================================
// =====================================================================================
// =====================================================================================

/* Variables */


/* Functions */
void RQ_Command_erqColorMask(uint8_t** data)
{
    int colorMask = *(int*)*data;
    *data += sizeof(int);
    glColorMask(colorMask & 0x1, colorMask & 0x2, colorMask & 0x4, colorMask & 0x8);
}
void RQ_Command_erqBlendFuncSeparate(uint8_t** data)
{
    GLenum sfactorRGB   = *(GLenum*)*data; *data += sizeof(int);
    GLenum dfactorRGB   = *(GLenum*)*data; *data += sizeof(int);
    GLenum sfactorAlpha = *(GLenum*)*data; *data += sizeof(int);
    GLenum dfactorAlpha = *(GLenum*)*data; *data += sizeof(int);

    glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}
void RQ_Command_erqBlendFunc(uint8_t** data)
{
    GLenum sfactor = *(GLenum*)*data; *data += sizeof(int);
    GLenum dfactor = *(GLenum*)*data; *data += sizeof(int);
    glBlendFunc(sfactor, dfactor);
}
void RQ_Command_erqEnable(uint8_t** data)
{
    GLenum cap = *(GLenum*)*data; *data += sizeof(int);
    glEnable(cap);
}
void RQ_Command_erqDisable(uint8_t** data)
{
    GLenum cap = *(GLenum*)*data; *data += sizeof(int);
    glDisable(cap);
}

/* Hooks */
DECL_HOOKv(RQ_Command_rqDebugMarker, uint8_t** data)
{
    int cmd = *(int*)*data;
    *data += sizeof(int);

    switch(cmd)
    {
        case erqColorMask:          return RQ_Command_erqColorMask(data);
        case erqBlendFuncSeparate:  return RQ_Command_erqBlendFuncSeparate(data);
        case erqBlendFunc:          return RQ_Command_erqBlendFunc(data);
        case erqEnable:             return RQ_Command_erqEnable(data);
        case erqDisable:            return RQ_Command_erqDisable(data);
    }
}

/* Main */
void StartRenderQueue()
{
    HOOKPLT(RQ_Command_rqDebugMarker, pGTASA + BYBIT(0x677850, 0x84D0C8));
}

ExtendedRQ extRQ;