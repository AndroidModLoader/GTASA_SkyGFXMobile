#include <externs.h>

/* Variables */
EmuShader FakeShaderContainer;
ES2Shader* pForcedShader = NULL;
BasicEvent<SimpleVoidFn> shadercreation;

/* Functions */
ES2Shader* CreateCustomShader(uint32_t flags, const char* pxlsrc, const char* vtxsrc, size_t pxllen, size_t vtxlen)
{
    if(!pxllen) pxllen = strlen(pxlsrc);
    if(!vtxlen) vtxlen = strlen(vtxsrc);

    // Original game code is freeing them
    // because originally it is created procedurely
    char* pxl = new char[pxllen + 1]; sprintf(pxl, "%s", pxlsrc);
    char* vtx = new char[vtxlen + 1]; sprintf(vtx, "%s", vtxsrc);

    return RQCreateShader(pxl, vtx, flags);
}

/* Hooks */
DECL_HOOKv(InitialiseGame)
{
    InitialiseGame();
    shadercreation.Call();
}
DECL_HOOKv(AssignEmuShader, bool hasNormals)
{
    if(!pForcedShader) AssignEmuShader(hasNormals);
    else
    {
        FakeShaderContainer.shader = pForcedShader;
        FakeShaderContainer.programFlags = pForcedShader->flags;
        SelectEmuShader(&FakeShaderContainer, true); // Always TRUE in original code?
    }
}

/* Main */
void StartShaders()
{
    HOOKPLT(InitialiseGame, pGTASA + 0x6740A4);
    HOOKPLT(AssignEmuShader, pGTASA + 0x674170);
}