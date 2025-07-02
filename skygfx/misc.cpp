#include <externs.h>
#include "include/renderqueue.h"

/* Variables */
bool g_bRemoveDumbWaterColorCalculations = true;
bool g_bFixMirrorIssue = true;
bool g_bPS2Sun = true;
bool g_bMoonPhases = true;
bool g_bPS2Flare = true;
bool g_bTransparentLockOn = true;

/* Configs */
ConfigEntry* pCFGPS2SunZTest;
ConfigEntry* pCFGPS2Flare;

/* Hooks */
DECL_HOOKv(MoonMask_RenderSprite, float ScreenX, float ScreenY, float ScreenZ, float SizeX, float SizeY, UInt8 R, UInt8 G, UInt8 B, Int16 Intensity16, float RecipZ, UInt8 Alpha, bool8 FlipU, bool8 FlipV, float uvPad1, float uvPad2)
{
    ERQ_BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ZERO);
    MoonMask_RenderSprite(ScreenX, ScreenY, ScreenZ, SizeX, SizeY, R, G, B, Intensity16, RecipZ, Alpha, FlipU, FlipV, uvPad1, uvPad2);
    ERQ_BlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO); // initial values
}
DECL_HOOKv(RenderBufferedOneXLUSprite2D, float ScreenX, float ScreenY, float SizeX, float SizeY, const RwRGBA *rgbaColor, int16 Intensity16, char Alpha)
{
    if(!g_bPS2Flare)
    {
        return RenderBufferedOneXLUSprite2D(ScreenX, ScreenY, SizeX, SizeY, rgbaColor, Intensity16, Alpha);
    }

    //float SizeX_Scaled = (float)RsGlobal->maximumWidth  / 640.0f;
    float SizeY_Scaled = (float)RsGlobal->maximumHeight / 448.0f;

    // Yes, i scale X with scaleY too.
    RenderBufferedOneXLUSprite2D(ScreenX, ScreenY, SizeX * SizeY_Scaled, SizeY * SizeY_Scaled, rgbaColor, Intensity16, Alpha);
}
DECL_HOOKv(RenderCoronasPLT)
{
    InitSpriteBuffer2D();
    RenderCoronasPLT();
}
DECL_HOOKv(CoronasRender_RenderSingleCorona, float ScreenX, float ScreenY, float ScreenZ, float SizeX, float SizeY, UInt8 R, UInt8 G, UInt8 B, Int16 Intensity16, float RecipZ, float Rotation, uintptr_t coronaValue)
{
    CoronasRender_RenderSingleCorona(ScreenX, ScreenY, ScreenZ, SizeX, SizeY, R, G, B, Intensity16, RecipZ, Rotation, 255);

  #ifdef AML32
    CRegisteredCorona* corona = (CRegisteredCorona*)( ( *(uintptr_t*)(pGTASA + 0x676C44) + 4 * coronaValue ) );
  #else
    CRegisteredCorona* corona = (CRegisteredCorona*)coronaValue;
  #endif

    // Seems to be PS2-exclusive stuff. Why the heck is it removed?
    if(corona->m_bDrawWithWhiteCore)
    {
        float coreIntensity = *Foggyness * fminf(ScreenZ, 40.0f) / 40.0f + 1.0f;
        float coreIntensity_Inv = 1.0f / coreIntensity;
        uint8_t R_WhiteCore = 100.0f * coreIntensity_Inv;

        // On PS2 it's Vector(100.0,100.0,100.0) and has 3 calculations for 3 variables. But they are the same. Using a single R_WhiteCore here.
        CoronasRender_RenderSingleCorona(ScreenX, ScreenY, ScreenZ, 0.1f * SizeX, 0.1f * SizeY, R_WhiteCore, R_WhiteCore, R_WhiteCore, Intensity16, RecipZ * 1.5f, Rotation * 1.5f, 255);
    }
}

