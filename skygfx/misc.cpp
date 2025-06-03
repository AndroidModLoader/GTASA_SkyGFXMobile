#include <externs.h>
#include "include/renderqueue.h"

/* Variables */
bool g_bRemoveDumbWaterColorCalculations = true;
bool g_bFixMirrorIssue = true;
bool g_bPS2Sun = true;

/* Configs */
ConfigEntry* pCFGPS2SunZTest;

/* Hooks */
DECL_HOOKv(MoonMask_RenderSprite, float ScreenX, float ScreenY, float ScreenZ, float SizeX, float SizeY, UInt8 R, UInt8 G, UInt8 B, Int16 Intensity16, float RecipZ, UInt8 Alpha, bool8 FlipU, bool8 FlipV, float uvPad1, float uvPad2)
{
    ERQ_BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ZERO);
    MoonMask_RenderSprite(ScreenX, ScreenY, ScreenZ, SizeX, SizeY, R, G, B, Intensity16, RecipZ, Alpha, FlipU, FlipV, uvPad1, uvPad2);
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

/* Main */
void StartMiscStuff()
{
    g_bRemoveDumbWaterColorCalculations = cfg->GetBool("RemoveDumbWaterColorCalc", g_bRemoveDumbWaterColorCalculations, "Visual");
    g_bFixMirrorIssue = cfg->GetBool("FixMirrorIssue", g_bFixMirrorIssue, "Visual"); // SkyGFX 4.0b: fixed mirror issue

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

    // Sun Z-Test disabled (to match PS2 logic, same in reverse on PS2)
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

    // Moon phases (works here, doesnt work in SAMP. HELLO?)
    HOOKBL(MoonMask_RenderSprite, pGTASA + BYBIT(0x59EE32, 0x6C2DE4));
}