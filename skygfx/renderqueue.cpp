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
void RQ_Command_erqSetActiveTexture(uint8_t** data)
{
    int texNum = RQUEUE_READINT(data);
    GLuint texId = (GLuint)RQUEUE_READINT(data);
    activeTextures[texNum] = texId;
}

/* Hooks */
DECL_HOOKv(RQ_Command_rqDebugMarker, uint8_t** data)
{
    int cmd = RQUEUE_READINT(data);

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
        CASE_RQ( erqSetActiveTexture );
    }
}
DECL_HOOKv(RQ_Command_rqTargetCreate, uint8_t** data)
{
    ES2RenderTarget* target = (ES2RenderTarget*)RQUEUE_READPTR(data);
    ES2Texture* targetTexture = target->targetTexture;
    unsigned int width = targetTexture->width, height = targetTexture->height;

    if(*backBuffer == -1) glGetIntegerv(GL_FRAMEBUFFER_BINDING, backBuffer);

    GLint lastFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFBO);

    // Depthbuf Part
    if(!target->sharedDepth && target->depthType != TDT_None)
    {
    #ifndef TEXTURE_DEPTHBUF
        glGenRenderbuffers(1, &target->depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, target->depthBuffer);
        if(target->depthType == TDT_HighAcc)
        {
            if(RQCaps->hasDepthNonLinearCap)
            {
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16_NONLINEAR_NV, width, height);
            }
            else if(RQCaps->has24BitDepthCap)
            {
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, width, height);
            }
            else
            {
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16_OES, width, height);
            }
        }
        else if(target->depthType == TDT_LowAcc)
        {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16_OES, width, height);
        }
    #else
        if(target->depthType == TDT_LowAcc)
        {
            glGenRenderbuffers(1, &target->depthBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, target->depthBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16_OES, width, height);
        }
    #endif
    }

    // Colorbuf Part
    glGenRenderbuffers(1, &target->colorBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, target->colorBuffer);
    if(target->colorType == TCT_RGBA_8888 && RQCaps->has32BitRenderTargetCap)
    {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, width, height);
    }
    else
    {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565_OES, width, height);
    }

    // Framebuf Part
    glGenFramebuffers(1, &target->frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, target->frameBuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, target->colorBuffer);

    // Attach depthbuffer to framebuffer
#ifndef TEXTURE_DEPTHBUF
    if(target->sharedDepth)
    {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target->sharedDepth->depthBuffer);
    }
    else if(target->depthType != TDT_None)
    {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target->depthBuffer);
    }
#else
    if(target->sharedDepth)
    {
        if(target->sharedDepth->depthType == TDT_HighAcc)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, target->sharedDepth->depthBuffer, 0);
        }
        else
        {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target->sharedDepth->depthBuffer);
        }
    }
    else if(target->depthType == TDT_LowAcc)
    {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target->depthBuffer);
    }
    else if(target->depthType == TDT_HighAcc)
    {
        glGenTextures(1, &target->depthBuffer);
        glBindTexture(GL_TEXTURE_2D, target->depthBuffer);
        if(RQCaps->has24BitDepthCap)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_OES, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, target->depthBuffer, 0);
    }
#endif

    // Generate GL texture for raster
    glGenTextures(1, &targetTexture->texID);
    if(*curActiveTexture != 5)
    {
        glActiveTexture(GL_TEXTURE5);
        *curActiveTexture = 5;
    }
    glBindTexture(GL_TEXTURE_2D, targetTexture->texID);
    boundTextures[5] = targetTexture->texID;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if(target->colorType == TCT_RGBA_8888)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targetTexture->texID, 0);
    glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);
}
DECL_HOOKv(RQ_Command_rqTargetDelete, uint8_t** data)
{
    ES2RenderTarget* target = (ES2RenderTarget*)RQUEUE_READPTR(data);
    ES2Texture* targetTexture = target->targetTexture;

    if(targetTexture) targetTexture->Destruct2();
    glDeleteFramebuffers(1, &target->frameBuffer);
    glDeleteRenderbuffers(1, &target->colorBuffer);
#ifndef TEXTURE_DEPTHBUF
    glDeleteRenderbuffers(1, &target->depthBuffer);
#else
    if(target->depthType == TDT_HighAcc)
    {
        glDeleteTextures(1, &target->depthBuffer);
    }
    else
    {
        glDeleteRenderbuffers(1, &target->depthBuffer);
    }
#endif

    delete target;
}
DECL_HOOKv(InitializeShaderAfterCompile, ES2Shader* self)
{
    InitializeShaderAfterCompile(self);

    GLint id = glGetUniformLocation(self->nShaderId, "DepthTex");
    if(id != -1) glUniform1i(id, 2);
}

/* Main */
void StartRenderQueue()
{
    HOOKPLT(RQ_Command_rqDebugMarker, pGTASA + BYBIT(0x677850, 0x84D0C8));
    HOOKPLT(RQ_Command_rqTargetCreate, pGTASA + BYBIT(0x677024, 0x84C080));
    HOOKPLT(RQ_Command_rqTargetDelete, pGTASA + BYBIT(0x6797B0, 0x850F68));
    HOOK(InitializeShaderAfterCompile, aml->GetSym(hGTASA, "_ZN9ES2Shader22InitializeAfterCompileEv"));
}