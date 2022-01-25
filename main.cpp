#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <dlfcn.h>

// SAUtils
#include <isautils.h>
ISAUtils* sautils = NULL;

#include <string> // memset

#include <shader.h>
#include <gtasa_things.h>
#include <renderware_things.h>

RwFrame *(*RwFrameTransform)(RwFrame * frame, const RwMatrix * m, RwOpCombineType combine);
RpLight *(*RpLightSetColor)(RpLight *light, const RwRGBAReal *color);

MYMODCFG(net.tofw.rusjj.gtasa.skygfx, GTA:SA PS2 Render, 1.0, TheOfficialFloW & RusJJ)
NEEDGAME(com.rockstargames.gtasa)

/* Config */
//ConfigEntry* pBonesOptimization;
//ConfigEntry* pMVPOptimization;
ConfigEntry* pDisablePedSpec;
ConfigEntry* pPS2Sun;
ConfigEntry* pPS2Shading;
ConfigEntry* pColorfilter;
ConfigEntry* pDisableDetailTexture;
ConfigEntry* pFixGreenDetailTexture;
ConfigEntry* pFixMipMaps;
ConfigEntry* pDontShadeUnexploredMap;

/* Patch Saves */
//const uint32_t sunCoronaRet = 0xF0F7ECE0;
const uint32_t sunCoronaDel = 0xBF00BF00;
int* deviceChip;
int* RasterExtOffset;
int* pGTASA_dword6BD1D8;
int* textureDetail;
float *openglAmbientLight;
float _rwOpenGLOpaqueBlack[4];
RwInt32 *p_rwOpenGLColorMaterialEnabled;
CColourSet *p_CTimeCycle__m_CurrentColours;
CVector *p_CTimeCycle__m_vecDirnLightToSun;
float *p_gfLaRiotsLightMult;
float *p_CCoronas__LightsMult;
uint8_t *p_CWeather__LightningFlash;
static float *skin_map;
static int *skin_dirty;
static int *skin_num;

