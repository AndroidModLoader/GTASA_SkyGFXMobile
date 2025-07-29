#include <externs.h>
#include "include/renderqueue.h"

// =====================================================================================
// =====================================================================================
// =====================================================================================
// ============================ This Was Taken From My SAMP ============================
// =====================================================================================
// =====================================================================================
// =====================================================================================

/* Variables */
ExtendedRQ extRQ;

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

    if(glIsEnabled(GL_BLEND))
    {
        glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
    }
    else
    {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
        glDisable(GL_BLEND);
    }
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
void RQ_Command_erqBackupViewport(uint8_t** data)
{
    glGetIntegerv(GL_VIEWPORT, &extRQ.m_aViewportBackup[0]);
    extRQ.m_bViewportBackupped = true;
}
void RQ_Command_erqRestoreViewport(uint8_t** data)
{
    if(extRQ.m_bViewportBackupped)
    {
        glViewport(extRQ.m_aViewportBackup[0], extRQ.m_aViewportBackup[1], extRQ.m_aViewportBackup[2], extRQ.m_aViewportBackup[3]);
        extRQ.m_bViewportBackupped = false;
    }
}
void RQ_Command_erqViewport(uint8_t** data)
{
    int x = RQUEUE_READINT(data);
    int y = RQUEUE_READINT(data);
    glViewport(0, 0, x, y);
}
void RQ_Command_erqRenderFast(uint8_t** data)
{
    GLuint id = (GLuint)RQUEUE_READINT(data);
    int x = RQUEUE_READINT(data);
    int y = RQUEUE_READINT(data);

    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, x, y);
    glBindTexture(GL_TEXTURE_2D, boundTextures[*curActiveTexture]);
}
void RQ_Command_erqAlphaBlendStatus(uint8_t** data)
{
    extRQ.m_bAlphaBlending = (glIsEnabled(GL_BLEND) == GL_TRUE);
}
void RQ_Command_erqGrabFramebuffer(uint8_t** data)
{
    ES2Texture* dst = (ES2Texture*)RQUEUE_READPTR(data);

    extRQ.m_nPrevTex = -1;
    extRQ.m_nPrevActiveTex = -1;
    extRQ.m_nPrevBuffer = -1;
    if(!dst || !(*backTarget) || !(*backTarget)->targetTexture) return;

#ifdef GPU_GRABBER
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &extRQ.m_nPrevBuffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &extRQ.m_nPrevActiveTex);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &extRQ.m_nPrevTex);

    glBindFramebuffer(GL_FRAMEBUFFER, dst->target->frameBuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (*backTarget)->targetTexture->texID);
#else
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &extRQ.m_nPrevBuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &extRQ.m_nPrevTex);

    glBindFramebuffer(GL_FRAMEBUFFER, (*backTarget)->frameBuffer);
    glBindTexture(GL_TEXTURE_2D, dst->texID);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, *renderWidth, *renderHeight);
#endif
}
void RQ_Command_erqGrabFramebufferPost(uint8_t** data)
{
    if(extRQ.m_nPrevBuffer != -1) glBindFramebuffer(GL_FRAMEBUFFER, extRQ.m_nPrevBuffer);
    if(extRQ.m_nPrevActiveTex != -1) glActiveTexture(extRQ.m_nPrevActiveTex);
    if(extRQ.m_nPrevTex != -1) glBindTexture(GL_TEXTURE_2D, extRQ.m_nPrevTex);
}

/* Hooks */
DECL_HOOKv(RQ_Command_rqDebugMarker, uint8_t** data)
{
    int cmd = *(int*)*data;
    *data += sizeof(int);

    // Fallback to the default (not used?)
    if(cmd == GL_GENERATE_MIPMAP_HINT)
    {
        glHint(GL_GENERATE_MIPMAP_HINT, *(GLenum*)*data);
        *data += sizeof(int);
    }

    #define CASE_RQ(__cmd) case __cmd: return RQ_Command_##__cmd(data)

    switch(cmd)
    {
        CASE_RQ( erqColorMask );
        CASE_RQ( erqBlendFuncSeparate );
        CASE_RQ( erqBlendFunc );
        CASE_RQ( erqEnable );
        CASE_RQ( erqDisable );
        CASE_RQ( erqBackupViewport );
        CASE_RQ( erqRestoreViewport );
        CASE_RQ( erqViewport );
        CASE_RQ( erqRenderFast );
        CASE_RQ( erqAlphaBlendStatus );
        CASE_RQ( erqGrabFramebuffer );
        CASE_RQ( erqGrabFramebufferPost );
    }
}

/* Main */
void StartRenderQueue()
{
    HOOKPLT(RQ_Command_rqDebugMarker, pGTASA + BYBIT(0x677850, 0x84D0C8));
}