/* Functions */
void PS2SunZTestSettingChanged(int oldVal, int newVal, void* data)
{
    if(oldVal == newVal) return;

    pCFGPS2SunZTest->SetBool(newVal != 0);
    g_bPS2Sun = pCFGPS2SunZTest->GetBool();
    
    if(g_bPS2Sun)
    {
        aml->PlaceNOP4(pGTASA + BYBIT(0x5A26B0, 0x6C600C), 1);
    }
    else
    {
        aml->Write32(pGTASA + BYBIT(0x5A26B0, 0x6C600C), BYBIT(0xE0ECF7F0, 0x97ED6FCD));
    }

    cfg->Save();
}
void PS2FlareSettingChanged(int oldVal, int newVal, void* data)
{
    if(oldVal == newVal) return;

    pCFGPS2Flare->SetBool(newVal != 0);
    g_bPS2Flare = pCFGPS2Flare->GetBool();
    
    if(g_bPS2Flare)
    {
        aml->Write32(pGTASA + BYBIT(0x5A293A, 0x6C62B4), BYBIT(0xBF002000, 0x52800000)); // A weirdo useless check if flare-part is visible..?
    }
    else
    {
        aml->Write32(pGTASA + BYBIT(0x5A293A, 0x6C62B4), BYBIT(0xE44AF7F2, 0x97ED7A23));
    }

    cfg->Save();
}

uintptr_t BrightnessRGBGrade_BackTo;
extern "C" uintptr_t BrightnessRGBGrade_Patch(float calculatedValue, RQVector* rGrade, RQVector* gGrade, RQVector* bGrade)
{
    rGrade->r += calculatedValue;
    rGrade->g += calculatedValue;
    rGrade->b += calculatedValue;
    //rGrade->a += calculatedValue;
    
    gGrade->r += calculatedValue;
    gGrade->g += calculatedValue;
    gGrade->b += calculatedValue;
    //gGrade->a += calculatedValue;
    
    bGrade->r += calculatedValue;
    bGrade->g += calculatedValue;
    bGrade->b += calculatedValue;
    //bGrade->a += calculatedValue;

    return BrightnessRGBGrade_BackTo;
}

/* Patches */
__attribute__((optnone)) __attribute__((naked)) void BrightnessRGBGrade_Inject(void)
{
#ifdef AML32
    asm("VMOV R0, S0");
    asm("ADD R1, SP, #0x30");
    asm("ADD R2, SP, #0x20");
    asm("ADD R3, SP, #0x10");
    //asm("VPUSH {D16-D19}");
    asm("BL BrightnessRGBGrade_Patch");
    //asm("VPOP {D16-D19}");
    asm("BX R0");
#else
    asm("ADD X0, SP, #0x30");
    asm("ADD X1, SP, #0x20");
    asm("ADD X2, SP, #0x10");
    asm("BL BrightnessRGBGrade_Patch");
    asm("BR X0");
#endif
}

