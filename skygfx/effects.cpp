#include <externs.h>
#include "include/renderqueue.h"
#include "include/neoWaterdrops.h"

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
bool g_bHeatHaze = true;
float g_fDOFStrength = 1.0f, g_fDOFDistance = 60.0f;
bool g_bDOFUseScale = false;
int g_nUWR = UWR_CLASSIC;
bool g_bCSB = false;
float g_fContrast = 1.0f, g_fSaturation = 1.0f, g_fBrightness = 1.0f, g_fGamma = 1.0f;
bool g_bBloom = false;
float g_fBloomIntensity = 0.55f;
bool g_bAutoExposure = false;
bool g_bFXAA = false;
bool g_bSSAO = false;

int g_nGrainRainStrength = 0;
int g_nInitialGrain = 0;
RwRaster* pGrainRaster = NULL;

int postfxX = 0, postfxY = 0;
float fpostfxX = 0, fpostfxY = 0;
float fpostfxXInv = 0, fpostfxYInv = 0;
RwRaster *pSkyGFXPostFXRaster1 = NULL, *pSkyGFXPostFXRaster2 = NULL,
         *pSkyGFXDepthRaster = NULL,   *pSkyGFXBrightnessRaster = NULL,
         *pSkyGFXBloomP1Raster = NULL, *pSkyGFXBloomP2Raster = NULL,
         *pSkyGFXBloomP3Raster = NULL,
         *pSkyGFXSceneBrightnessRaster = NULL, *pSkyGFXOcclusionRaster = NULL,
         *pSkyGFXOcclusionP1Raster = NULL, *pSkyGFXOcclusionP2Raster = NULL,
         *pSkyGFXOcclusionP3Raster = NULL, *pSkyGFXRadiosityRaster = NULL,
         *pSkyGFXNormalRaster = NULL;
RwRaster *pDarkRaster = NULL, *pNoiseRaster = NULL;

ES2Shader* g_pFramebufferRenderShader = NULL;
ES2Shader* g_pSimpleInverseShader = NULL;
ES2Shader* g_pSimpleDepthShader = NULL;
ES2Shader* g_pSimpleBrightShader = NULL;
ES2Shader* g_pChromaticAberrationShader = NULL;
ES2Shader* g_pChromaticAberrationShader_Intensive = NULL;
ES2Shader* g_pVignetteShader = NULL;
ES2Shader* g_pFakeRayShader = NULL;
ES2Shader* g_pDOFShader = NULL;
ES2Shader* g_pDADOFShader = NULL;
ES2Shader* g_pUnderwaterRippleShader = NULL;
ES2Shader* g_pCSBShader = NULL;
ES2Shader* g_pBloomP1Shader = NULL;
ES2Shader* g_pBloomP2Shader = NULL;
ES2Shader* g_pBloomShader = NULL;
ES2Shader* g_pFXAAShader = NULL; // FXAA v2.0 shader
ES2Shader* g_pOcclusionBufferShader = NULL;
ES2Shader* g_pSSAOShader = NULL;
ES2Shader* g_pDepthNormalMapShader = NULL;
ES2Shader* g_pRadiosityBlurShader = NULL;
ES2Shader* g_pRadiosityShader = NULL;

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
ConfigEntry *pCFGCSB;
ConfigEntry *pCFGBloom;
ConfigEntry *pCFGNeoWaterDrops;
ConfigEntry *pCFGNeoBloodDrops;
ConfigEntry *pCFGFXAA;

