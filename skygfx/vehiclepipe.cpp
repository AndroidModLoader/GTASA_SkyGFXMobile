#include <externs.h>

/* Variables */
bool g_bVehicleAtomicsPipeline = true;
bool g_bRenderingVehicleWheel = false;

/* Hooks */
DECL_HOOK(RpAtomic*, RenderWheelAtomicCB, RpAtomic* pAtomic)
{
    g_bRenderingVehicleWheel = true;
    RenderWheelAtomicCB(pAtomic);
    g_bRenderingVehicleWheel = false;
    return pAtomic;
}

/* Functions */

/* Main */
void StartVehiclePipeline()
{
    g_bVehicleAtomicsPipeline = cfg->GetBool("VehicleComponentsUseCarPipeline", g_bVehicleAtomicsPipeline, "Vehicles");

    if(g_bVehicleAtomicsPipeline)
    {
        aml->WriteAddr(pGTASA + BYBIT(0x671D7C, 0x8433A0), pGTASA + BYBIT(0x2CB7E1, 0x38CF7C));
    }

    HOOK(RenderWheelAtomicCB, aml->GetSym(hGTASA, "_ZN18CVisibilityPlugins19RenderWheelAtomicCBEP8RpAtomic"));
}