/* Main */
void StartMiscStuff()
{
    g_bRemoveDumbWaterColorCalculations = cfg->GetBool("RemoveDumbWaterColorCalc", g_bRemoveDumbWaterColorCalculations, "Visual");
    g_bFixMirrorIssue = cfg->GetBool("FixMirrorIssue", g_bFixMirrorIssue, "Visual"); // SkyGFX 4.0b: fixed mirror issue
    g_bMoonPhases = cfg->GetBool("MoonPhases", g_bMoonPhases, "Visual");
    g_bTransparentLockOn = cfg->GetBool("TransparentLockOn", g_bTransparentLockOn, "Visual");
    
    if(g_bRemoveDumbWaterColorCalculations)
    {
      #ifdef AML32
        aml->PlaceNOP4(pGTASA + 0x5989A8, 1);
        aml->PlaceNOP4(pGTASA + 0x5989B0, 1);
      #else
        aml->PlaceNOP4(pGTASA + 0x6BD1C4, 1);
      #endif
    }

    if(g_bFixMirrorIssue)
    {
      #ifdef AML32
        aml->WriteFloat(pGTASA + 0x5C5664, -216.1f);
        // TODO: value for +216.1 ...
      #else
        aml->WriteFloat(pGTASA + 0x764D50, -216.1f);
        // TODO: value for +216.1 ...
      #endif
    }

    if(g_bMoonPhases)
    {
        HOOKBL(MoonMask_RenderSprite, pGTASA + BYBIT(0x59EE32, 0x6C2DE4));
    }

    // Sun Z-Test disabled (to match PS2 logic, same in reverse of PS2 game)
    pCFGPS2SunZTest = cfg->Bind("PS2SunZTest", g_bPS2Sun, "Visual");
    PS2SunZTestSettingChanged(false, pCFGPS2SunZTest->GetBool(), NULL);
    AddSetting("PS2 Sun Z-Test", g_bPS2Sun, 0, sizeofA(aYesNo)-1, aYesNo, PS2SunZTestSettingChanged, NULL);

    // Better RGBA quality rasters
  #ifdef AML32
    aml->Write8(pGTASA + 0x1AE95E, 0x01); // RwRasterCreate -> _rwOpenGLRasterCreate
    aml->Write8(pGTASA + 0x1BBA5E, 0x01); // emu_SetAltRenderTarget -> backTarget
  #else
    aml->Write32(pGTASA + 0x23FDE0, 0x52800022); // RwRasterCreate -> _rwOpenGLRasterCreate
    aml->Write32(pGTASA + 0x24ECC4, 0x52800022); // emu_SetAltRenderTarget -> backTarget
  #endif

    // Flare rendering like it was on PS2
    HOOKPLT(RenderBufferedOneXLUSprite2D, pGTASA + BYBIT(0x674E98, 0x8483A0));
    pCFGPS2Flare = cfg->Bind("PS2Flare", g_bPS2Flare, "Visual");
    PS2FlareSettingChanged(false, pCFGPS2Flare->GetBool(), NULL);
    AddSetting("PS2 Flare", g_bPS2Flare, 0, sizeofA(aYesNo)-1, aYesNo, PS2FlareSettingChanged, NULL);

    // Fixing uninitialized variable (Jesus Christ, wtf Rockstar)
    HOOKPLT(RenderCoronasPLT, pGTASA + BYBIT(0x670490, 0x840B58));

    // White Core in coronas (exists on PS2 only?)
  #ifdef AML32
    aml->Write16(pGTASA + 0x5A27DC, 0x465E);
  #else
    aml->Write32(pGTASA + 0x6C6150, 0xAA0803E4);
  #endif
    HOOKBLX(CoronasRender_RenderSingleCorona, pGTASA + BYBIT(0x5A2816, 0x6C6158));

    // Fixing colorcycle grades with brightness 60+
  #ifdef AML32
    BrightnessRGBGrade_BackTo = pGTASA + 0x5B6818 + 0x1;
    aml->Redirect(pGTASA + 0x5B67F4 + 0x1, (uintptr_t)BrightnessRGBGrade_Inject);
  #else
    BrightnessRGBGrade_BackTo = pGTASA + 0x6DAAE4;
    aml->Redirect(pGTASA + 0x6DAACC, (uintptr_t)BrightnessRGBGrade_Inject);
  #endif

    // Remove black background of lockon siphon
    if(g_bTransparentLockOn)
    {
      #ifdef AML32
        aml->PlaceNOP4(pGTASA + 0x5E3602, 1);
        aml->PlaceNOP4(pGTASA + 0x5E364C, 1);
        aml->PlaceNOP4(pGTASA + 0x5E378C, 1);
        aml->PlaceNOP4(pGTASA + 0x5E37F4, 1);
      #else
        aml->PlaceNOP4(pGTASA + 0x70907C, 1);
        aml->PlaceNOP4(pGTASA + 0x7090C4, 1);
        aml->PlaceNOP4(pGTASA + 0x7091C8, 1);
        aml->PlaceNOP4(pGTASA + 0x70921C, 1);
      #endif
    }

    // Amazing WarDrum stuff (darker world when looking at the sun)
    WRITEADDR(pGTASA + BYBIT(0x6786A8, 0x84ED78), pGTASA + BYBIT(0x6B15EC, 0x88DFF0));
}