/* Functions */
void CreateEffectsShaders()
{
    char sFRPxl[] = "precision mediump float;\n"
                    "uniform sampler2D Diffuse;\n"
                    "varying mediump vec2 Out_Tex0;\n"
                    "void main() {\n"
                    "  gl_FragColor = texture2D(Diffuse, Out_Tex0);\n"
                    "}";
    char sFRVtx[] = "attribute vec3 Position;\n"
                    "attribute vec2 TexCoord0;\n"
                    "varying mediump vec2 Out_Tex0;\n"
                    "void main() {\n"
                    "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                    "  Out_Tex0 = TexCoord0;\n"
                    "}";
    g_pFramebufferRenderShader = CreateCustomShaderAlloc(0, sFRPxl, sFRVtx, sizeof(sFRPxl), sizeof(sFRVtx));
    
    char sSimpleDepthPxl[] = "precision highp float;\n"
                             "uniform highp sampler2D DepthTex;\n"
                             "varying highp vec2 Out_Tex0;\n"
                             "varying highp vec3 FarNearLogic;\n"
                             "void main() {\n"
                             "  float depth = 2.0 * texture2D(DepthTex, Out_Tex0).r - 1.0;\n"
                             "  depth = FarNearLogic.x / (FarNearLogic.y - depth * FarNearLogic.z);\n"
                             "  gl_FragColor = vec4(vec3(1.0 - depth), 1.0);\n"
                             "}";
    char sSimpleDepthVtx[] = "precision highp float;\n"
                             "attribute vec3 Position;\n"
                             "attribute vec2 TexCoord0;\n"
                             "varying highp vec2 Out_Tex0;\n"
                             "uniform highp vec4 GFX1v;\n"
                             "varying highp vec3 FarNearLogic;\n"
                             "void main() {\n"
                             "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                             "  Out_Tex0 = TexCoord0;\n"
                             "  float zNear = GFX1v.x;\n"
                             "  float zFar = GFX1v.y;\n"
                             "  FarNearLogic = vec3(2.0 * zNear, zFar + zNear, zFar - zNear);\n"
                             "}";
    g_pSimpleDepthShader = CreateCustomShaderAlloc(0, sSimpleDepthPxl, sSimpleDepthVtx, sizeof(sSimpleDepthPxl), sizeof(sSimpleDepthVtx));

    char sSimpleBrightPxl[] = "precision mediump float;\n"
                              "uniform sampler2D Diffuse;\n"
                              "varying mediump vec2 Out_Tex0;\n"
                              "uniform mediump vec4 GFX1v;\n"
                              "varying mediump float CalculatedScaler;\n"
                              "const vec3 LumCoeff = vec3(0.2126, 0.7152, 0.0722);\n"
                              "void main() {\n"
                              "  vec3 color = texture2D(Diffuse, Out_Tex0).rgb;\n"
                              "  float intens = dot(color, LumCoeff);\n"
                              "  float intensScaled = max(0.0, exp((intens - 1.0) * 2.2) - GFX1v.x) * CalculatedScaler;\n"
                              "  gl_FragColor = vec4(color * intensScaled, 1.0);\n"
                              "}";
    char sSimpleBrightVtx[] = "precision highp float;\n"
                              "attribute vec3 Position;\n"
                              "attribute vec2 TexCoord0;\n"
                              "varying mediump vec2 Out_Tex0;\n"
                              "uniform mediump vec4 GFX1v;\n"
                              "varying mediump float CalculatedScaler;\n"
                              "void main() {\n"
                              "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                              "  Out_Tex0 = TexCoord0;\n"
                              "  CalculatedScaler = 1.0 / (1.0 - GFX1v.x);\n"
                              "}";
    g_pSimpleBrightShader = CreateCustomShaderAlloc(0, sSimpleBrightPxl, sSimpleBrightVtx, sizeof(sSimpleBrightPxl), sizeof(sSimpleBrightVtx));

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
                          "uniform mediump vec4 GFX1v;\n"
                          "const vec2 centerVec = vec2(0.5, 0.5);\n"
                          "float vignette(vec2 uv, float radius, float smoothness) {\n"
                          "  float diff = radius - distance(uv, centerVec);\n"
                          "  return smoothstep(-smoothness, smoothness, diff);\n"
                          "}\n"
                          "void main() {\n"
                          "  float vignetteValue = 1.0 - vignette(Out_Tex0, 0.25, 0.5);\n"
                          "  gl_FragColor = texture2D(Diffuse, Out_Tex0) * vignetteValue;\n"
                          "  gl_FragColor.a *= GFX1v.x;\n"
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
                         "uniform highp sampler2D DepthTex;\n"
                         "varying mediump vec2 Out_Tex0;\n"
                         "varying mediump vec2 vPos;\n"
                         "const vec3 LumCoeff = vec3(0.2126, 0.7152, 0.0722);\n"
                         "void main() {\n"
                         "  vec2 tex = Out_Tex0 - 0.5;\n"
                         "  float pz = 0.0;\n"
                         "  for(float i = 0.0; i < 50.0; i++) {\n"
                         "    vec3 shiftColor = texture2D(Diffuse, (tex *= 0.99) + 0.5).rgb;\n"
                         "    float depth = texture2D(DepthTex, tex + 0.5).r;\n"
                         "    if(depth < 0.01) {\n"
                         "      float pixelBrightness = dot(shiftColor, LumCoeff);\n"
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
                     "uniform highp sampler2D DepthTex;\n"
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
                       "uniform highp sampler2D DepthTex;\n"
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

    char sUWRPxl[] = "precision highp float;\n"
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
                     "const vec3 LumCoeff = vec3(0.2126, 0.7152, 0.0722);\n"
                     "const vec3 AvgLumin = vec3(0.5, 0.5, 0.5);\n"
                     "void main() {\n"
                     "  vec3 brtColor  = texture2D(Diffuse, Out_Tex0).rgb * vec3(GFX1v.z);\n"
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

    char sBloomVtx[] = "precision highp float;\n"
                       "attribute vec3 Position;\n"
                       "attribute vec2 TexCoord0;\n"
                       "varying mediump vec2 Out_Tex0;\n"
                       "varying highp float WeightGauss[5];\n"
                       "void main() {\n"
                       "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                       "  Out_Tex0 = TexCoord0;\n"
                       "  WeightGauss[0] = 0.2270270270;\n"
                       "  WeightGauss[1] = 0.1945945946;\n"
                       "  WeightGauss[2] = 0.1216216216;\n"
                       "  WeightGauss[3] = 0.0540540541;\n"
                       "  WeightGauss[4] = 0.0162162162;\n"
                       "}";
    char sBloom1Pxl[] = "precision mediump float;\n"
                        "uniform sampler2D Diffuse;\n"
                        "uniform mediump vec4 GFX1v;\n"
                        "varying mediump vec2 Out_Tex0;\n"
                        "varying highp float WeightGauss[5];\n"
                        "void main() {\n"
                        "  vec3 color = texture2D(Diffuse, Out_Tex0).rgb * WeightGauss[0];\n"
                        "  for (int i = 1; i < 5; ++i) {\n"
                        "    float axisV = GFX1v.z * float(i);\n"
                        "    color += texture2D(Diffuse, Out_Tex0 + GFX1v.xy * vec2(axisV, 0.0)).rgb * WeightGauss[i];\n"
                        "    color += texture2D(Diffuse, Out_Tex0 - GFX1v.xy * vec2(axisV, 0.0)).rgb * WeightGauss[i];\n"
                        "  }\n"
                        "  gl_FragColor = vec4(color, 1.0);\n"
                        "}";
    g_pBloomP1Shader = CreateCustomShaderAlloc(0, sBloom1Pxl, sBloomVtx, sizeof(sBloom1Pxl), sizeof(sBloomVtx)); // Horizontal blur

    char sBloom2Pxl[] = "precision mediump float;\n"
                        "uniform sampler2D Diffuse;\n"
                        "uniform mediump vec4 GFX1v;\n"
                        "varying mediump vec2 Out_Tex0;\n"
                        "varying highp float WeightGauss[5];\n"
                        "void main() {\n"
                        "  vec3 color = texture2D(Diffuse, Out_Tex0).rgb * WeightGauss[0];\n"
                        "  for (int i = 1; i < 5; ++i) {\n"
                        "    float axisV = GFX1v.z * float(i);\n"
                        "    color += texture2D(Diffuse, Out_Tex0 + GFX1v.xy * vec2(0.0, axisV)).rgb * WeightGauss[i];\n"
                        "    color += texture2D(Diffuse, Out_Tex0 - GFX1v.xy * vec2(0.0, axisV)).rgb * WeightGauss[i];\n"
                        "  }\n"
                        "  gl_FragColor = vec4(color, 1.0);\n"
                        "}";
    g_pBloomP2Shader = CreateCustomShaderAlloc(0, sBloom2Pxl, sBloomVtx, sizeof(sBloom2Pxl), sizeof(sBloomVtx)); // Vertical blur

    char sBloomPxl[] = "precision mediump float;\n"
                       "uniform sampler2D Diffuse;\n"
                       "uniform sampler2D BrightBuf;\n"
                       "uniform mediump vec4 GFX1v;\n"
                       "varying mediump vec2 Out_Tex0;\n"
                       "void main() {\n"
                       "  vec3 color = texture2D(Diffuse, Out_Tex0).rgb;\n"
                       "  vec3 bloom = GFX1v.z * texture2D(BrightBuf, Out_Tex0).rgb;\n"
                       "  gl_FragColor = vec4(color + bloom, 1.0);\n"
                       "}";
    g_pBloomShader = CreateCustomShaderAlloc(0, sBloomPxl, sBloomVtx, sizeof(sBloomPxl), sizeof(sBloomVtx));

    char sFXAAPxl[] = "precision mediump float;\n"
                      "uniform sampler2D Diffuse;\n"
                      "varying mediump vec2 Out_Tex0;\n"
                      "uniform mediump vec4 GFX1v;\n"
                      "varying mediump vec2 TexelNegatedAxis;\n"
                      "const vec3 LumCoeff = vec3(0.2126, 0.7152, 0.0722);\n"
                      "mediump vec3 rgbNW;\n"
                      "mediump vec3 rgbNE;\n"
                      "mediump vec3 rgbSW;\n"
                      "mediump vec3 rgbSE;\n"
                      "mediump vec3 rgbM;\n"
                      "const float FXAA_SPAN_MAX = 8.0;\n"
                      "const float FXAA_REDUCE_MUL = 1.0 / 8.0;\n"
                      "const float FXAA_REDUCE_MIN = 1.0 / 128.0;\n"
                      "\n"
                      "void main() {\n"
                      "  rgbNW = texture2D(Diffuse, Out_Tex0 - GFX1v.xy).rgb;\n"
                      "  rgbNE = texture2D(Diffuse, Out_Tex0 - TexelNegatedAxis).rgb;\n"
                      "  rgbSW = texture2D(Diffuse, Out_Tex0 + TexelNegatedAxis).rgb;\n"
                      "  rgbSE = texture2D(Diffuse, Out_Tex0 + GFX1v.xy).rgb;\n"
                      "  rgbM  = texture2D(Diffuse, Out_Tex0).rgb;\n"
                      "  float lumaNW = dot(rgbNW, LumCoeff);\n"
                      "  float lumaNE = dot(rgbNE, LumCoeff);\n"
                      "  float lumaSW = dot(rgbSW, LumCoeff);\n"
                      "  float lumaSE = dot(rgbSE, LumCoeff);\n"
                      "  float lumaM  = dot(rgbM,  LumCoeff);\n"
                      "  float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));\n"
                      "  float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));\n"
                      "  vec2 dir;\n"
                      "  dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));\n"
                      "  dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));\n"
                      "  float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);\n"
                      "  float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);\n"
                      "  dir = min(vec2(FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX), dir * rcpDirMin)) * GFX1v.xy;\n"
                      "  vec3 rgbA = 0.5 * (texture2D(Diffuse, Out_Tex0 - dir * 0.1666667).rgb + texture2D(Diffuse, Out_Tex0 + dir * 0.1666667).rgb);\n"
                      "  vec3 rgbB = 0.5 * rgbA + 0.25 * (texture2D(Diffuse, Out_Tex0 - dir * 0.5).rgb + texture2D(Diffuse, Out_Tex0 + dir * 0.5).rgb);\n"
                      "  float lumaB = dot(rgbB, LumCoeff);\n"
                      "  gl_FragColor = vec4((lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB, 1.0);\n"
                      "}";
    char sFXAAVtx[] = "precision highp float;\n"
                      "attribute vec3 Position;\n"
                      "attribute vec2 TexCoord0;\n"
                      "varying mediump vec2 Out_Tex0;\n"
                      "uniform mediump vec4 GFX1v;\n"
                      "varying mediump vec2 TexelNegatedAxis;\n"
                      "void main() {\n"
                      "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                      "  Out_Tex0 = TexCoord0;\n"
                      "  TexelNegatedAxis = vec2(-GFX1v.x, GFX1v.y);\n"
                      "}";
    g_pFXAAShader = CreateCustomShaderAlloc(0, sFXAAPxl, sFXAAVtx, sizeof(sFXAAPxl), sizeof(sFXAAVtx));

    //char sOBPxl[] = "precision highp float;\n"
    //                "uniform highp sampler2D Diffuse;\n"
    //                "uniform highp sampler2D NormalBuf;\n"
    //                "uniform highp sampler2D DepthTex;\n"
    //                "varying highp vec2 Out_Tex0;\n"
    //                "uniform highp vec4 GFX1v;\n"
    //                "varying highp vec3 SampleSphere[16];\n"
    //                "uniform mat4 ProjMatrix;\n"
    //                "varying mat4 ProjMatrixInv;\n"
    //                "vec3 getViewPos(vec2 uv) {\n"
    //                "  float linearDepth = 1.0 - texture2D(DepthTex, uv).r;\n"
    //                "  vec2 ndc = uv * 2.0 - 1.0;\n"
    //                "  vec4 clipPos = vec4(ndc, linearDepth, 1.0);\n"
    //                "  vec4 viewPos = ProjMatrixInv * clipPos;\n"
    //                "  viewPos /= viewPos.w;\n"
    //                "  return viewPos.xyz;\n"
    //                "}\n"
    //                "void main() {\n"
    //                "  vec3 normal = texture2D(NormalBuf, Out_Tex0).rgb;\n"
    //                "  normal = normalize(normal * 2.0 - 1.0);\n"
    //                "  vec3 fragPos = getViewPos(Out_Tex0);\n"
    //                "  vec3 randomVec = texture2D(Diffuse, Out_Tex0 * GFX1v.xy).xyz;\n"
    //                "  randomVec = normalize(randomVec * 2.0 - 1.0);\n"
    //                "  vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));\n"
    //                "  vec3 bitangent = cross(normal, tangent);\n"
    //                "  mat3 TBN = mat3(tangent, bitangent, normal);\n"
    //                "  float occlusion = 0.0;\n"
    //                "  for (int i = 0; i < 16; ++i) {\n"
    //                "    vec3 sampleVec = TBN * SampleSphere[i];\n"
    //                "    sampleVec = fragPos + sampleVec * GFX1v.z;\n"
    //                "    vec4 offset = ProjMatrix * vec4(sampleVec, 1.0);\n"
    //                "    offset.xyz /= offset.w;\n"
    //                "    vec2 sampleUV = offset.xy * 0.5 + 0.5;\n"
    //                "    float sampleDepth = 1.0 - texture2D(DepthTex, sampleUV).r;\n"
    //                "    float rangeCheck = smoothstep(0.0, 1.0, GFX1v.z / abs(fragPos.z - sampleDepth));\n"
    //                "    if ((sampleDepth + GFX1v.w) < sampleVec.z) { occlusion += rangeCheck; }\n"
    //                "  }\n"
    //                "  occlusion = 1.0 - (occlusion / 16.0);\n"
    //                "  gl_FragColor = vec4(vec3(occlusion), 1.0);\n"
    //                "}";
    char sOBPxl[] = "precision highp float;\n"
                    "uniform highp sampler2D Diffuse;\n"
                    "uniform highp sampler2D DepthTex;\n"
                    "varying highp vec2 Out_Tex0;\n"
                    "uniform highp vec4 GFX1v;\n"
                    "varying highp vec3 SampleSphere[16];\n"
                    "const float ssaoStrength = 1.2;\n"
                    "const float ssaoBase = 0.1;\n"
                    "const float ssaoArea = 0.03;\n"
                    "const float ssaoFalloff = 0.000001;\n"
                    "const float ssaoRadius = 0.004;\n"
                    "vec3 getNormalFromDepth(float depth, vec2 uv) {\n"
                    "  vec2 offset1 = vec2(0.0, GFX1v.w);\n"
                    "  vec2 offset2 = vec2(GFX1v.z, 0.0);\n"
                    "  float depth1 = texture2D(DepthTex, uv + offset1).r;\n"
                    "  float depth2 = texture2D(DepthTex, uv + offset2).r;\n"
                    "  vec3 p1 = vec3(offset1, depth1 - depth);\n"
                    "  vec3 p2 = vec3(offset2, depth2 - depth);\n"
                    "  vec3 normal = cross(p1, p2);\n"
                    "  normal.z = -normal.z;\n"
                    "  return normalize(normal);\n"
                    "}\n"
                    "void main() {\n"
                    "  highp float depth = texture2D(DepthTex, Out_Tex0).r;\n"
                    "  if(depth > 1.0) {\n"
                    "    gl_FragColor = vec4(vec3(ssaoBase), 1.0);\n"
                    "    return;\n"
                    "  }\n"
                    "  vec3 random = normalize(texture2D(Diffuse, Out_Tex0 * GFX1v.xy).rgb);\n"
                    "  vec3 position = vec3(Out_Tex0, depth);\n"
                    "  vec3 normal = getNormalFromDepth(depth, Out_Tex0);\n"
                    "  float radiusDepth = ssaoRadius / depth;\n"
                    "  float occlusion = 0.0;\n"
                    "  for(int i = 0; i < 16; i++) {\n"
                    "    vec3 ray = radiusDepth * reflect(SampleSphere[i], random);\n"
                    "    vec3 hemiRay = position + sign(dot(ray, normal)) * ray;\n"
                    "    float occDepth = texture2D(DepthTex, clamp(hemiRay.xy, 0.0, 1.0)).r;\n"
                    "    float difference = depth - occDepth;\n"
                    "    occlusion += step(ssaoFalloff, difference) * (1.0 - smoothstep(ssaoFalloff, ssaoArea, difference));\n"
                    "  }\n"
                    "  occlusion = 1.0 - ssaoStrength * (occlusion / 16.0);\n"
                    "  gl_FragColor = vec4(clamp(occlusion + ssaoBase, 0.0, 1.0));\n"
                    "}";
    char sOBVtx[] = "precision highp float;\n"
                    "attribute vec3 Position;\n"
                    "attribute vec2 TexCoord0;\n"
                    "varying highp vec2 Out_Tex0;\n"
                    "uniform highp vec4 GFX1v;\n"
                    "varying highp vec3 SampleSphere[16];\n"
                    "uniform mat4 ProjMatrix;\n"
                    "varying mat4 ProjMatrixInv;\n"
                    "mat4 inverseMat4(mat4 m) {\n"
                    "  float a00 = m[0][0], a01 = m[0][1], a02 = m[0][2], a03 = m[0][3], a10 = m[1][0], a11 = m[1][1], a12 = m[1][2], a13 = m[1][3],\n"
                    "  a20 = m[2][0], a21 = m[2][1], a22 = m[2][2], a23 = m[2][3], a30 = m[3][0], a31 = m[3][1], a32 = m[3][2], a33 = m[3][3];\n"
                    "  float b00 = a00 * a11 - a01 * a10, b01 = a00 * a12 - a02 * a10, b02 = a00 * a13 - a03 * a10, b03 = a01 * a12 - a02 * a11,\n"
                    "  b04 = a01 * a13 - a03 * a11, b05 = a02 * a13 - a03 * a12, b06 = a20 * a31 - a21 * a30, b07 = a20 * a32 - a22 * a30,\n"
                    "  b08 = a20 * a33 - a23 * a30, b09 = a21 * a32 - a22 * a31, b10 = a21 * a33 - a23 * a31, b11 = a22 * a33 - a23 * a32;\n"
                    "  float det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;\n"
                    "  return mat4(a11 * b11 - a12 * b10 + a13 * b09, a02 * b10 - a01 * b11 - a03 * b09, a31 * b05 - a32 * b04 + a33 * b03,\n"
                    "              a22 * b04 - a21 * b05 - a23 * b03, a12 * b08 - a10 * b11 - a13 * b07, a00 * b11 - a02 * b08 + a03 * b07,\n"
                    "              a32 * b02 - a30 * b05 - a33 * b01, a20 * b05 - a22 * b02 + a23 * b01, a10 * b10 - a11 * b08 + a13 * b06,\n"
                    "              a01 * b08 - a00 * b10 - a03 * b06, a30 * b04 - a31 * b02 + a33 * b00, a21 * b02 - a20 * b04 - a23 * b00,\n"
                    "              a11 * b07 - a10 * b09 - a12 * b06, a00 * b09 - a01 * b07 + a02 * b06, a31 * b01 - a30 * b03 - a32 * b00,\n"
                    "              a20 * b03 - a21 * b01 + a22 * b00) / det;\n"
                    "}\n"
                    "void main() {\n"
                    "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                    "  Out_Tex0 = TexCoord0;\n"
                    "  SampleSphere[0]  = vec3( 0.5381,  0.1856, -0.4319);\n"
                    "  SampleSphere[1]  = vec3( 0.1379,  0.2486,  0.4430);\n"
                    "  SampleSphere[2]  = vec3( 0.3371,  0.5679, -0.0057);\n"
                    "  SampleSphere[3]  = vec3(-0.6999, -0.0451, -0.0019);\n"
                    "  SampleSphere[4]  = vec3( 0.0689, -0.1598, -0.8547);\n"
                    "  SampleSphere[5]  = vec3( 0.0560,  0.0069, -0.1843);\n"
                    "  SampleSphere[6]  = vec3(-0.0146,  0.1402,  0.0762);\n"
                    "  SampleSphere[7]  = vec3( 0.0100, -0.1924, -0.0344);\n"
                    "  SampleSphere[8]  = vec3(-0.3577, -0.5301, -0.4358);\n"
                    "  SampleSphere[9]  = vec3(-0.3169,  0.1063,  0.0158);\n"
                    "  SampleSphere[10] = vec3( 0.0103, -0.5869,  0.0046);\n"
                    "  SampleSphere[11] = vec3(-0.0897, -0.4940,  0.3287);\n"
                    "  SampleSphere[12] = vec3( 0.7119, -0.0154, -0.0918);\n"
                    "  SampleSphere[13] = vec3(-0.0533,  0.0596, -0.5411);\n"
                    "  SampleSphere[14] = vec3( 0.0352, -0.0631,  0.5460);\n"
                    "  SampleSphere[15] = vec3(-0.4776,  0.2847, -0.0271);\n"
                    "  //ProjMatrixInv = inverseMat4(ProjMatrix);\n"
                    "}";
    g_pOcclusionBufferShader = CreateCustomShaderAlloc(0, sOBPxl, sOBVtx, sizeof(sOBPxl), sizeof(sOBVtx));

    char sDNMPxl[] = "precision highp float;\n"
                     "uniform sampler2D Diffuse;\n"
                     "uniform highp sampler2D DepthTex;\n"
                     "varying highp vec2 Out_Tex0;\n"
                     "uniform highp vec4 GFX1v;\n"
                     "const float diffThreshold = 0.0022;\n"
                     "void main() {\n"
                     "  float depthC = texture2D(DepthTex, Out_Tex0).r;\n"
                     "  float depthL = texture2D(DepthTex, Out_Tex0 + vec2(-GFX1v.x, 0.0)).r;\n"
                     "  float depthR = texture2D(DepthTex, Out_Tex0 + vec2(GFX1v.x, 0.0)).r;\n"
                     "  float depthU = texture2D(DepthTex, Out_Tex0 + vec2(0.0, -GFX1v.y)).r;\n"
                     "  float depthD = texture2D(DepthTex, Out_Tex0 + vec2(0.0, GFX1v.y)).r;\n"
                     "  if (abs(depthC - depthL) > diffThreshold) depthL = depthC;\n"
                     "  if (abs(depthC - depthR) > diffThreshold) depthR = depthC;\n"
                     "  if (abs(depthC - depthU) > diffThreshold) depthU = depthC;\n"
                     "  if (abs(depthC - depthD) > diffThreshold) depthD = depthC;\n"
                     "  vec3 pC = vec3(Out_Tex0, depthC);\n"
                     "  vec3 pL = vec3(Out_Tex0.x - GFX1v.x, Out_Tex0.y, depthL);\n"
                     "  vec3 pR = vec3(Out_Tex0.x + GFX1v.x, Out_Tex0.y, depthR);\n"
                     "  vec3 pU = vec3(Out_Tex0.x, Out_Tex0.y - GFX1v.y, depthU);\n"
                     "  vec3 pD = vec3(Out_Tex0.x, Out_Tex0.y + GFX1v.y, depthD);\n"
                     "  vec3 tangentX = pR - pL;\n"
                     "  vec3 tangentY = pD - pU;\n"
                     "  vec3 normal = normalize(cross(tangentY, tangentX));\n"
                     "  normal = normal * 0.5 + 0.5;\n"
                     "  gl_FragColor = vec4(normal, 1.0);\n"
                     "}";
    char sDNMVtx[] = "precision highp float;\n"
                     "attribute vec3 Position;\n"
                     "attribute vec2 TexCoord0;\n"
                     "varying highp vec2 Out_Tex0;\n"
                     "void main() {\n"
                     "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                     "  Out_Tex0 = TexCoord0;\n"
                     "}";
    g_pDepthNormalMapShader = CreateCustomShaderAlloc(0, sDNMPxl, sDNMVtx, sizeof(sDNMPxl), sizeof(sDNMVtx));

    char sSSAOPxl[] = "precision mediump float;\n"
                      "uniform sampler2D Diffuse;\n"
                      "uniform sampler2D BrightBuf;\n"
                      "varying mediump vec2 Out_Tex0;\n"
                      "void main() {\n"
                      "  vec3 color = texture2D(Diffuse, Out_Tex0).rgb;\n"
                      "  vec3 occ = texture2D(BrightBuf, Out_Tex0).rgb;\n"
                      "  gl_FragColor = vec4(color * occ, 1.0);\n"
                      "}";
    char sSSAOVtx[] = "precision highp float;\n"
                      "attribute vec3 Position;\n"
                      "attribute vec2 TexCoord0;\n"
                      "varying highp vec2 Out_Tex0;\n"
                      "void main() {\n"
                      "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                      "  Out_Tex0 = TexCoord0;\n"
                      "}";
    g_pSSAOShader = CreateCustomShaderAlloc(0, sSSAOPxl, sSSAOVtx, sizeof(sSSAOPxl), sizeof(sSSAOVtx));

    char sRBlurPxl[] = "precision mediump float;\n"
                       "uniform sampler2D Diffuse;\n"
                       "varying mediump vec2 Out_Tex0;\n"
                       "uniform highp vec4 GFX1v;\n"
                       "void main() {\n"
                       "  vec3 c = vec3(0.0);\n"
                       "  for(float i = 0.0; i < 10.0; ++i) {\n"
                       "    vec2 uv = Out_Tex0 + GFX1v.xy * (i / 9.0 - 0.5) * GFX1v.z;\n"
                       "    c += texture2D(Diffuse, uv).rgb;\n"
                       "  }\n"
                       "  gl_FragColor = vec4(c / 10.0, 1.0);\n"
                       "}";
    char sRBlurVtx[] = "precision highp float;\n"
                       "attribute vec3 Position;\n"
                       "attribute vec2 TexCoord0;\n"
                       "varying highp vec2 Out_Tex0;\n"
                       "void main() {\n"
                       "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                       "  Out_Tex0 = TexCoord0;\n"
                       "}";
    g_pRadiosityBlurShader = CreateCustomShaderAlloc(0, sRBlurPxl, sRBlurVtx, sizeof(sRBlurPxl), sizeof(sRBlurVtx));

    char sRadioPxl[] = "precision mediump float;\n"
                       "uniform sampler2D Diffuse;\n"
                       "varying mediump vec2 Out_Tex0;\n"
                       "uniform highp vec4 GFX1v;\n"
                       "uniform highp vec4 GFX2v;\n"
                       "void main() {\n"
                       "  vec2 uv = Out_Tex0;\n"
                       "  //uv = uv * GFX2v.zw + GFX2v.xy;\n"
                       "  vec3 c = texture2D(Diffuse, uv).rgb;\n"
                       "  c = clamp(c * 2.0 - vec3(1.0) * GFX1v.x, 0.0, 1.0) * GFX1v.y * GFX1v.z;\n"
                       "  gl_FragColor = vec4(c, 1.0);\n"
                       "}";
    char sRadioVtx[] = "precision highp float;\n"
                       "attribute vec3 Position;\n"
                       "attribute vec2 TexCoord0;\n"
                       "varying highp vec2 Out_Tex0;\n"
                       "void main() {\n"
                       "  gl_Position = vec4(Position.xy - 1.0, 0.0, 1.0);\n"
                       "  Out_Tex0 = TexCoord0;\n"
                       "}";
    g_pRadiosityShader = CreateCustomShaderAlloc(0, sRadioPxl, sRadioVtx, sizeof(sRadioPxl), sizeof(sRadioVtx));

    char sInvrsPxl[] = "precision mediump float;\n"
                       "uniform sampler2D Diffuse;\n"
                       "varying mediump vec2 Out_Tex0;\n"
                       "void main() {\n"
                       "  gl_FragColor = texture2D(Diffuse, Out_Tex0);\n"
                       "}";
    char sInvrsVtx[] = "precision highp float;\n"
                       "uniform mat4 ProjMatrix;\n"
                       "uniform mat4 ViewMatrix;\n"
                       "uniform mat4 ObjMatrix;\n"
                       "attribute vec3 Position;\n"
                       "attribute vec2 TexCoord0;\n"
                       "varying highp vec2 Out_Tex0;\n"
                       "void main() {\n"
                       "  vec4 WorldPos = ObjMatrix * vec4(Position,1.0);\n"
                       "  vec4 ViewPos = ViewMatrix * WorldPos;\n"
                       "  gl_Position = ProjMatrix * ViewPos;\n"
                       "  Out_Tex0 = vec2(TexCoord0.x, 1.0 - TexCoord0.y);\n"
                       "}";
    g_pSimpleInverseShader = CreateCustomShaderAlloc(0, sInvrsPxl, sInvrsVtx, sizeof(sInvrsPxl), sizeof(sInvrsVtx));
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
void BloomSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGBloom->SetBool(newVal != 0);
    g_bBloom = pCFGBloom->GetBool();

    cfg->Save();
}
void NEOWaterDropsSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGNeoWaterDrops->SetBool(newVal != 0);
    WaterDrops::neoWaterDrops = pCFGNeoWaterDrops->GetBool();

    cfg->Save();
}
void NEOBloodDropsSettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGNeoBloodDrops->SetBool(newVal != 0);
    WaterDrops::neoBloodDrops = pCFGNeoBloodDrops->GetBool();

    cfg->Save();
}
void FXAASettingChanged(int oldVal, int newVal, void* data = NULL)
{
    if(oldVal == newVal) return;

    pCFGFXAA->SetBool(newVal != 0);
    g_bFXAA = pCFGFXAA->GetBool();

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
void CreateRandomTexture(RwRaster* target)
{
    if(!target) return;

    RwUInt8 *pixels = RwRasterLock(target, 0, 1);
    int pixSize = target->width * target->height;
    for(int i = 0; i < pixSize; ++i)
    {
        *(pixels++) = (rand() % 64);
        *(pixels++) = (rand() % 64);
        *(pixels++) = (rand() % 64);
        *(pixels++) = 255;
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
    ES2Texture* depthTexture = GetES2Raster(pSkyGFXDepthRaster);
    ERQ_SetActiveTexture(2, depthTexture->texID);
}
void GFX_ActivateFX1Texture()
{
    ES2Texture* brTexture = GetES2Raster(pSkyGFXPostFXRaster1);
    ERQ_SetActiveTexture(2, brTexture->texID);
}
void GFX_ActivateFX2Texture()
{
    ES2Texture* brTexture = GetES2Raster(pSkyGFXPostFXRaster2);
    ERQ_SetActiveTexture(2, brTexture->texID);
}
void GFX_ActivateFinalBloomTexture()
{
    ES2Texture* brTexture = GetES2Raster(pSkyGFXBloomP3Raster);
    ERQ_SetActiveTexture(2, brTexture->texID);
}
void GFX_ActivateFinalOcclusionTexture()
{
    ES2Texture* brTexture = GetES2Raster(pSkyGFXOcclusionP3Raster);
    ERQ_SetActiveTexture(2, brTexture->texID);
}
void GFX_DeActivateTexture(int tex = 2)
{
    ERQ_SetActiveTexture(tex, 0);
}
void GFX_ActivateNormalBufferTexture()
{
    ES2Texture* depthTexture = GetES2Raster(pSkyGFXNormalRaster);
    ERQ_SetActiveTexture(1, depthTexture->texID);
}
void GFX_CheckBuffersSize()
{
    if(postfxX != *renderWidth || postfxY != *renderHeight)
    {
        postfxX = *renderWidth;
        postfxY = *renderHeight;

        fpostfxX = postfxX;
        fpostfxY = postfxY;

        fpostfxXInv = 1.0f / fpostfxX;
        fpostfxYInv = 1.0f / fpostfxY;

        if(pSkyGFXPostFXRaster1) RwRasterDestroy(pSkyGFXPostFXRaster1);
        pSkyGFXPostFXRaster1 = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);

        if(pSkyGFXPostFXRaster2) RwRasterDestroy(pSkyGFXPostFXRaster2);
        pSkyGFXPostFXRaster2 = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);

        if(pSkyGFXDepthRaster) RwRasterDestroy(pSkyGFXDepthRaster);
        pSkyGFXDepthRaster = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);

        if(pSkyGFXBrightnessRaster) RwRasterDestroy(pSkyGFXBrightnessRaster);
        pSkyGFXBrightnessRaster = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);

        if(pSkyGFXBloomP1Raster) RwRasterDestroy(pSkyGFXBloomP1Raster);
        pSkyGFXBloomP1Raster = RwRasterCreate(0.5f * fpostfxX, 0.5f * fpostfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);

        if(pSkyGFXBloomP2Raster) RwRasterDestroy(pSkyGFXBloomP2Raster);
        pSkyGFXBloomP2Raster = RwRasterCreate(0.25f * fpostfxX, 0.25f * fpostfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);

        if(pSkyGFXBloomP3Raster) RwRasterDestroy(pSkyGFXBloomP3Raster);
        pSkyGFXBloomP3Raster = RwRasterCreate(0.125f * fpostfxX, 0.125f * fpostfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);

        /*if(pSkyGFXOcclusionRaster) RwRasterDestroy(pSkyGFXOcclusionRaster);
        pSkyGFXOcclusionRaster = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);

        if(pSkyGFXOcclusionP1Raster) RwRasterDestroy(pSkyGFXOcclusionP1Raster);
        pSkyGFXOcclusionP1Raster = RwRasterCreate(0.5f * fpostfxX, 0.5f * fpostfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);

        if(pSkyGFXOcclusionP2Raster) RwRasterDestroy(pSkyGFXOcclusionP2Raster);
        pSkyGFXOcclusionP2Raster = RwRasterCreate(0.25f * fpostfxX, 0.25f * fpostfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);

        if(pSkyGFXOcclusionP3Raster) RwRasterDestroy(pSkyGFXOcclusionP3Raster);
        pSkyGFXOcclusionP3Raster = RwRasterCreate(0.25f * fpostfxX, 0.25f * fpostfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);*/

        if(pSkyGFXRadiosityRaster) RwRasterDestroy(pSkyGFXRadiosityRaster);
        pSkyGFXRadiosityRaster = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);

        if(pSkyGFXNormalRaster) RwRasterDestroy(pSkyGFXNormalRaster);
        pSkyGFXNormalRaster = RwRasterCreate(postfxX, postfxY, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);
    }
}
void GFX_GrabScreen(bool second = false)
{
    GFX_CheckBuffersSize();

    ERQ_GrabFramebuffer(second ? pSkyGFXPostFXRaster2 : pSkyGFXPostFXRaster1);
    ERQ_GrabFramebufferPost();
}
void GFX_GrabDepth()
{
    GFX_CheckBuffersSize();

    if(TheCamera->m_pRwCamera)
    {
        ImmediateModeRenderStatesStore();
        ImmediateModeRenderStatesSet();

        GFX_ActivateRawDepthTexture();

        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

        RQVector uniValues = RQVector{ TheCamera->m_pRwCamera->nearClip, TheCamera->m_pRwCamera->farClip, 0.0f, 0.0f };
        pForcedShader = g_pSimpleDepthShader;
        pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);

        float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
        DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
        PostEffectsDrawQuad(0.0f, 0.0f, 2.0f, 2.0f, 255, 255, 255, 255, NULL);
        pForcedShader = NULL;

        ERQ_GrabFramebuffer(pSkyGFXDepthRaster);
        ERQ_GrabFramebufferPost();

        ImmediateModeRenderStatesReStore();
    }
}
void GFX_GrabTexIntoTex(RwRaster* src, RwRaster* dst, ES2Shader* shader = NULL, RQVector* uniValues1 = NULL, RQVector* uniValues2 = NULL, RQVector* uniValues3 = NULL)
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);

    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    if(!shader) shader = g_pFramebufferRenderShader;
    if(shader)
    {
        DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
        pForcedShader = shader;
        if(uniValues1) pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues1->x, 4);
        if(uniValues2) pForcedShader->SetVectorConstant(SVCID_GreenGrade, &uniValues2->x, 4);
        if(uniValues3) pForcedShader->SetVectorConstant(SVCID_BlueGrade, &uniValues3->x, 4);
        PostEffectsDrawQuad(0.0f, 0.0f,
                            2.0f * (float)dst->width * fpostfxXInv,
                            2.0f * (float)dst->height * fpostfxYInv,
                            255, 255, 255, 255, src);
        pForcedShader = NULL;
    }

    ERQ_GrabFramebuffer(dst);
    ERQ_GrabFramebufferPost();

    ImmediateModeRenderStatesReStore();
}
void GFX_GrabBrightness(float luminance)
{
    GFX_CheckBuffersSize();

    if(pSkyGFXBrightnessRaster)
    {
        GFX_ActivateProcessedDepthTexture();

        ImmediateModeRenderStatesStore();
        ImmediateModeRenderStatesSet();

        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

        RQVector uniValues = RQVector{ luminance, 0.0f, 0.0f, 0.0f };
        pForcedShader = g_pSimpleBrightShader;
        pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);

        float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
        DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
        PostEffectsDrawQuad(0.0f, 0.0f, 2.0f, 2.0f, 255, 255, 255, 255, NULL);
        pForcedShader = NULL;

        ERQ_GrabFramebuffer(pSkyGFXBrightnessRaster);
        ERQ_GrabFramebufferPost();

        ImmediateModeRenderStatesReStore();
    }
}
void GFX_GrabSceneBrightness()
{
    GFX_CheckBuffersSize();
    GFX_GrabTexIntoTex(pSkyGFXPostFXRaster1, pSkyGFXSceneBrightnessRaster);
}
void GFX_NormalBuffer()
{
    GFX_CheckBuffersSize();

    if(pSkyGFXNormalRaster)
    {
        GFX_ActivateRawDepthTexture();

        ImmediateModeRenderStatesStore();
        ImmediateModeRenderStatesSet();

        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

        RQVector uniValues = RQVector{ fpostfxXInv, fpostfxYInv, 0.0f, 0.0f };
        pForcedShader = g_pDepthNormalMapShader;
        pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);

        float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
        DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
        PostEffectsDrawQuad(0.0f, 0.0f, 2.0f, 2.0f, 255, 255, 255, 255, NULL);
        pForcedShader = NULL;

        ERQ_GrabFramebuffer(pSkyGFXNormalRaster);
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
void GFX_FrameBuffer(bool second);
void GFX_Radiosity(int intensityLimit, int filterPasses, int renderPasses, int intensity) // Does not work?
{
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)false);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)false);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)false);

    RQVector uniValues1 = RQVector{ 0.0f, 1.0f / (float)pSkyGFXRadiosityRaster->height,
                                   ((float)pSkyGFXRadiosityRaster->width / 640.0f) * (1 << filterPasses), 0.0f };
    GFX_GrabTexIntoTex(pSkyGFXPostFXRaster2, pSkyGFXRadiosityRaster, g_pRadiosityBlurShader, &uniValues1);

    uniValues1.x = 1.0f / (float)pSkyGFXRadiosityRaster->width;
    uniValues1.y = 0.0f;
    GFX_GrabTexIntoTex(pSkyGFXRadiosityRaster, pSkyGFXRadiosityRaster, g_pRadiosityBlurShader, &uniValues1);

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

    float off = ((1 << filterPasses) - 1);
    float offu = off * 2; // m_RadiosityFilterUCorrection;
    float offv = off * 2; // m_RadiosityFilterVCorrection;
    float minu = offu;
    float minv = offv;
    float maxu = pSkyGFXRadiosityRaster->width - offu; //off*2;
    float maxv = pSkyGFXRadiosityRaster->height - offv; //off*2;
    float cu = (offu*(pSkyGFXRadiosityRaster->width+0.5f) + offu/*off*2*/*0.5f) / pSkyGFXRadiosityRaster->width;
    float cv = (offv*(pSkyGFXRadiosityRaster->height+0.5f) + offv/*off*2*/*0.5f) / pSkyGFXRadiosityRaster->height;

    uniValues1.x = (float)intensityLimit / 255.0f;
    uniValues1.y = (float)intensity / 255.0f;
    uniValues1.z = (float)renderPasses;
    RQVector uniValues2 = RQVector{ cu / (float)pSkyGFXRadiosityRaster->width, cv / (float)pSkyGFXRadiosityRaster->height,
                                   (maxu - minu) / (float)pSkyGFXRadiosityRaster->width, (maxv - minv) / (float)pSkyGFXRadiosityRaster->height };

    GFX_FrameBuffer(true);
    
    pForcedShader = g_pRadiosityShader;
    pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues1.x, 4);
    pForcedShader->SetVectorConstant(SVCID_GreenGrade, &uniValues2.x, 4);

    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0f, 0.0f, 2.0f, 2.0f, 255, 255, 255, 255, pSkyGFXRadiosityRaster);
    pForcedShader = NULL;

    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)NULL);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    
    GFX_GrabScreen();
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
    const float recipNearClip = 1.0f / (TheCamera->m_pRwCamera->nearClip);
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

    RQVector uniValues = RQVector{ (float)alpha / 256.0f, 0.0f, 0.0f, 0.0f };
    pForcedShader = g_pVignetteShader;
    pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);
    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmax, umax, vmax, umax, vmin, umin, vmin);
    PostEffectsDrawQuad(0.0f, 0.0f, 2.0f, 2.0f, 0, 0, 0, 255, pDarkRaster);
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
        if(g_fDOFDistance >= TheCamera->m_pRwCamera->farClip) return;
        calc01Dist = 1.0f - g_fDOFDistance / (TheCamera->m_pRwCamera->farClip - TheCamera->m_pRwCamera->nearClip);
    }

    GFX_GrabScreen(true);
    
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    RQVector uniValues = RQVector{ fpostfxXInv, fpostfxYInv, 2.0f * g_fDOFStrength, calc01Dist };
    pForcedShader = g_pDOFShader; // g_pDADOFShader (not yet little guys)
    pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);

    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0, 0.0, 2.0f, 2.0f, 255, 255, 255, 255, pSkyGFXPostFXRaster2);
    pForcedShader = NULL;
    
    ImmediateModeRenderStatesReStore();
}
void GFX_GrabOcclusion()
{
    GFX_ActivateNormalBufferTexture();
    //GFX_ActivateProcessedDepthTexture();
    GFX_ActivateRawDepthTexture();

    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);

    RQVector uniValues = RQVector{ fpostfxX / 8.0f, fpostfxY / 8.0f, fpostfxXInv, fpostfxYInv };
    //RQVector uniValues = RQVector{ fpostfxX / 8.0f, fpostfxY / 8.0f, 0.5f, 0.025f };
    pForcedShader = g_pOcclusionBufferShader;
    pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);

    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0, 0.0, 2.0f, 2.0f, 255, 255, 255, 255, pNoiseRaster);
    pForcedShader = NULL;

    ERQ_GrabFramebuffer(pSkyGFXOcclusionRaster);
    ERQ_GrabFramebufferPost();
    
    ImmediateModeRenderStatesReStore();

    GFX_DeActivateTexture(1);
    GFX_DeActivateTexture();
}
void GFX_FrameBuffer(bool second = false) // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmax, umax, vmax, umax, vmin, umin, vmin);
    PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 255, 255, 255, 255, second ? pSkyGFXPostFXRaster2 : pSkyGFXPostFXRaster1);
    
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
void GFX_FrameBufferFXAA() // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

    RQVector uniValues = RQVector{ fpostfxXInv, fpostfxYInv, 0.0f, 0.0f };
    pForcedShader = g_pFXAAShader;
    pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);
    
    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0, 0.0, 2.0f, 2.0f, 255, 255, 255, 255, pSkyGFXPostFXRaster2);

    pForcedShader = NULL;
    ImmediateModeRenderStatesReStore();
}
void GFX_FrameBufferBloom(float intensity) // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);

    RQVector uniValues = RQVector{ 1.0f / (float)RsGlobal->maximumWidth, 1.0f / (float)RsGlobal->maximumHeight, intensity, 0.0f };
    pForcedShader = g_pBloomShader;
    pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);
    
    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0, 0.0, 2.0f, 2.0f, 255, 255, 255, 255, pSkyGFXPostFXRaster1);

    pForcedShader = NULL;
    ImmediateModeRenderStatesReStore();

    GFX_GrabScreen(); // Update the framebuffer with Bloom-processed image
}
void GFX_FrameBufferSSAO() // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);

    //RQVector uniValues = RQVector{ 1.0f / (float)RsGlobal->maximumWidth, 1.0f / (float)RsGlobal->maximumHeight, intensity, 0.0f };
    pForcedShader = g_pSSAOShader;
    //pForcedShader->SetVectorConstant(SVCID_RedGrade, &uniValues.x, 4);
    
    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmin, umax, vmin, umax, vmax, umin, vmax);
    PostEffectsDrawQuad(0.0, 0.0, 2.0f, 2.0f, 255, 255, 255, 255, pSkyGFXPostFXRaster2);

    pForcedShader = NULL;
    ImmediateModeRenderStatesReStore();

    GFX_GrabScreen(); // Update the framebuffer with Bloom-processed image
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
void GFX_BloomBuffer() // Completed
{
    ImmediateModeRenderStatesStore();
    ImmediateModeRenderStatesSet();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);

    float umin = 0.0f, vmin = 0.0f, umax = 1.0f, vmax = 1.0f;
    DrawQuadSetUVs(umin, vmax, umax, vmax, umax, vmin, umin, vmin);
    PostEffectsDrawQuad(0.0, 0.0, RsGlobal->maximumWidth, RsGlobal->maximumHeight, 255, 255, 255, 255, pSkyGFXBloomP3Raster);
    
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
static bool g_bPostFXStarted = false;
DECL_HOOKv(PostFX_Init)
{
    PostFX_Init();

    if(g_bPostFXStarted) return;

    if(!pGrainRaster)
    {
        pGrainRaster = RwRasterCreate(64, 64, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
        CreateGrainTexture(pGrainRaster);
    }

    if(!pDarkRaster)
    {
        pDarkRaster = RwRasterCreate(64, 64, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
        CreatePlainTexture(pDarkRaster, CRGBA(0, 0, 0));
    }

    if(!pNoiseRaster)
    {
        pNoiseRaster = RwRasterCreate(8, 8, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
        CreateRandomTexture(pNoiseRaster);
    }

    if(!pSkyGFXSceneBrightnessRaster)
    {
        pSkyGFXSceneBrightnessRaster = RwRasterCreate(8, 8, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);
    }

    *pbFog = false; // uninitialised variable
    g_bPostFXStarted = true;
}
float gfWaterGreen = 0.0f;
CRGBA m_waterCol(64, 64, 64, 64);
DECL_HOOKv(PostFX_Render)
{
    GFX_GrabScreen();
    GFX_GrabDepth();
    //GFX_NormalBuffer();
    if(g_bCSB)
    {
        // I got a request for this.
        GFX_FrameBufferCSB(g_fContrast, g_fSaturation, g_fBrightness, g_fGamma);
    }
    else
    {
        GFX_FrameBuffer();
    }

    if(g_bSSAO)
    {
        GFX_GrabScreen(true);
        GFX_GrabOcclusion();
        // First downscale
        GFX_GrabTexIntoTex(pSkyGFXOcclusionRaster, pSkyGFXOcclusionP1Raster);
        // Second downscale + horizontal blur
        RQVector uniValues = RQVector{ 1.0f / (float)pSkyGFXOcclusionP3Raster->width, 1.0f / (float)pSkyGFXOcclusionP3Raster->height, 1.0f, 0.0f };
        GFX_GrabTexIntoTex(pSkyGFXOcclusionP1Raster, pSkyGFXOcclusionP2Raster, g_pBloomP1Shader, &uniValues);
        // Vertical blur
        GFX_GrabTexIntoTex(pSkyGFXOcclusionP2Raster, pSkyGFXOcclusionP3Raster, g_pBloomP2Shader, &uniValues);

        GFX_ActivateFinalOcclusionTexture();
        GFX_FrameBufferSSAO();
        GFX_DeActivateTexture();
    }

    if(g_bBloom)
    {
        // Grabbing brightness values
        GFX_GrabBrightness( *currArea == 0 ? 0.1f : 0.3f );
        // Downscale 2 times
        GFX_GrabTexIntoTex(pSkyGFXBrightnessRaster, pSkyGFXBloomP1Raster);
        GFX_GrabTexIntoTex(pSkyGFXBloomP1Raster, pSkyGFXBloomP2Raster);
        // Bloom pass 3 (Final downscaling + horizontal blurring)
        RQVector uniValues = RQVector{ 1.0f / (float)pSkyGFXBloomP3Raster->width, 1.0f / (float)pSkyGFXBloomP3Raster->height, 1.0f, 0.0f };
        GFX_GrabTexIntoTex(pSkyGFXBloomP2Raster, pSkyGFXBloomP3Raster, g_pBloomP1Shader, &uniValues);
        // Bloom pass 4 (Vertical blurring)
        GFX_GrabTexIntoTex(pSkyGFXBloomP3Raster, pSkyGFXBloomP3Raster, g_pBloomP2Shader, &uniValues);
        // Apply bloom to the framebuffer
        GFX_ActivateFinalBloomTexture();
        float bloomIntensity = g_fBloomIntensity;
        if(*currArea != 0) bloomIntensity *= 0.72f;
        GFX_FrameBufferBloom( bloomIntensity );
        // Deactivate activated texture (so it can be used!)
        GFX_DeActivateTexture();
    }

    if(g_bAutoExposure)
    {
        GFX_GrabSceneBrightness();
        GFX_FrameBuffer();
    }

    if(*pbFog) RenderScreenFogPostEffect();

    PostFX_Render();

    if(g_bRadiosity && !*m_bDarknessFilter)
    {
        GFX_GrabScreen(true);
        GFX_Radiosity(m_CurrentColours->intensityLimit, *m_RadiosityFilterPasses, *m_RadiosityRenderPasses, *m_RadiosityIntensity);
    }
    
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

    if(!*pbInCutscene && g_nSpeedFX != SPFX_INACTIVE)
    {
        CAutomobile* veh = (CAutomobile*)FindPlayerVehicle(-1, false);
        if(veh && (veh->m_nVehicleSubType < VEHICLE_TYPE_HELI || veh->m_nVehicleSubType > VEHICLE_TYPE_TRAIN) )
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

    if(WaterDrops::neoWaterDrops)
    {
        WaterDrops::Process();
        WaterDrops::Render();
    }

    if(g_bFXAA)
    {
        GFX_GrabScreen(true);
        GFX_FrameBufferFXAA();
    }
    
    GFX_Vignette(g_nVignette * 2.55f);

    GFX_DeActivateTexture();
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
        WaterDrops::InitialiseHooks();
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

        pCFGRadiosity = cfg->Bind("Radiosity", g_bRadiosity, "Effects");
        RadiositySettingChanged(g_bRadiosity, pCFGRadiosity->GetBool());
        AddSetting("Radiosity", g_bRadiosity, 0, sizeofA(aYesNo)-1, aYesNo, RadiositySettingChanged, NULL);
        *m_RadiosityFilterPasses = cfg->GetInt("Radiosity_FilterPasses", *m_RadiosityFilterPasses, "Effects");
        *m_RadiosityRenderPasses = cfg->GetInt("Radiosity_RenderPasses", *m_RadiosityRenderPasses, "Effects");
        *m_RadiosityIntensity = cfg->GetInt("Radiosity_Intensity", *m_RadiosityIntensity, "Effects");

        pCFGNeoWaterDrops = cfg->Bind("NEOWaterDrops", WaterDrops::neoWaterDrops, "Effects");
        NEOWaterDropsSettingChanged(WaterDrops::neoWaterDrops, pCFGNeoWaterDrops->GetBool());
        AddSetting("NEO Water Drops", WaterDrops::neoWaterDrops, 0, sizeofA(aYesNo)-1, aYesNo, NEOWaterDropsSettingChanged, NULL);

        pCFGNeoBloodDrops = cfg->Bind("NEOBloodDrops", WaterDrops::neoBloodDrops, "Effects");
        NEOBloodDropsSettingChanged(WaterDrops::neoBloodDrops, pCFGNeoBloodDrops->GetBool());
        AddSetting("NEO Blood Drops", WaterDrops::neoBloodDrops, 0, sizeofA(aYesNo)-1, aYesNo, NEOBloodDropsSettingChanged, NULL);

        pCFGFXAA = cfg->Bind("AntiAliasing", g_bFXAA, "EnchancedEffects");
        FXAASettingChanged(g_bFXAA, pCFGFXAA->GetBool());
        AddSetting("FXAA", g_bFXAA, 0, sizeofA(aYesNo)-1, aYesNo, FXAASettingChanged, NULL);
        
        pCFGCrAbFX = cfg->Bind("ChromaticAberration", g_nCrAb, "EnchancedEffects");
        CrAbFXSettingChanged(g_nCrAb, pCFGCrAbFX->GetInt());
        AddSetting("Chromatic Aberration", g_nCrAb, 0, sizeofA(aCrAbFXSettings)-1, aCrAbFXSettings, CrAbFXSettingChanged, NULL);

        pCFGVignette = cfg->Bind("VignetteIntensity", g_nVignette, "EnchancedEffects");
        VignetteSettingChanged(g_nVignette, pCFGVignette->GetInt());
        AddSlider("Vignette Intensity", g_nVignette, 0, 100, VignetteSettingChanged, NULL, NULL);

        pCFGDOF = cfg->Bind("DOFType", g_nDOF, "EnchancedEffects");
        DOFSettingChanged(g_nDOF, pCFGDOF->GetInt());
        AddSetting("Depth'o'Field", g_nDOF, 0, sizeofA(aDOFSettings)-1, aDOFSettings, DOFSettingChanged, NULL);
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

        pCFGBloom = cfg->Bind("Bloom", g_bBloom, "EnchancedEffects");
        BloomSettingChanged(g_bBloom, pCFGBloom->GetBool());
        AddSetting("Bloom", g_bBloom, 0, sizeofA(aYesNo)-1, aYesNo, BloomSettingChanged, NULL);
        g_fBloomIntensity = cfg->GetFloat("Bloom_Intensity", g_fBloomIntensity, "EnchancedEffects");

        if(g_bHeatHaze)
        {
            HOOKBLX(HeatHazeFX_GrabBuffer, pGTASA + BYBIT(0x5B51E6, 0x6D94EC));
        }

        // Normal map buffer (dumbass Qualcomm removed a required extension from a list, though its supported)

        /*

        // 30 max including terminator
        const char* gl_FragColorRepl1 = "gl_FragData[0].a=Out_Color.a;";
        aml->Write(pGTASA + BYBIT(0x0, 0x7115B2), gl_FragColorRepl1, strlen(gl_FragColorRepl1)+1);
        // 39 max including terminator
        const char* gl_FragColorRepl2 = "if(gl_FragData[0].a < 0.8){ discard; }";
        aml->Write(pGTASA + BYBIT(0x0, 0x71161E), gl_FragColorRepl2, strlen(gl_FragColorRepl2)+1);
        // 39 max including terminator
        const char* gl_FragColorRepl3 = "if(gl_FragData[0].a < 0.5){ discard; }";
        aml->Write(pGTASA + BYBIT(0x0, 0x711645), gl_FragColorRepl3, strlen(gl_FragColorRepl3)+1);
        // 39 max including terminator
        const char* gl_FragColorRepl4 = "if(gl_FragData[0].a < 0.2){ discard; }";
        aml->Write(pGTASA + BYBIT(0x0, 0x71166C), gl_FragColorRepl4, strlen(gl_FragColorRepl4)+1);
        // 33 max including terminator
        const char* gl_FragColorRepl5 = "gl_FragData[0].a*=AlphaModulate;";
        aml->Write(pGTASA + BYBIT(0x0, 0x711693), gl_FragColorRepl5, strlen(gl_FragColorRepl5)+1);
        const char* sApplyColorShaderPart = "gl_FragData[0] = fcolor;\ngl_FragData[1] = vec4(WorldNormal, 1.0);"; // max is 511
    #ifdef AML32
        aml->WriteAddr(pGTASA + 0x1CF8A0, (uintptr_t)sApplyColorShaderPart - pGTASA - 0x0);
    #else
        aml->Write32(pGTASA + 0x263BF4, 0xF942BC42);
        aml->WriteAddr(pGTASA + 0x711578, (uintptr_t)sApplyColorShaderPart);
    #endif
        // Pre-Final part: Remove vec3 init from WorldNormal, its varying now
        aml->Write(pGTASA + BYBIT(0x0, 0x711F38), "   ");
        aml->Write(pGTASA + BYBIT(0x0, 0x711F8C), "   ");
        aml->Write(pGTASA + BYBIT(0x0, 0x711FCF), "   ");
        aml->Write(pGTASA + BYBIT(0x0, 0x712006), "   ");
        // Most Final part: Enable multiple targets
        const char* sShaderTopPart = "#extension GL_EXT_draw_buffers : require\nprecision mediump float;"; // max is 511
    #ifdef AML32
        aml->WriteAddr(pGTASA + 0x1CF8A0, (uintptr_t)sShaderTopPart - pGTASA - 0x0);
    #else
        aml->Write32(pGTASA + 0x263650, 0xF947A842);
        aml->WriteAddr(pGTASA + 0x710F50, (uintptr_t)sShaderTopPart);
    #endif

        */
    }
}
