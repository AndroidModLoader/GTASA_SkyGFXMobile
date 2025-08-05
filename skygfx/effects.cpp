#include <externs.h>
#include "include/renderqueue.h"

/* Enums */
enum eSpeedFX : uint8_t
{
    SPFX_INACTIVE = 0,
    SPFX_TOGGLED_ON,
    SPFX_LIKE_ON_PC,
    SPFX_LIKE_ON_PS2,

    SPEEDFX_SETTINGS
};
enum eUWRipple : uint8_t
{
    UWR_INACTIVE = 0,
    UWR_CLASSIC,
    UWR_SHADER,

    UWR_SETTINGS
};
enum eChromaticAberration : uint8_t
{
    CRAB_INACTIVE = 0,
    CRAB_ENABLED,
    CRAB_INTENSE,

    CRAB_SETTINGS
};
enum eDOF : uint8_t
{
    DOF_INACTIVE = 0,
    DOF_CUTSCENES,
    DOF_ALWAYS,

    DOF_SETTINGS
};

/* Variables */
bool g_bFixSandstorm = true;
bool g_bFixFog = true;
bool g_bExtendRainSplashes = true;
bool g_bMissingEffects = true;
bool g_bMissingPostEffects = true;
bool g_bRenderGrain = true;
int g_nSpeedFX = SPFX_LIKE_ON_PC;
int g_nCrAb = CRAB_INACTIVE;
int g_nVignette = 0;
bool g_bRadiosity = true;
int g_nDOF = DOF_INACTIVE;
bool g_bHeatHaze = false;
float g_fDOFStrength = 1.0f, g_fDOFDistance = 60.0f;
bool g_bDOFUseScale = false;
int g_nUWR = UWR_CLASSIC;
bool g_bCSB = false;
float g_fContrast = 1.0f, g_fSaturation = 1.0f, g_fBrightness = 1.0f, g_fGamma = 1.0f;

int g_nGrainRainStrength = 0;
int g_nInitialGrain = 0;
RwRaster* pGrainRaster = NULL;

int postfxX = 0, postfxY = 0;
RwRaster *pSkyGFXPostFXRaster1 = NULL, *pSkyGFXPostFXRaster2 = NULL, *pSkyGFXDepthRaster = NULL;
RwRaster *pDarkRaster = NULL;

ES2Shader* g_pSimpleInverseShader = NULL;
ES2Shader* g_pSimpleDepthShader = NULL;
ES2Shader* g_pChromaticAberrationShader = NULL;
ES2Shader* g_pChromaticAberrationShader_Intensive = NULL;
ES2Shader* g_pVignetteShader = NULL;
ES2Shader* g_pFakeRayShader = NULL;
ES2Shader* g_pDOFShader = NULL;
ES2Shader* g_pDADOFShader = NULL;
ES2Shader* g_pUnderwaterRippleShader = NULL;
ES2Shader* g_pCSBShader = NULL;

/* Other */
static const char* aSpeedFXSettings[SPEEDFX_SETTINGS] = 
{
    "Disabled",
    "Enabled",
    "Enabled (PC 16:9 fix)",
    "Enabled (PS2 4:3 fix)"
};
static const char* aUWRSettings[UWR_SETTINGS] = 
{
    "Disabled",
    "PC Ripple",
    "Shader"
};
static const char* aCrAbFXSettings[CRAB_SETTINGS] = 
{
    "Disabled",
    "Enabled",
    "Intensive"
};
static const char* aDOFSettings[DOF_SETTINGS] = 
{
    "Disabled",
    "Only cutscenes",
    "Always"
};

/* Configs */
ConfigEntry *pCFGRenderGrain;
ConfigEntry *pCFGSpeedFX;
ConfigEntry *pCFGCrAbFX;
ConfigEntry *pCFGVignette;
ConfigEntry *pCFGRadiosity;
ConfigEntry *pCFGDOF;
ConfigEntry *pCFGUWR;
ConfigEntry* pCFGCSB;