/* Functions declaring */
int (* GetMobileEffectSetting)();
void (*emu_glLightModelfv)(GLenum pname, const GLfloat *params);
void (*emu_glMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
void (*emu_glColorMaterial)(GLenum face, GLenum mode);
void (*emu_glEnable)(GLenum cap);
void (*emu_glDisable)(GLenum cap);

/* Saves */
void* pGTASA = 0;
uintptr_t pGTASAAddr = 0;
uintptr_t pGTASAAddr_Colorfilter = 0;


////////////////////////////////////////////////////
void Redirect(uintptr_t addr, uintptr_t to) // Was taken from TheOfficialFloW's git repo (will be in AML 1.0.0.6)
{
    if(!addr) return;
    if(addr & 1)
    {
        addr &= ~1;
        if (addr & 2)
        {
            aml->PlaceNOP(addr, 1);
            addr += 2;
        }
        uint32_t hook[2];
        hook[0] = 0xF000F8DF;
        hook[1] = to;
        aml->Write(addr, (uintptr_t)hook, sizeof(hook));
    }
    else
    {
        uint32_t hook[2];
        hook[0] = 0xE51FF004;
        hook[1] = to;
        aml->Write(addr, (uintptr_t)hook, sizeof(hook));
    }
}
////////////////////////////////////////////////////

extern char pxlbuf[8192], vtxbuf[8192];
DECL_HOOK(int, RQShaderBuildSource, int flags, char **pxlsrc, char **vtxsrc)
{
    pxlbuf[0] = '\0'; vtxbuf[0] = '\0';
    if(pPS2Shading->GetBool())
    {
        BuildPixelSource_SkyGfx(flags); 
        BuildVertexSource_SkyGfx(flags);
    }
    else
    {
        BuildPixelSource_Reversed(flags); 
        BuildVertexSource_Reversed(flags);
    }
    *pxlsrc = pxlbuf; *vtxsrc = vtxbuf;
    return 1;
}

#define GREEN_TEXTURE_ID 14
DECL_HOOKv(emu_TextureSetDetailTexture, void* texture, unsigned int tilingScale)
{
    if(!texture)
    {
      noDetail:
        emu_TextureSetDetailTexture(NULL, 0);
        return;
    }
    if(texture == *(void**)(**(int**)(*pGTASA_dword6BD1D8 + 4 * (GREEN_TEXTURE_ID-1)) + *RasterExtOffset))
    {
        *textureDetail = 0;
        goto noDetail;
    }
    emu_TextureSetDetailTexture(texture, tilingScale);
    *textureDetail = 1;
}

DECL_HOOKv(glCompressedTex2D, GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
    if (level == 0 || (width >= 4 && height >= 4) || (format != 0x8C01 && format != 0x8C02))
        glCompressedTex2D(target, level, format, width, height, border, imageSize, data);
}

int emu_InternalSkinGetVectorCount(void)
{
    return 4 * *skin_num;
}

void SkinSetMatrices(uintptr_t skin, float* matrix)
{
    int num = *(int*)(skin + 4);
    memcpy(skin_map, matrix, 64 * num);
    *skin_dirty = 1;
    *skin_num = num;
}

extern const char* pColorFilterSettings[4];
extern "C" void OnModLoad()
{
    logger->SetTag("PS2 Render");
// Config
    //pBonesOptimization = cfg->Bind("BonesOptim", false);
    //pMVPOptimization = cfg->Bind("MVPOptim", false);
    pDisablePedSpec = cfg->Bind("DisablePedSpec", true);
    pPS2Sun = cfg->Bind("PS2_Sun", true);
    pPS2Shading = cfg->Bind("PS2_Shading", true);
    pColorfilter = cfg->Bind("Colorfilter", "ps2");
    pDisableDetailTexture = cfg->Bind("DisableDetailTexture", false);
    pFixGreenDetailTexture = cfg->Bind("FixGreenDetailTexture", true);
    pFixMipMaps = cfg->Bind("FixMipMaps", false);
    pDontShadeUnexploredMap = cfg->Bind("DontShadeUnexploredMap", false);
    cfg->Bind("Colorfilter_Values", "default none ps2 pc", "Information");
// Addresses
    pGTASA = dlopen("libGTASA.so", RTLD_LAZY);
    pGTASAAddr = aml->GetLib("libGTASA.so");
    pGTASAAddr_Colorfilter = pGTASAAddr + 0x5B6444 + 0x1;
    pGTASA_dword6BD1D8 = (int*)(pGTASAAddr + 0x6BD1D8);
    sautils = (ISAUtils*)GetInterface("SAUtils");
// Getting GTA:SA functions
    p_pAmbient = (RpLight**)aml->GetSym(pGTASA, "pAmbient");
    p_pDirect = (RpLight**)aml->GetSym(pGTASA, "pDirect");
    p_AmbientLightColourForFrame = (RwRGBAReal*)aml->GetSym(pGTASA, "AmbientLightColourForFrame");
    p_AmbientLightColourForFrame_PedsCarsAndObjects = (RwRGBAReal*)aml->GetSym(pGTASA, "AmbientLightColourForFrame_PedsCarsAndObjects");
    p_DirectionalLightColourForFrame = (RwRGBAReal*)aml->GetSym(pGTASA, "DirectionalLightColourForFrame");
    p_DirectionalLightColourFromDay = (RwRGBAReal*)aml->GetSym(pGTASA, "DirectionalLightColourFromDay");
    p_CTimeCycle__m_CurrentColours = (CColourSet*)aml->GetSym(pGTASA, "_ZN10CTimeCycle16m_CurrentColoursE");
    p_CTimeCycle__m_vecDirnLightToSun = (CVector*)aml->GetSym(pGTASA, "_ZN10CTimeCycle19m_vecDirnLightToSunE");
    p_gfLaRiotsLightMult = (float*)aml->GetSym(pGTASA, "gfLaRiotsLightMult");
    p_CCoronas__LightsMult = (float*)aml->GetSym(pGTASA, "_ZN8CCoronas10LightsMultE");
    p_CWeather__LightningFlash = (uint8_t*)aml->GetSym(pGTASA, "_ZN8CWeather14LightningFlashE");
    // For shading
    openglAmbientLight = (float*)aml->GetSym(pGTASA, "openglAmbientLight");
    p_rwOpenGLColorMaterialEnabled = (RwInt32*)aml->GetSym(pGTASA, "_rwOpenGLColorMaterialEnabled");
    emu_glLightModelfv = (void (*)(GLenum, const GLfloat *))aml->GetSym(pGTASA, "_Z18emu_glLightModelfvjPKf");
    emu_glMaterialfv = (void (*)(GLenum, GLenum, const GLfloat *))aml->GetSym(pGTASA, "_Z16emu_glMaterialfvjjPKf");
    emu_glColorMaterial = (void (*)(GLenum, GLenum))aml->GetSym(pGTASA, "_Z19emu_glColorMaterialjj");    // no-op
    emu_glEnable = (void (*)(GLenum))aml->GetSym(pGTASA, "_Z12emu_glEnablej");
    emu_glDisable = (void (*)(GLenum))aml->GetSym(pGTASA, "_Z13emu_glDisablej");
// Patches
    // Shading
    if(pPS2Shading->GetBool())
    {
        logger->Info("PS2 Shading enabled!");
        aml->PlaceNOP(pGTASAAddr + 0x1C1382);
        aml->PlaceNOP(pGTASAAddr + 0x1C13BA);
        Redirect(aml->GetSym(pGTASA, "_Z36_rwOpenGLLightsSetMaterialPropertiesPK10RpMaterialj"), (uintptr_t)_rwOpenGLLightsSetMaterialProperties);
        Redirect(aml->GetSym(pGTASA, "_Z28SetLightsWithTimeOfDayColourP7RpWorld"), (uintptr_t)SetLightsWithTimeOfDayColour);
        // Bones (for SkyGfx shaders only, not yet, doesnt draw ped models)
        /*if(pBonesOptimization->GetBool())
        {
            skin_map = (float*)aml->GetSym(pGTASA, "skin_map");
            aml->Unprot((uintptr_t)skin_map, 0x1800);
            skin_dirty = (int*)aml->GetSym(pGTASA, "skin_dirty");
            skin_num = (int*)aml->GetSym(pGTASA, "skin_num");
            hook_addr(aml->GetSym(pGTASA, "_Z30emu_InternalSkinGetVectorCountv"), (uintptr_t)emu_InternalSkinGetVectorCount);
            hook_addr(pGTASAAddr + 0x1C8670 + 0x1, (uintptr_t)SkinSetMatrices);
        }*/
    }

    // PS2 Sun Corona
    if(pPS2Sun->GetBool())
    {
        logger->Info("PS2 Sun Corona enabled!");
        aml->Unprot(pGTASAAddr + 0x5A26B0);
        *(uint32_t*)(pGTASAAddr + 0x5A26B0) = sunCoronaDel;
    }

    // Detail textures
    if(pDisableDetailTexture->GetBool())
    {
        logger->Info("Detail textures are disabled!");
        aml->PlaceRET(pGTASAAddr + 0x1BCBC4);
    }
    else if(pFixGreenDetailTexture->GetBool())
    {
        logger->Info("Fixing a green detail texture is enabled!");
        RasterExtOffset = (int*)aml->GetSym(pGTASA, "RasterExtOffset");
        textureDetail = (int*)aml->GetSym(pGTASA, "textureDetail");
        aml->PlaceNOP(pGTASAAddr + 0x1B00B0, 5); // Dont set textureDetail variable! We'll handle it by ourselves!
        HOOK(emu_TextureSetDetailTexture, aml->GetSym(pGTASA, "_Z27emu_TextureSetDetailTexturePvj"));
    }

    // Color Filter
    if(!strcmp(pColorfilter->GetString(), "none")) nColorFilter = 1;
    else if(!strcmp(pColorfilter->GetString(), "ps2")) nColorFilter = 2;
    else if(!strcmp(pColorfilter->GetString(), "pc")) nColorFilter = 3;
    if(nColorFilter != 0)
    {
        logger->Info("Colorfilter \"%s\" enabled!", pColorfilter->GetString());
        RwFrameTransform = (RwFrame *(*)(RwFrame*,const RwMatrix*,RwOpCombineType))aml->GetSym(pGTASA, "_Z16RwFrameTransformP7RwFramePK11RwMatrixTag15RwOpCombineType");
        RpLightSetColor = (RpLight *(*)(RpLight*, const RwRGBAReal*))aml->GetSym(pGTASA, "_Z15RpLightSetColorP7RpLightPK10RwRGBAReal");
        Redirect(pGTASAAddr + 0x5B643C + 0x1, (uintptr_t)ColorFilter_stub);
        aml->Unprot(pGTASAAddr + 0x5B6444, sizeof(uint16_t));
        memcpy((void *)(pGTASAAddr + 0x5B6444), (void *)(pGTASAAddr + 0x5B63DC), sizeof(uint16_t));
        aml->Unprot(pGTASAAddr + 0x5B6446, sizeof(uint16_t));
        memcpy((void *)(pGTASAAddr + 0x5B6446), (void *)(pGTASAAddr + 0x5B63EA), sizeof(uint16_t));

        if(sautils != NULL)
        {
            sautils->AddClickableItem(Display, "Colorfilter", nColorFilter, 0, sizeofA(pColorFilterSettings)-1, pColorFilterSettings, ColorfilterChanged);
        }
    }

    // Mipmaps
    if(pFixMipMaps->GetBool())
	{
        logger->Info("MipMaps fix is enabled!");
		HOOKPLT(glCompressedTex2D, pGTASAAddr + 0x674838);
	}

	// Unexplored map shading
	if(pDontShadeUnexploredMap->GetBool())
	{
        logger->Info("Unexplored map sectors shading disabled!");
		Redirect(pGTASAAddr + 0x2AADE0 + 0x1, pGTASAAddr + 0x2AAF9A + 0x1);
	}

    // Shaders
    GetMobileEffectSetting = (int (*)())aml->GetSym(pGTASA, "_Z22GetMobileEffectSettingv");
    RQCaps = (RQCapabilities*)aml->GetSym(pGTASA, "RQCaps");
    RQMaxBones = (int*)aml->GetSym(pGTASA, "RQMaxBones");
    deviceChip = (int*)aml->GetSym(pGTASA, "deviceChip");
    HOOK(RQShaderBuildSource, aml->GetSym(pGTASA, "_ZN8RQShader11BuildSourceEjPPKcS2_"));
}