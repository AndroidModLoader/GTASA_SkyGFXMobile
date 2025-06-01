#include <externs.h>

/* Variables */
bool g_bRemoveDumbWaterColorCalculations = true;
bool g_bFixMirrorIssue = true;

/* Hooks */
DECL_HOOKv(enableLights, int e)
{
    enableLights(1);
}

/* Functions */

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

    // A test fix or camnorm textures being dark at some angles (might broke everything)
    HOOK(enableLights, aml->GetSym(hGTASA, "_Z21_rwOpenGLLightsEnablei"));
}