/* Functions */
void CreateEffectsShaders()
{
    char sSimpleInversePxl[] = "precision mediump float;\n"
                               "uniform sampler2D Diffuse;\n"
                               "varying mediump vec2 Out_Tex0;\n"
                               "void main() {\n"
                               "  gl_FragColor = texture2D(Diffuse, vec2(Out_Tex0.x, 1.0 - Out_Tex0.y));\n"
                               "}";
    char sSimpleInverseVtx[] = "precision highp float;\n"
                               "attribute vec3 Position;\n"
                               "attribute vec2 TexCoord0;\n"
                               "varying mediump vec2 Out_Tex0;\n"
                               "void main() {\n"
                               "  gl_Position = vec4(Position.xy, 0.0, 1.0);\n"
                               "  Out_Tex0 = TexCoord0;\n"
                               "}";
    g_pSimpleInverseShader = CreateCustomShaderAlloc(0, sSimpleInversePxl, sSimpleInverseVtx, sizeof(sSimpleInversePxl), sizeof(sSimpleInverseVtx));

    char sSimpleDepthPxl[] = "precision highp float;\n"
                             "uniform sampler2D DepthTex;\n"
                             "varying mediump vec2 Out_Tex0;\n"
                             "uniform highp vec4 GFX1v;\n"
                             "void main() {\n"
                             "  float zNear = GFX1v.x;\n"
                             "  mediump float zFar = GFX1v.y;\n"
                             "  float depth = 2.0 * texture2D(DepthTex, Out_Tex0).r - 1.0;\n"
                             "  depth = 2.0 * zNear / (zFar + zNear - depth * (zFar - zNear));\n"
                             "  gl_FragColor = vec4(vec3(1.0 - depth), 1.0);\n"
                             "}";
    char sSimpleDepthVtx[] = "precision highp float;\n"
                             "attribute vec3 Position;\n"
                             "attribute vec2 TexCoord0;\n"
                             "varying mediump vec2 Out_Tex0;\n"
                             "void main() {\n"
                             "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                             "  Out_Tex0 = TexCoord0;\n"
                             "}";
    g_pSimpleDepthShader = CreateCustomShaderAlloc(0, sSimpleDepthPxl, sSimpleDepthVtx, sizeof(sSimpleDepthPxl), sizeof(sSimpleDepthVtx));

    // https://lettier.github.io/3d-game-shaders-for-beginners/chromatic-aberration.html
    char sChromaPxl[] = "precision mediump float;\n"
                        "uniform sampler2D Diffuse;\n"
                        "varying mediump vec2 Out_Tex0;\n"
                        "varying mediump vec2 vPos;\n"
                        "void main() {\n"
                        "  vec4 Rcolor = texture2D(Diffuse, Out_Tex0 + vPos * 0.009);\n"
                        "  vec4 Gcolor = texture2D(Diffuse, Out_Tex0 + vPos * 0.006);\n"
                        "  vec4 Bcolor = texture2D(Diffuse, Out_Tex0 - vPos * 0.006);\n"
                        "  gl_FragColor = vec4(Rcolor.r, Gcolor.g, Bcolor.ba);\n"
                        "}";
    char sChromaVtx[] = "precision highp float;\n"
                        "attribute vec3 Position;\n"
                        "attribute vec2 TexCoord0;\n"
                        "varying mediump vec2 Out_Tex0;\n"
                        "varying mediump vec2 vPos;\n"
                        "void main() {\n"
                        "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                        "  vPos.x = pow(gl_Position.x * 0.5, 3.0);\n"
                        "  vPos.y = pow(gl_Position.y, 3.0);\n"
                        "  Out_Tex0 = TexCoord0;\n"
                        "}";
    g_pChromaticAberrationShader = CreateCustomShaderAlloc(0, sChromaPxl, sChromaVtx, sizeof(sChromaPxl), sizeof(sChromaVtx));
    
    char sChromaVtx2[] = "precision highp float;\n"
                         "attribute vec3 Position;\n"
                         "attribute vec2 TexCoord0;\n"
                         "varying mediump vec2 Out_Tex0;\n"
                         "varying mediump vec2 vPos;\n"
                         "void main() {\n"
                         "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                         "  vPos.x = gl_Position.x * 0.5;\n"
                         "  vPos.y = gl_Position.y;\n"
                         "  Out_Tex0 = TexCoord0;\n"
                         "}";
    g_pChromaticAberrationShader_Intensive = CreateCustomShaderAlloc(0, sChromaPxl, sChromaVtx2, sizeof(sChromaPxl), sizeof(sChromaVtx2));

    // https://github.com/TyLindberg/glsl-vignette
    // mediump -> highp for smoother vignette
    char sVignettePxl[] = "precision highp float;\n"
                          "uniform sampler2D Diffuse;\n"
                          "varying mediump vec2 Out_Tex0;\n"
                          "float vignette(vec2 uv, float radius, float smoothness) {\n"
                          "  float diff = radius - distance(uv, vec2(0.5, 0.5));\n"
                          "  return smoothstep(-smoothness, smoothness, diff);\n"
                          "}\n"
                          "void main() {\n"
                          "  float vignetteValue = 1.0 - vignette(Out_Tex0, 0.5, 0.5);\n"
                          "  gl_FragColor = texture2D(Diffuse, Out_Tex0) * vignetteValue;\n"
                          "}";
    char sVignetteVtx[] = "precision highp float;\n"
                          "attribute vec3 Position;\n"
                          "attribute vec2 TexCoord0;\n"
                          "varying mediump vec2 Out_Tex0;\n"
                          "void main() {\n"
                          "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                          "  Out_Tex0 = TexCoord0;\n"
                          "}";
    g_pVignetteShader = CreateCustomShaderAlloc(0, sVignettePxl, sVignetteVtx, sizeof(sVignettePxl), sizeof(sVignetteVtx));

    char sFakeRayPxl[] = "precision mediump float;\n"
                         "uniform sampler2D Diffuse;\n"
                         "uniform sampler2D DepthTex;\n"
                         "varying mediump vec2 Out_Tex0;\n"
                         "varying mediump vec2 vPos;\n"
                         "void main() {\n"
                         "  vec2 tex = Out_Tex0 - 0.5;\n"
                         "  float pz = 0.0;\n"
                         "  for(float i = 0.0; i < 50.0; i++) {\n"
                         "    vec3 shiftColor = texture2D(Diffuse, (tex *= 0.99) + 0.5).rgb;\n"
                         "    float depth = texture2D(DepthTex, tex + 0.5).r;\n"
                         "    if(depth < 0.01) {\n"
                         "      float pixelBrightness = dot(shiftColor, vec3(0.299, 0.587, 0.114));\n"
                         "      pz += pow(max(0.0, 0.5 - length(1.0 - shiftColor.rg)), 2.0) * exp(-i * 0.1);\n"
                         "    }\n"
                         "  }\n"
                         "  gl_FragColor = vec4(texture2D(Diffuse, Out_Tex0).rgb + pz, 1.0);\n"
                         "}";
    char sFakeRayVtx[] = "precision highp float;\n"
                         "attribute vec3 Position;\n"
                         "attribute vec2 TexCoord0;\n"
                         "varying mediump vec2 Out_Tex0;\n"
                         "varying mediump vec2 vPos;\n"
                         "void main() {\n"
                         "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                         "  vPos = gl_Position.xy;\n"
                         "  Out_Tex0 = TexCoord0;\n"
                         "}";
    g_pFakeRayShader = CreateCustomShaderAlloc(0, sFakeRayPxl, sFakeRayVtx, sizeof(sFakeRayPxl), sizeof(sFakeRayVtx));

    char sDOFPxl[] = "precision mediump float;\n"
                     "uniform sampler2D Diffuse;\n"
                     "uniform sampler2D DepthTex;\n"
                     "varying mediump vec2 Out_Tex0;\n"
                     "uniform mediump vec4 GFX1v;\n"
                     "const float steps = 3.0;\n"
                     "const float blurPasses = 6.0;\n"
                     "void main() {\n"
                     "  vec3 color = texture2D(Diffuse, Out_Tex0).rgb;\n"
                     "  float coc = clamp((GFX1v.w - texture2D(DepthTex, Out_Tex0).r) * GFX1v.z, 0.0, 1.0);\n"
                     "  if(coc > 0.0) {\n"
                     "    int radius = int(blurPasses * coc);\n"
                     "    float passes = 1.0;\n"
                     "    for(int x = -radius; x <= radius; x += 2) {\n"
                     "      for(int y = -radius; y <= radius; y += 2) {\n"
                     "        color += texture2D(Diffuse, Out_Tex0 + vec2(float(x), float(y)) * GFX1v.xy).rgb;\n"
                     "        passes += 1.0;\n"
                     "      }\n"
                     "    }\n"
                     "    color /= passes;\n"
                     "  }\n"
                     "  gl_FragColor = vec4(color, 1.0);\n"
                     "}";
    char sDOFVtx[] = "precision highp float;\n"
                     "attribute vec3 Position;\n"
                     "attribute vec2 TexCoord0;\n"
                     "varying mediump vec2 Out_Tex0;\n"
                     "void main() {\n"
                     "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                     "  Out_Tex0 = TexCoord0;\n"
                     "}";
    g_pDOFShader = CreateCustomShaderAlloc(0, sDOFPxl, sDOFVtx, sizeof(sDOFPxl), sizeof(sDOFVtx));

    char sDOFDAPxl[] = "precision mediump float;\n"
                       "uniform sampler2D Diffuse;\n"
                       "uniform sampler2D DepthTex;\n"
                       "varying mediump vec2 Out_Tex0;\n"
                       "uniform mediump vec4 GFX1v;\n"
                       "const float steps = 3.0;\n"
                       "const float blurPasses = 6.0;\n"
                       "void main() {\n"
                       "  vec3 color = texture2D(Diffuse, Out_Tex0).rgb;\n"
                       "  highp float pixelDepth = texture2D(DepthTex, Out_Tex0).r;\n"
                       "  mediump float coc = clamp((GFX1v.w - pixelDepth) * GFX1v.z, 0.0, 1.0);\n"
                       "  if(coc > 0.0) {\n"
                       "    int radius = int(blurPasses * coc);\n"
                       "    float passes = 1.0;\n"
                       "    for(int x = -radius; x <= radius; x += 2) {\n"
                       "      for(int y = -radius; y <= radius; y += 2) {\n"
                       "        float targetDepth = texture2D(DepthTex, Out_Tex0 + vec2(x, y) * GFX1v.xy).r;\n"
                       "        if(targetDepth > pixelDepth) continue;\n"
                       "        color += texture2D(Diffuse, Out_Tex0 + vec2(x, y) * GFX1v.xy).rgb;\n"
                       "        passes += 1.0;\n"
                       "      }\n"
                       "    }\n"
                       "    color /= passes;\n"
                       "  }\n"
                       "  gl_FragColor = vec4(color, 1.0);\n"
                       "}";
    char sDOFDAVtx[] = "precision highp float;\n"
                       "attribute vec3 Position;\n"
                       "attribute vec2 TexCoord0;\n"
                       "varying mediump vec2 Out_Tex0;\n"
                       "void main() {\n"
                       "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                       "  Out_Tex0 = TexCoord0;\n"
                       "}";
    g_pDADOFShader = CreateCustomShaderAlloc(0, sDOFDAPxl, sDOFDAVtx, sizeof(sDOFDAPxl), sizeof(sDOFDAVtx));

    char sUWRPxl[] = "precision mediump float;\n"
                     "uniform sampler2D Diffuse;\n"
                     "uniform mediump vec4 GFX1v;\n"
                     "varying mediump vec2 Out_Tex0;\n"
                     "varying mediump vec2 vPos;\n"
                     "const float pi = 3.141593;\n"
                     "const float pi2 = 6.283185;\n"
                     "const float pi4 = 12.566371;\n"
                     "void main() {\n"
                     "  mediump vec2 tex = Out_Tex0;\n"
                     "  float xo = 0.004 * GFX1v.x;\n"
                     "  float yo = 77.14 * vPos.y;\n"
                     "  float wave = sin(GFX1v.z * yo * pi2 + GFX1v.y) * xo;\n"
                     "  tex.x += wave * min(1.0, pow(1.7 - abs(vPos.x), 4.0));\n"
                     "  gl_FragColor = vec4(texture2D(Diffuse, tex).rgb, 1.0);\n"
                     "}";
    char sUWRVtx[] = "precision highp float;\n"
                     "attribute vec3 Position;\n"
                     "attribute vec2 TexCoord0;\n"
                     "varying mediump vec2 Out_Tex0;\n"
                     "varying mediump vec2 vPos;\n"
                     "void main() {\n"
                     "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                     "  vPos = gl_Position.xy;\n"
                     "  Out_Tex0 = TexCoord0;\n"
                     "}";
    g_pUnderwaterRippleShader = CreateCustomShaderAlloc(0, sUWRPxl, sUWRVtx, sizeof(sUWRPxl), sizeof(sUWRVtx));

    // https://github.com/SableRaf/Filters4Processing/blob/master/sketches/ContrastSaturationBrightness/data/ContrastSaturationBrightness.glsl
    char sCSBPxl[] = "precision mediump float;\n"
                     "uniform sampler2D Diffuse;\n"
                     "uniform mediump vec4 GFX1v;\n"
                     "varying mediump vec2 Out_Tex0;\n"
                     "const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);\n"
                     "const vec3 AvgLumin = vec3(0.5, 0.5, 0.5);\n"
                     "void main() {\n"
                     "  vec3 color = texture2D(Diffuse, Out_Tex0).rgb;\n"
                     "  vec3 brtColor  = color * GFX1v.z;\n"
                     "  vec3 intensity = vec3(dot(brtColor, LumCoeff));\n"
                     "  vec3 satColor  = mix(intensity, brtColor, GFX1v.y);\n"
                     "  gl_FragColor = vec4(pow(mix(AvgLumin, satColor, GFX1v.x), vec3(GFX1v.w)), 1.0);\n"
                     "}";
    char sCSBVtx[] = "precision highp float;\n"
                     "attribute vec3 Position;\n"
                     "attribute vec2 TexCoord0;\n"
                     "varying mediump vec2 Out_Tex0;\n"
                     "void main() {\n"
                     "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                     "  Out_Tex0 = TexCoord0;\n"
                     "}";
    g_pCSBShader = CreateCustomShaderAlloc(0, sCSBPxl, sCSBVtx, sizeof(sCSBPxl), sizeof(sCSBVtx));
}
void RenderGrainSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGRenderGrain->SetBool(newVal != 0);
    g_bRenderGrain = pCFGRenderGrain->GetBool();

    cfg->Save();
}
void SpeedFXSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGSpeedFX->SetInt(newVal);
    pCFGSpeedFX->Clamp(0, SPEEDFX_SETTINGS-1);
    g_nSpeedFX = pCFGSpeedFX->GetInt();

    cfg->Save();
}
void CrAbFXSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGCrAbFX->SetInt(newVal);
    pCFGCrAbFX->Clamp(0, CRAB_SETTINGS-1);
    g_nCrAb = pCFGCrAbFX->GetInt();

    cfg->Save();
}
void VignetteSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGVignette->SetInt(newVal);
    pCFGVignette->Clamp(0, 100);
    g_nVignette = pCFGVignette->GetInt();

    cfg->Save();
}
void RadiositySettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGRadiosity->SetBool(newVal != 0);
    g_bRadiosity = pCFGRadiosity->GetBool();

    cfg->Save();
}
void DOFSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGDOF->SetInt(newVal);
    pCFGDOF->Clamp(0, DOF_SETTINGS);
    g_nDOF = pCFGDOF->GetInt();

    cfg->Save();
}
void UWRSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGUWR->SetInt(newVal);
    pCFGUWR->Clamp(0, UWR_SETTINGS);
    g_nUWR = pCFGUWR->GetInt();

    cfg->Save();
}
void CSBSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGCSB->SetBool(newVal != 0);
    g_bCSB = pCFGCSB->GetBool();

    cfg->Save();
}
void CreateGrainTexture(RwRaster* target)
{
    if(!target) return;

    RwUInt8 *pixels = RwRasterLock(target, 0, 1);
    vrinit(rand());
    int x = vrget(), pixSize = target->width * target->height;
    for(int i = 0; i < pixSize; ++i)
    {
        *(pixels++) = x;
        *(pixels++) = x;
        *(pixels++) = x;
        *(pixels++) = 80;
        x = vrnext();
    }
    RwRasterUnlock(target);
}
void CreatePlainTexture(RwRaster* target, CRGBA clr)
{
    if(!target) return;

    RwUInt8 *pixels = RwRasterLock(target, 0, 1);
    int pixSize = target->width * target->height;
    for(int i = 0; i < pixSize; ++i)
    {
        *(pixels++) = clr.r;
        *(pixels++) = clr.g;
        *(pixels++) = clr.b;
        *(pixels++) = clr.a;
    }
    RwRasterUnlock(target);
}
void RenderGrainEffect(uint8_t strength)
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    
    float uOffset = (float)rand() / (float)RAND_MAX;
    float vOffset = (float)rand() / (float)RAND_MAX;

    // Optimised algorithm. We dont need to create it every frame.
    float umin = uOffset;
    float vmin = vOffset;
    float umax = (1.5f * RsGlobal->maximumWidth) / 640.0f + umin;
    float vmax = (1.5f * RsGlobal->maximumHeight) / 640.0f + vmin;
    
    DrawQuadSetUVs(umin, vmax, umax, vmax, umax, vmin, umin, vmin);
    
    PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 255, 255, 255, strength, pGrainRaster);
    
    DrawQuadSetDefaultUVs();
    ImmediateModeRenderStatesReStore();
}
void GFX_ActivateRawDepthTexture()
{
    ERQ_SetActiveTexture(2, (*backTarget)->depthBuffer);
}
void GFX_ActivateProcessedDepthTexture()
{
    ES2Texture* depthTexture = *(ES2Texture**)( (char*)&pSkyGFXDepthRaster->parent + *RasterExtOffset );
    ERQ_SetActiveTexture(2, depthTexture->texID);
}
void GFX_DeActivateDepthTexture()
{
    ERQ_SetActiveTexture(2, 0);
}
void GFX_CheckBuffersSize()
{
    if(postfxX != *renderWidth || postfxY != *renderHeight)
    {
        postfxX = *renderWidth;
        postfxY = *renderHeight;

        if(pSkyGFXPostFXRaster1) RwRasterDestroy(pSkyGFXPostFXRaster1);
        pSkyGFXPostFXRaster1 = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMATDEFAULT);

        if(pSkyGFXPostFXRaster2) RwRasterDestroy(pSkyGFXPostFXRaster2);
        pSkyGFXPostFXRaster2 = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMATDEFAULT);

        if(pSkyGFXDepthRaster) RwRasterDestroy(pSkyGFXDepthRaster);
        pSkyGFXDepthRaster = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMATDEFAULT);
    }
}
void GFX_GrabScreen(bool second = false)
{
    GFX_CheckBuffersSize();

    if((!second && pSkyGFXPostFXRaster1) || (second && pSkyGFXPostFXRaster2))
    {
        ERQ_GrabFramebuffer(second ? pSkyGFXPostFXRaster2 : pSkyGFXPostFXRaster1);
    #ifdef GPU_GRABBER
        emu_glBegin(5u);
        emu_glVertex3f(-1.0, 1.0, *gradeBlur);
        emu_glTexCoord2f(0.0, 1.0);
        emu_glVertex3f(1.0, 1.0, *gradeBlur);
        emu_glTexCoord2f(1.0, 1.0);
        emu_glVertex3f(-1.0, -1.0, *gradeBlur);
        emu_glTexCoord2f(0.0, 0.0);
        emu_glVertex3f(1.0, -1.0, *gradeBlur);
        emu_glTexCoord2f(1.0, 0.0);
        emu_glEnd();
    #endif
        ERQ_GrabFramebufferPost();
    }
}
void GFX_GrabDepth()
{
    GFX_CheckBuffersSize();

    if(pSkyGFXDepthRaster && Scene->camera)
    {
        ImmediateModeRenderStatesStore();
        ImmediateModeRenderStatesSet();

        GFX_ActivateRawDepthTexture();

        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

        RQVector uniValues = RQVector{ Scene->camera->nearClip, Scene->camera->farClip, 0.0f, 0.0f };
        pForcedShader = g_pSimpleDepthShader;
        pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);

        float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
        DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
        PostEffectsDrawQuad(0.0f, 0.0f, 2.0f, 2.0f, 255, 255, 255, 255, NULL);
        pForcedShader = NULL;

        ERQ_GrabFramebuffer(pSkyGFXDepthRaster);
    #ifdef GPU_GRABBER
        emu_glBegin(5u);
        emu_glVertex3f(-1.0, 1.0, *gradeBlur);
        emu_glTexCoord2f(0.0, 1.0);
        emu_glVertex3f(1.0, 1.0, *gradeBlur);
        emu_glTexCoord2f(1.0, 1.0);
        emu_glVertex3f(-1.0, -1.0, *gradeBlur);
        emu_glTexCoord2f(0.0, 0.0);
        emu_glVertex3f(1.0, -1.0, *gradeBlur);
        emu_glTexCoord2f(1.0, 0.0);
        emu_glEnd();
    #endif
        ERQ_GrabFramebufferPost();

        ImmediateModeRenderStatesReStore();
    }
}
void GFX_CCTV() // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    uint32 lineHeight  = (uint32)(2.0f * ((float)RsGlobal->maximumHeight) / 448.0f);
    uint32 linePadding = 2 * lineHeight;
    uint32 numLines    = RsGlobal->maximumHeight / linePadding;
    for (auto i = 0u, y = 0u; i < numLines; i++, y += linePadding)
    {
        // Pure black lines. We dont need to waste CPU by fixing this:
        /*float umin = 0;
        float vmin = y / (float)RsGlobal->maximumHeight;
        float umax = 1.0f;
        float vmax = (y + lineHeight) / (float)RsGlobal->maximumHeight;
        
        DrawQuadSetUVs(umin, -vmin, umax, -vmin, umax, -vmax, umin, -vmax);*/
        PostEffectsDrawQuad(0.0f, y, RsGlobal->maximumWidth, lineHeight, 0, 0, 0, 255, pSkyGFXPostFXRaster1);
    }

    ImmediateModeRenderStatesReStore();
}
void GFX_SpeedFX(float speed) // Completed
{
    fxSpeedSettings* fx = NULL;
    for(int i = 6; i >= 0; --i)
    {
        if(speed >= FX_SPEED_VARS[i].fSpeedThreshHold)
        {
            fx = &FX_SPEED_VARS[i];
            break;
        }
    }
    if(!fx) return;

#ifdef AML32
    int DirectionWasLooking = TheCamera->m_apCams[TheCamera->m_nCurrentActiveCam].DirectionWasLooking;
#else
    int DirectionWasLooking = TheCamera->Cams[TheCamera->ActiveCam].DirectionWasLooking;
#endif

    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    const float ar43 = 4.0f / 3.0f;
    const float ar169 = 16.0f / 9.0f;
    float ar = (float)RsGlobal->maximumWidth / (float)RsGlobal->maximumHeight;
    float arM = 1.0f;
         if(g_nSpeedFX == SPFX_LIKE_ON_PC)  arM = ar169 / ar;
    else if(g_nSpeedFX == SPFX_LIKE_ON_PS2) arM = ar43  / ar;
    

    int targetShift = fx->nShift;
    int targetShake = fx->nShake;
    int nLoops = fx->nLoops;
    if(DirectionWasLooking <= 2)
    {
        if(targetShift < 1) targetShift = 1;
        targetShift *= 2;

        targetShake = 0;
    }

    // Calculated once per frame
    float uOffset = 0.0f, vOffset = 0.0f, fShiftOffset = (float)targetShift / 400.0f;
    if(targetShake > 0)
    {
        uOffset = ((float)targetShake / 250.0f) * ((float)rand() / (float)RAND_MAX);
        vOffset = ((float)targetShake / 250.0f) * ((float)rand() / (float)RAND_MAX);
    }

    // Unique per loop (gets bigger every iteration)
    float fLoopShiftX1 = fShiftOffset, fLoopShiftX2 = fShiftOffset,
          fLoopShiftY1 = fShiftOffset, fLoopShiftY2 = fShiftOffset;
    for(int i = 0; i < nLoops; ++i)
    {
        float umin = arM * ( ( (DirectionWasLooking > 2) ? 0.0f : uOffset ) + ( (DirectionWasLooking == 2) ? 0.0f : fLoopShiftX1 ) );
        float vmin = ( (DirectionWasLooking <= 2) ? 0.0f : vOffset ) + ( (DirectionWasLooking > 2) ? 0.0f : fLoopShiftY1 );
        float umax = 1.0f - arM * ( ( (DirectionWasLooking > 2) ? 0.0f : uOffset ) + ( (DirectionWasLooking == 1) ? 0.0f : fLoopShiftX2 ) );
        float vmax = 1.0f - ( (DirectionWasLooking > 2) ? 0.0f : fLoopShiftY2 );
        DrawQuadSetUVs(umin, vmax - ( (DirectionWasLooking > 2) ? 0.0f : vOffset ), umax, vmax - ( (DirectionWasLooking > 2) ? 0.0f : uOffset ), umax, vmin, umin, vmin);

        PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 255, 255, 255, 36, pSkyGFXPostFXRaster1);

        if(i > 0) // Just a little optimisation, don't waste CPU for that
        {
            fLoopShiftX1 = fShiftOffset + ( (DirectionWasLooking == 2) ? 0.0f : fLoopShiftX1 );
            fLoopShiftY1 = fShiftOffset + ( (DirectionWasLooking > 2) ? 0.0f : fLoopShiftY1 );
            fLoopShiftX2 = fShiftOffset + ( (DirectionWasLooking == 1) ? 0.0f : fLoopShiftX2 );
            fLoopShiftY2 = fShiftOffset + ( (DirectionWasLooking > 2) ? 0.0f : fLoopShiftY2 );
        }
    }
    ImmediateModeRenderStatesReStore();
}
void GFX_Radiosity(int intensityLimit, int filterPasses, int renderPasses, int intensity) // Does not work?
{
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)false);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pSkyGFXPostFXRaster1);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)false);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

    TempBufferVerticesStored = 0;

    const float m_RadiosityFilterUCorrection = 2.0f, m_RadiosityFilterVCorrection = 2.0f;
    float m_RadiosityPixelsX = RsGlobal->maximumWidth, m_RadiosityPixelsY = RsGlobal->maximumHeight;
    const float rhw = 1.0f / Scene->camera->nearClip;
    const float fRasterWidth = pSkyGFXPostFXRaster1->width;
    const float fRasterHeight = pSkyGFXPostFXRaster1->height;

    Set2DQuadRHW(rhw, 0.0f, 0.0f, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 0.0f, 0.0f, 1.0f, 1.0f, RwRGBA(255,255,255));

    TempBufferVerticesStored = 0;
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    if(filterPasses > 0)
    {
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
        for(int i = 0; i < filterPasses; ++i)
        {
            float secU = m_RadiosityPixelsX / fRasterWidth;
            float secV = m_RadiosityPixelsY / fRasterHeight;
            m_RadiosityPixelsX = ( (m_RadiosityPixelsX > 0) ? (m_RadiosityPixelsX * 0.5f) : 1.0f );
            m_RadiosityPixelsY = ( (m_RadiosityPixelsY > 0) ? (m_RadiosityPixelsY * 0.5f) : 1.0f );

            Set2DQuadRHW(rhw, 0.0f, 0.0f, RsGlobal->maximumWidth, RsGlobal->maximumHeight,
                         m_RadiosityFilterUCorrection / fRasterWidth, m_RadiosityFilterVCorrection / fRasterHeight,
                         secU, secV, RwRGBA(255,255,255));
        }
    }

    Set2DQuadRHW(rhw, 0.0f, 0.0f, m_RadiosityPixelsX + 1.0f, m_RadiosityPixelsY + 1.0f,
                         0.5f / fRasterWidth, 0.5f / fRasterHeight,
                         (m_RadiosityPixelsX + 0.5f) / fRasterWidth, (m_RadiosityPixelsY + 0.5f) / fRasterHeight,
                         RwRGBA(intensityLimit, intensityLimit, intensityLimit, 128));
    
    if(renderPasses > 0)
    {
        if(TempBufferVerticesStored > 2)
        {
            RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, TempVertexBuffer.m_2d, TempBufferVerticesStored);
        }
        TempBufferVerticesStored = 0;

        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);

        for(int i = 0; i < renderPasses; ++i)
        {
            Set2DQuadRHW(rhw, 0.0f, 0.0f, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 0.0f, 0.0f,
                         m_RadiosityPixelsX / fRasterWidth, m_RadiosityPixelsY / fRasterHeight, RwRGBA(0,0,0,intensity));
        }
    }

    if(TempBufferVerticesStored > 2)
    {
        RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, TempVertexBuffer.m_2d, TempBufferVerticesStored);
    }

    TempBufferVerticesStored = 0;
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)false);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
}
void GFX_HeatHaze(float intensity, bool alphaMaskMode)
{
    pForcedShader = g_pSimpleInverseShader;
    RwRaster* bak = *pRasterFrontBuffer;
    *pRasterFrontBuffer = pSkyGFXPostFXRaster1;
    HeatHazeFX(intensity, alphaMaskMode);
    *pRasterFrontBuffer = bak;
    pForcedShader = NULL;
}
// https://github.com/gta-reversed/gta-reversed/blob/7dc7e9696214e17594e8a9061be9dd808d8ae9c5/source/game_sa/PostEffects.cpp#L428
void GFX_UnderWaterRipple(CRGBA col, float xo, float yo, int strength, float speed, float freq) // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    col.a = 255;
    TempBufferVerticesStored = 0;
    TempBufferIndicesStored = 0;

    const int nYO = (int)yo;
    const float recipNearClip = 1.0f / (Scene->camera->nearClip);
    const float nearScreen = RwIm2DGetNearScreenZ();

    const float nScreenX = RsGlobal->maximumWidth;
    const float nScreenY = RsGlobal->maximumHeight;
    const float fScreenX = (float)nScreenX;
    const float fScreenY = (float)nScreenY;
    const float fInvScreenX = 1.0f / (float)RsGlobal->maximumWidth;
    const float fInvScreenY = 1.0f / (float)RsGlobal->maximumHeight;

    const auto EmitVertex = [&](float wave, int32 y)
    {
        const auto i = TempBufferVerticesStored;

        TempVertexBuffer.m_2d[i].pos.x          = 0.0f;
        TempVertexBuffer.m_2d[i].pos.y          = float(y);
        TempVertexBuffer.m_2d[i].pos.z          = nearScreen;
        TempVertexBuffer.m_2d[i].rhw            = recipNearClip;
        TempVertexBuffer.m_2d[i].texCoord.u     = (wave + xo) * fInvScreenX;
        TempVertexBuffer.m_2d[i].texCoord.v     = 1.0f - float(y) * fInvScreenY;
        TempVertexBuffer.m_2d[i].rgba           = *(RwRGBA*)&col;

        TempVertexBuffer.m_2d[i + 1].pos.x      = float(int32(2.0f * xo) + fScreenX);
        TempVertexBuffer.m_2d[i + 1].pos.y      = float(y);
        TempVertexBuffer.m_2d[i + 1].pos.z      = nearScreen;
        TempVertexBuffer.m_2d[i + 1].rhw        = recipNearClip;
        TempVertexBuffer.m_2d[i + 1].texCoord.u = (fScreenX + wave - xo) * fInvScreenX;
        TempVertexBuffer.m_2d[i + 1].texCoord.v = 1.0f - float(y) * fInvScreenY;
        TempVertexBuffer.m_2d[i + 1].rgba       = *(RwRGBA*)&col;

        TempBufferVerticesStored += 2;
    };

    int y = 0;
    for(; y < nScreenY; y += nYO)
    {
        EmitVertex(sinf(freq * y + speed * *m_snTimeInMilliseconds) * xo, y);
    }
    EmitVertex(sinf(freq * y + speed * *m_snTimeInMilliseconds) * xo, y);

    if(TempBufferVerticesStored > 2) 
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pSkyGFXPostFXRaster1);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
        RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, TempVertexBuffer.m_2d, TempBufferVerticesStored);
    }
    
    ImmediateModeRenderStatesReStore();
}
void GFX_UnderWaterRipple_Shader(CRGBA col, float xoIntensity, float speed, float freq)
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    RQVector uniValues = RQVector{ xoIntensity, speed * *m_snTimeInMilliseconds, freq, 0.0f };
    g_pUnderwaterRippleShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);

    pForcedShader = g_pUnderwaterRippleShader;
    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0f, 0.0f, 2.0f, 2.0f, col.r, col.g, col.b, 255, pSkyGFXPostFXRaster1);
    pForcedShader = NULL;

    ImmediateModeRenderStatesReStore();
}
void GFX_DarknessFilter(int alpha) // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmax, umax, vmax, umax, vmin, umin, vmin);
    PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 0, 0, 0, alpha, NULL);
    
    ImmediateModeRenderStatesReStore();
}
void GFX_ChromaticAberration() // Completed
{
    GFX_GrabScreen(true);

    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    pForcedShader = ( g_nCrAb == CRAB_INTENSE ? g_pChromaticAberrationShader_Intensive : g_pChromaticAberrationShader );
    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0f, 0.0f, 2.0f, 2.0f, 255, 255, 255, 255, pSkyGFXPostFXRaster2);
    pForcedShader = NULL;
    
    ImmediateModeRenderStatesReStore();
}
void GFX_Vignette(int alpha) // Completed
{
    if(alpha <= 0) return;
    if(alpha > 255) alpha = 255;

    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    pForcedShader = g_pVignetteShader;
    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmax, umax, vmax, umax, vmin, umin, vmin);
    PostEffectsDrawQuad(0.0f, 0.0f, 2.0f, 2.0f, 0, 0, 0, alpha, pDarkRaster);
    pForcedShader = NULL;
    
    ImmediateModeRenderStatesReStore();
}
void GFX_FakeRay()
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    pForcedShader = g_pFakeRayShader;
    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0, 0.0, 2.0f, 2.0f, 255, 255, 255, 255, pSkyGFXPostFXRaster1);
    pForcedShader = NULL;
    
    ImmediateModeRenderStatesReStore();
}
void GFX_DOF() // Completed
{
    float calc01Dist;
    if(g_bDOFUseScale)
    {
        if(g_fDOFDistance < 0.0f || g_fDOFDistance >= 0.99f) return;
        calc01Dist = 1.0f - g_fDOFDistance;
    }
    else
    {
        if(g_fDOFDistance >= Scene->camera->farClip) return;
        calc01Dist = 1.0f - g_fDOFDistance / (Scene->camera->farClip - Scene->camera->nearClip);
    }

    GFX_GrabScreen(true);
    
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    RQVector uniValues = RQVector{ 1.0f / (float)postfxX, 1.0f / (float)postfxY, 2.0f * g_fDOFStrength, calc01Dist };
    pForcedShader = g_pDOFShader; // g_pDADOFShader (not yet little guys)
    pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);

    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0, 0.0, 2.0f, 2.0f, 255, 255, 255, 255, pSkyGFXPostFXRaster2);
    pForcedShader = NULL;
    
    ImmediateModeRenderStatesReStore();
}
void GFX_FrameBuffer() // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmax, umax, vmax, umax, vmin, umin, vmin);
    PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 255, 255, 255, 255, pSkyGFXPostFXRaster1);
    
    ImmediateModeRenderStatesReStore();
}
void GFX_FrameBufferCSB(float contrast, float saturation, float brightness, float gamma = 1.0f) // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    RQVector uniValues = RQVector{ contrast, saturation, brightness, 1.0f / g_fGamma };
    pForcedShader = g_pCSBShader;
    pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);
    
    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0, 0.0, 2.0f, 2.0f, 255, 255, 255, 255, pSkyGFXPostFXRaster1);

    pForcedShader = NULL;
    ImmediateModeRenderStatesReStore();

    GFX_GrabScreen(); // Update the framebuffer with CSB-processed image
}
void GFX_DepthBuffer() // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmax, umax, vmax, umax, vmin, umin, vmin);
    PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 255, 255, 255, 255, pSkyGFXDepthRaster);
    
    ImmediateModeRenderStatesReStore();
}

