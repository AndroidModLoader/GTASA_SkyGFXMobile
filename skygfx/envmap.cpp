#include <externs.h>

/* Others */
enum
{
    ENVMAP_MOBILE = 0,
    ENVMAP_VCS_LCS,
    ENVMAP_XBOX, // NEO

    ENVMAP_MAX
};

/* Variables */
int g_nEnvMapType = ENVMAP_MOBILE;
float g_fForcedEnvRadius = 50.0f;

const char* aEnvMapNames[ENVMAP_MAX] = 
{
    "Default (Mobile)",
    "VCS/LCS Style",
    "Xbox Style",
};

/* Configs */
ConfigEntry* pCFGEnvMapType;

/* Hooks */
DECL_HOOKv(RenderEnvMap)
{
    float saveVal = *ms_fFarClip;
    *ms_fFarClip = 0.985f * g_fForcedEnvRadius; // env sphere farclip

    // TODO: need to rewrite RenderScene(0) (which is above that func)
    if(g_nEnvMapType == ENVMAP_MOBILE)
    {
        RenderEnvMap(); // CBirds::Render()
    }
    else if(g_nEnvMapType == ENVMAP_VCS_LCS)
    {
        RenderSkyPolys();
        //RenderCoronas(); // TODO: Not working here
    }
    else if(g_nEnvMapType == ENVMAP_XBOX)
    {
        //RenderClouds();
        //RenderCoronas();
    }

    *ms_fFarClip = saveVal;
}
DECL_HOOKv(EnvMapColor, CRGBA* self, UInt8 red, UInt8 green, UInt8 blue, UInt8 alpha)
{
    if(g_nEnvMapType == ENVMAP_MOBILE)
    {
        if(red < 64) red = 64;
        if(green < 64) green = 64;
        if(blue < 64) blue = 64;
    }
    else if(g_nEnvMapType == ENVMAP_VCS_LCS)
    {
        
    }
    else if(g_nEnvMapType == ENVMAP_XBOX)
    {
        red   = 0.70f * p_CTimeCycle__m_CurrentColours->skybotr + 0.30f * 255;
        green = 0.75f * p_CTimeCycle__m_CurrentColours->skybotg + 0.25f * 255;
        blue  = 0.70f * p_CTimeCycle__m_CurrentColours->skybotb + 0.30f * 255;
    }

    self->r = red;
    self->g = green;
    self->b = blue;
    self->a = alpha;
}

/* Functions */
void EnvMapSettingChanged(int oldVal, int newVal, void* data)
{
    if(oldVal == newVal) return;

    pCFGEnvMapType->SetInt(newVal);
    pCFGEnvMapType->Clamp(0, ENVMAP_MAX - 1);
    g_nEnvMapType = pCFGEnvMapType->GetInt();

    cfg->Save();
}

/* Main */
void StartEnvMapStuff()
{
    pCFGEnvMapType = cfg->Bind("EnvReflectionType", g_nEnvMapType, "Visuals");
    EnvMapSettingChanged(ENVMAP_MOBILE, pCFGEnvMapType->GetInt(), NULL);
    AddSetting("Env Reflection Type", g_nEnvMapType, 0, sizeofA(aEnvMapNames)-1, aEnvMapNames, EnvMapSettingChanged, NULL);

  #ifdef AML32
    aml->PlaceNOP(pGTASA + 0x5C4EC2, 1);
    aml->PlaceNOP(pGTASA + 0x5C4EC8, 1);
    aml->PlaceNOP(pGTASA + 0x5C4ECE, 1);
    aml->WriteFloat(pGTASA + 0x5C4FFC, g_fForcedEnvRadius);
    aml->WriteFloat(pGTASA + 0x5C5000, g_fForcedEnvRadius); // envMap radius
  #else
    aml->Write32(pGTASA + 0x6E94A4, 0x2A0903E1);
    aml->Write32(pGTASA + 0x6E94AC, 0x2A0B03E2);
    aml->Write32(pGTASA + 0x6E94B4, 0x2A0803E3);
    aml->WriteFloat(pGTASA + 0x764DD0, g_fForcedEnvRadius);
    aml->WriteFloat(pGTASA + 0x764DD4, g_fForcedEnvRadius); // envMap radius
  #endif
    
    HOOKBLX(RenderEnvMap, pGTASA + BYBIT(0x5C4F94, 0x6E9594));
    HOOKBLX(EnvMapColor, pGTASA + BYBIT(0x5C4EDA, 0x6E94B8));
}