/* Hooks */
DECL_HOOKv(WaterCannons_Render)
{
    WaterCannons_Render();

    RenderWaterFog();
    RenderMovingFog();
    RenderVolumetricClouds();
}
DECL_HOOKv(PostFX_Init)
{
    if(pGrainRaster)
    {
        RwRasterDestroy(pGrainRaster);
        pGrainRaster = NULL;
    }
    pGrainRaster = RwRasterCreate(64, 64, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
    CreateGrainTexture(pGrainRaster);

    if(pDarkRaster)
    {
        RwRasterDestroy(pDarkRaster);
        pDarkRaster = NULL;
    }
    pDarkRaster = RwRasterCreate(64, 64, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
    CreatePlainTexture(pDarkRaster, CRGBA(0, 0, 0));

    *pbFog = false; // uninitialised variable
}
float gfWaterGreen = 0.0f;
CRGBA m_waterCol(64, 64, 64, 64);
DECL_HOOKv(PostFX_Render)
{
    GFX_GrabScreen();
    GFX_GrabDepth();
    if(g_bCSB)
    {
        // I got a request for this.
        GFX_FrameBufferCSB(g_fContrast, g_fSaturation, g_fBrightness, g_fGamma);
    }
    else
    {
        GFX_FrameBuffer();
    }

    if(*pbFog) RenderScreenFogPostEffect();

    PostFX_Render();

    if(!*pbInCutscene && g_nSpeedFX != SPFX_INACTIVE)
    {
        CAutomobile* veh = (CAutomobile*)FindPlayerVehicle(-1, false);
        if(veh && (veh->m_nVehicleType < VEHICLE_TYPE_HELI || veh->m_nVehicleType > VEHICLE_TYPE_TRAIN) )
        {
            float dirDot = DotProduct(veh->m_vecMoveSpeed, veh->m_matrix->m_forward);
        #ifdef AML32
            if(veh->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE && dirDot > 0.2f && (veh->m_nLocalFlags & 0x80000) != 0 && *(float*)( ((char*)veh) + 2232) < 0.0f )
        #else
            if(veh->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE && dirDot > 0.2f && (veh->hFlagsLocal & 0x80000) != 0 && veh->m_fTyreTemp < 0.0f )
        #endif
            {
                GFX_SpeedFX(2.0f * dirDot * (veh->m_fGasPedal + 1.0f));
            }
            else
            {
                GFX_SpeedFX(FindPlayerSpeed(-1)->Magnitude());
            }
        }
    }

    if(g_bRadiosity && !*m_bDarknessFilter)
    {
        GFX_Radiosity(m_CurrentColours->intensityLimit, 2, 1, 35);
    }

    if(g_bRenderGrain && pGrainRaster)
    {
        if(!*pbInCutscene)
        {
            if(*pbNightVision) RenderGrainEffect(GRAIN_NIGHTVISION_STRENGTH);
            else if(*pbInfraredVision) RenderGrainEffect(GRAIN_INFRAREDVISION_STRENGTH);
        }
        if(*pbRainEnable || g_nGrainRainStrength)
        {
            uint8_t nTargetStrength = (uint8_t)(*pfWeatherRain * 128.0f);
                 if(g_nGrainRainStrength < nTargetStrength) ++g_nGrainRainStrength;
            else if(g_nGrainRainStrength > nTargetStrength) --g_nGrainRainStrength;

            if(!CamNoRain() && !PlayerNoRain() && *pfWeatherUnderwaterness <= 0.0f && !*currArea)
            {
                CVector& cameraPos = TheCamera->GetPosition();
                if(cameraPos.z <= 900.0f) RenderGrainEffect(g_nGrainRainStrength / 4);
            }
        }
        if(*pbGrainEnable) RenderGrainEffect(*pnGrainStrength);
    }

    if(g_bHeatHaze)
    {
        if(*pfWeatherHeatHaze > 0.0f || (*m_bHeatHazeFX && *m_foundHeatHazeInfo) || *pfWeatherUnderwaterness >= 0.535f)
        {
            if(*m_bHeatHazeFX || *pfWeatherUnderwaterness >= 0.535f)
            {
                GFX_HeatHaze(1.0f, false);
            }
            else if(*pfWeatherHeatHaze <= 0.0f)
            {
                if(*m_foundHeatHazeInfo) GFX_HeatHaze(1.0f, true);
            }
            else
            {
                GFX_HeatHaze(*HeatHazeFXControl, false);
            }
        }
    }

    if(g_nUWR != UWR_INACTIVE)
    {
        if(*pfWeatherUnderwaterness >= 0.535f)
        {
            CRGBA underwaterCol;
            float waterColScale = 1.0f - fminf(*WaterDepth, 90.0f) / 90.0f;

            int col = m_waterCol.r + 184;
            underwaterCol.r = waterColScale * ((col > 255) ? 255 : col);
            col = m_waterCol.g + 184 + (int)gfWaterGreen;
            underwaterCol.g = waterColScale * ((col > 255) ? 255 : col);
            col = m_waterCol.b + 184;
            underwaterCol.b = waterColScale * ((col > 255) ? 255 : col);
            underwaterCol.a = m_waterCol.a; // replaced with 255 in ripple func

            if(g_nUWR == UWR_CLASSIC)
            {
                float xoffset = (4.0f * gfWaterGreen / 24.0f) * (((float)RsGlobal->maximumWidth) / 640.0f);
                float yoffset = 24.0f * (((float)RsGlobal->maximumHeight) / 448.0f);
                GFX_UnderWaterRipple(underwaterCol, xoffset, yoffset, 64, 0.0015f, 0.04f);
            }
            else if(g_nUWR == UWR_SHADER)
            {
                GFX_UnderWaterRipple_Shader(underwaterCol, gfWaterGreen / 24.0f, 0.0015f, 0.04f);
            }

            gfWaterGreen = fminf(gfWaterGreen + *ms_fTimeStep, 24.0f);
        }
        else
        {
            gfWaterGreen = 0.0f;
        }
    }

    // Enchanced PostFXs
    GFX_ActivateProcessedDepthTexture();
    if(g_nDOF != DOF_INACTIVE && *currArea == 0)
    {
        if(g_nDOF == DOF_ALWAYS || (g_nDOF == DOF_CUTSCENES && *pbInCutscene))
        {
            GFX_DOF();
        }
    }
    if(g_nCrAb != CRAB_INACTIVE) GFX_ChromaticAberration();
    //GFX_FakeRay(); // doesnt look cool enough
    GFX_Vignette(g_nVignette * 2.55f);

    GFX_DeActivateDepthTexture();
}
DECL_HOOKv(PostFX_CCTV)
{
    GFX_CCTV();
}
DECL_HOOKv(ShowGameBuffer)
{
    ShowGameBuffer();
    GFX_GrabScreen();
}
DECL_HOOKv(HeatHazeFX_GrabBuffer, void* a, int b, int c)
{
    GFX_GrabScreen();
}

/* Main */
void StartEffectsStuff()
{
    g_bFixSandstorm = cfg->GetBool("FixSandstormIntensity", g_bFixSandstorm, "Effects");
    g_bFixFog = cfg->GetBool("FixFogIntensity", g_bFixFog, "Effects");
    g_bExtendRainSplashes = cfg->GetBool("ExtendedRainSplashes", g_bExtendRainSplashes, "Effects");
    g_bMissingEffects = cfg->GetBool("MissingPCEffects", g_bMissingEffects, "Effects");
    g_bMissingPostEffects = cfg->GetBool("MissingPCPostEffects", g_bMissingPostEffects, "Effects");

    if(g_bFixSandstorm)
    {
      #ifdef AML32
        aml->Write16(pGTASA + 0x5CE16C, 0xE00C);
      #else
        aml->Write32(pGTASA + 0x6F26F8, 0x1400000A);
      #endif
    }

    if(g_bFixFog)
    {
        aml->PlaceNOP(pGTASA + BYBIT(0x5CDB90, 0x6F2204), 1);
    }

    if(g_bExtendRainSplashes)
    {
      #ifdef AML32
        aml->Write32(pGTASA + 0x5CD9E0, 0x0000E9CD);
      #else
        aml->Write32(pGTASA + 0x6F2070, 0x52800024);
      #endif
    }

    if(g_bMissingEffects)
    {
        HOOK(WaterCannons_Render, aml->GetSym(hGTASA, "_ZN13CWaterCannons6RenderEv"));
        aml->PlaceNOP4(pGTASA + BYBIT(0x5CCB98, 0x6F16B4), 1); // CWeather::WaterFogFXControl, dont mult by 1.4
        aml->WriteFloat(aml->GetSym(hGTASA, "_ZN11CWaterLevel17m_fWaterFogHeightE"), 7.0f); // lower the WaterFog max height
    }

    if(g_bMissingPostEffects)
    {
        HOOK(PostFX_Init,   aml->GetSym(hGTASA, "_ZN12CPostEffects21SetupBackBufferVertexEv"));
        HOOK(PostFX_Render, aml->GetSym(hGTASA, "_ZN12CPostEffects12MobileRenderEv"));
        HOOK(PostFX_CCTV,   aml->GetSym(hGTASA, "_ZN12CPostEffects4CCTVEv"));
        shadercreation += CreateEffectsShaders;

        //HOOKBLX(ShowGameBuffer, pGTASA + BYBIT(0x1BC5D4 + 0x1, 0x24F994));

        pCFGRenderGrain = cfg->Bind("RenderGrainEffect", g_bRenderGrain, "Effects");
        RenderGrainSettingChanged(g_bRenderGrain, pCFGRenderGrain->GetBool());
        AddSetting("Grain Effect", g_bRenderGrain, 0, sizeofA(aYesNo)-1, aYesNo, RenderGrainSettingChanged, NULL);

        pCFGSpeedFX = cfg->Bind("SpeedFXType", g_nSpeedFX, "Effects");
        SpeedFXSettingChanged(g_nSpeedFX, pCFGSpeedFX->GetInt());
        AddSetting("Speed FX", g_nSpeedFX, 0, sizeofA(aSpeedFXSettings)-1, aSpeedFXSettings, SpeedFXSettingChanged, NULL);

        pCFGUWR = cfg->Bind("UnderwaterRippleType", g_nUWR, "Effects");
        UWRSettingChanged(g_nUWR, pCFGUWR->GetInt());
        AddSetting("Underwater Ripple", g_nUWR, 0, sizeofA(aUWRSettings)-1, aUWRSettings, UWRSettingChanged, NULL);

        pCFGCrAbFX = cfg->Bind("ChromaticAberration", g_nCrAb, "EnchancedEffects");
        CrAbFXSettingChanged(g_nCrAb, pCFGCrAbFX->GetInt());
        AddSetting("Chromatic Aberration", g_nCrAb, 0, sizeofA(aCrAbFXSettings)-1, aCrAbFXSettings, CrAbFXSettingChanged, NULL);

        pCFGVignette = cfg->Bind("VignetteIntensity", g_nVignette, "EnchancedEffects");
        VignetteSettingChanged(g_nVignette, pCFGVignette->GetInt());
        AddSlider("Vignette Intensity", g_nVignette, 0, 100, VignetteSettingChanged, NULL, NULL);

        //pCFGRadiosity = cfg->Bind("Radiosity", g_bRadiosity, "Effects");
        //RadiositySettingChanged(g_bRadiosity, pCFGRadiosity->GetBool());
        //AddSetting("Radiosity", g_bRadiosity, 0, sizeofA(aYesNo)-1, aYesNo, RadiositySettingChanged, NULL);

        pCFGDOF = cfg->Bind("DOFType", g_nDOF, "EnchancedEffects");
        DOFSettingChanged(g_nDOF, pCFGDOF->GetInt());
        AddSetting("Depth'o'Field", g_nDOF, 0, sizeofA(aDOFSettings)-1, aDOFSettings, DOFSettingChanged, NULL);
        AddSetting("CSB Image Filter", g_bCSB, 0, sizeofA(aYesNo)-1, aYesNo, CSBSettingChanged, NULL);
        g_fDOFStrength = cfg->GetFloat("DOFStrength", g_fDOFStrength, "EnchancedEffects");
        g_bDOFUseScale = cfg->GetFloat("DOFUseScaleInsteadDist", g_bDOFUseScale, "EnchancedEffects");
        g_fDOFDistance = cfg->GetFloat("DOFDistance", g_fDOFDistance, "EnchancedEffects");

        pCFGCSB = cfg->Bind("CSBFilter", g_bCSB, "EnchancedEffects");
        CSBSettingChanged(g_bCSB, pCFGCSB->GetBool());
        AddSetting("CSB Image Filter", g_bCSB, 0, sizeofA(aYesNo)-1, aYesNo, CSBSettingChanged, NULL);
        g_fContrast = cfg->GetFloat("CSBFilter_Contrast", g_fContrast, "EnchancedEffects");
        g_fSaturation = cfg->GetFloat("CSBFilter_Saturation", g_fSaturation, "EnchancedEffects");
        g_fBrightness = cfg->GetFloat("CSBFilter_Brightness", g_fBrightness, "EnchancedEffects");
        g_fGamma = cfg->GetFloat("CSBFilter_Gamma", g_fGamma, "EnchancedEffects");
        

        if(g_bHeatHaze)
        {
            HOOKBLX(HeatHazeFX_GrabBuffer, pGTASA + BYBIT(0x5B51E6, 0x6D94EC));
        }
    }
}
