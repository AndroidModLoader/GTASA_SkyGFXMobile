#include <externs.h>

/* Others */
enum
{
    SHADING_MOBILE = 0,
    SHADING_PC_PS2,
    SHADING_NONE,

    SHADING_MAX
};

/* Variables */
int g_nShading = SHADING_MOBILE;
const char* aShadingSwitch[SHADING_MAX] = 
{
    "Default (Mobile)",
    "PC/PS2 Algorithm",
    "Simplified Algorithm",
};

/* Configs */
ConfigEntry* pCFGShading;

/* Functions */
void ShadingSettingChanged(int oldVal, int newVal, void* data)
{
    if(oldVal == newVal) return;

    pCFGShading->SetInt(newVal);
    pCFGShading->Clamp(0, SHADING_MAX - 1);
    g_nShading = pCFGShading->GetInt();

    cfg->Save();
}
inline void _rwOpenGLEnableColorMaterial(RwInt32 enable)
{
    if(enable)
    {
        if(!*p_rwOpenGLColorMaterialEnabled)
        {
            emu_glEnable(GL_COLOR_MATERIAL);
            *p_rwOpenGLColorMaterialEnabled = 1;
        }
    }
    else
    {
        if(*p_rwOpenGLColorMaterialEnabled)
        {
            emu_glDisable(GL_COLOR_MATERIAL);
            *p_rwOpenGLColorMaterialEnabled = 0;
        }
    }
}

/* Hooks */
DECL_HOOKv(_rwOpenGLLightsSetMaterialProperties, const RpMaterial *mat, RwUInt32 flags)
{
    if(g_nShading == SHADING_MOBILE || g_nShading == SHADING_NONE)
    {
        _rwOpenGLLightsSetMaterialProperties(mat, flags);
        return;
    }

    float surfProps[4];
    float colorScale[4];
    surfProps[0] = mat->surfaceProps.ambient;
    surfProps[1] = mat->surfaceProps.diffuse;
    // could use for env and spec data perhaps
    surfProps[2] = 0.0f;
    surfProps[3] = 0.0f;
    colorScale[0] = mat->color.red   / 255.0f;
    colorScale[1] = mat->color.green / 255.0f;
    colorScale[2] = mat->color.blue  / 255.0f;
    colorScale[3] = mat->color.alpha / 255.0f;
    // repurposing material colors here
    emu_glMaterialfv(GL_FRONT, GL_AMBIENT, surfProps);
    emu_glMaterialfv(GL_FRONT, GL_DIFFUSE, colorScale);

    // multiplied by 1.5 internally, let's undo that
    float ambHack[4];
    ambHack[0] = openglAmbientLight[0] / 1.5f;
    ambHack[1] = openglAmbientLight[1] / 1.5f;
    ambHack[2] = openglAmbientLight[2] / 1.5f;
    ambHack[3] = openglAmbientLight[3] / 1.5f;
    emu_glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambHack);

    if((flags & rxGEOMETRY_PRELIT) != 0) // prelight
    {    
        _rwOpenGLEnableColorMaterial(1);
        emu_glColorMaterial(GL_FRONT, GL_EMISSION);
    }
    else
    {
        _rwOpenGLEnableColorMaterial(0);
        // have no use for this yet
        emu_glMaterialfv(GL_FRONT, GL_EMISSION, _rwOpenGLOpaqueBlack);
    }
}

void SetLightsWithTimeOfDayColour_PC_PS2(RpWorld* world)
{
    if(*p_pAmbient)
    {
        float ambMult = *p_gfLaRiotsLightMult * *p_CCoronas__LightsMult;
        p_AmbientLightColourForFrame->red   = p_CTimeCycle__m_CurrentColours->ambr * ambMult;
        p_AmbientLightColourForFrame->green = p_CTimeCycle__m_CurrentColours->ambg * ambMult;
        p_AmbientLightColourForFrame->blue  = p_CTimeCycle__m_CurrentColours->ambb * ambMult;

        ambMult = *p_CCoronas__LightsMult * 1.22f; // def. 1.5f
        p_AmbientLightColourForFrame_PedsCarsAndObjects->red = p_CTimeCycle__m_CurrentColours->ambobjr * ambMult;
        p_AmbientLightColourForFrame_PedsCarsAndObjects->green = p_CTimeCycle__m_CurrentColours->ambobjg * ambMult;
        p_AmbientLightColourForFrame_PedsCarsAndObjects->blue = p_CTimeCycle__m_CurrentColours->ambobjb * ambMult;

        if(*p_CWeather__LightningFlash)
        {
            p_AmbientLightColourForFrame->red   = 1.0f;
            p_AmbientLightColourForFrame->green = 1.0f;
            p_AmbientLightColourForFrame->blue  = 1.0f;

            p_AmbientLightColourForFrame_PedsCarsAndObjects->red   = 1.0f;
            p_AmbientLightColourForFrame_PedsCarsAndObjects->green = 1.0f;
            p_AmbientLightColourForFrame_PedsCarsAndObjects->blue  = 1.0f;
        }

        RpLightSetColor(*p_pAmbient, p_AmbientLightColourForFrame);
    }

    if(*p_pDirect)
    {
        (*p_pDirect)->isMainLight = 1;

        float dirMult = (255.0f * *p_CCoronas__LightsMult) / 256.0f;
        p_DirectionalLightColourForFrame->red = p_CTimeCycle__m_CurrentColours->directionalmult * dirMult;
        p_DirectionalLightColourForFrame->green = p_CTimeCycle__m_CurrentColours->directionalmult * dirMult;
        p_DirectionalLightColourForFrame->blue = p_CTimeCycle__m_CurrentColours->directionalmult * dirMult;
        RpLightSetColor(*p_pDirect, p_DirectionalLightColourForFrame);

        RwMatrix mat;
        memset(&mat, 0, sizeof(mat));
        CVector vecsun = *p_CTimeCycle__m_vecDirnLightToSun;
        CVector vec1 = { 0.0f, 0.0f, 1.0f };
        CVector vec2 = CrossProduct(&vec1, &vecsun);
        VectorNormalise(&vec2);
        vec1 = CrossProduct(&vec2, &vecsun);
        mat.at.x = -vecsun.x;
        mat.at.y = -vecsun.y;
        mat.at.z = -vecsun.z;
        mat.right.x = vec1.x;
        mat.right.y = vec1.y;
        mat.right.z = vec1.z;
        mat.up.x = vec2.x;
        mat.up.y = vec2.y;
        mat.up.z = vec2.z;

        RwFrameTransform(RpLightGetParent(*p_pDirect), &mat, rwCOMBINEREPLACE);

        // Default mobile code
        /*p_DirectionalLightColourFromDay->red   = (float)p_CTimeCycle__m_CurrentColours->skytopr / 255.0f + 0.65f * (0.33f - (float)p_CTimeCycle__m_CurrentColours->skytopr / 255.0f);
        p_DirectionalLightColourFromDay->green = (float)p_CTimeCycle__m_CurrentColours->skytopg / 255.0f + 0.75f * (0.23f - (float)p_CTimeCycle__m_CurrentColours->skytopg / 255.0f);
        p_DirectionalLightColourFromDay->blue  = (float)p_CTimeCycle__m_CurrentColours->skytopb / 255.0f + 0.75f * (0.23f - (float)p_CTimeCycle__m_CurrentColours->skytopb / 255.0f);*/
        
        // Some stuff close to RGB->EYE brightness percentage
        /*p_DirectionalLightColourFromDay->red   = (float)p_CTimeCycle__m_CurrentColours->skytopr / 255.0f + 0.65f * (0.33f - (float)p_CTimeCycle__m_CurrentColours->skytopr / 255.0f);
        p_DirectionalLightColourFromDay->green = (float)p_CTimeCycle__m_CurrentColours->skytopg / 255.0f + 0.75f * (0.23f - (float)p_CTimeCycle__m_CurrentColours->skytopg / 255.0f);
        p_DirectionalLightColourFromDay->blue  = (float)p_CTimeCycle__m_CurrentColours->skytopb / 255.0f + 0.55f * (0.43f - (float)p_CTimeCycle__m_CurrentColours->skytopb / 255.0f);*/

        // Slightly modified default code
        p_DirectionalLightColourFromDay->red   = (float)p_CTimeCycle__m_CurrentColours->skytopr / 255.0f + 0.60f * (0.28f - (float)p_CTimeCycle__m_CurrentColours->skytopr / 255.0f);
        p_DirectionalLightColourFromDay->green = (float)p_CTimeCycle__m_CurrentColours->skytopg / 255.0f + 0.72f * (0.25f - (float)p_CTimeCycle__m_CurrentColours->skytopg / 255.0f);
        p_DirectionalLightColourFromDay->blue  = (float)p_CTimeCycle__m_CurrentColours->skytopb / 255.0f + 0.75f * (0.23f - (float)p_CTimeCycle__m_CurrentColours->skytopb / 255.0f);

        //*p_DirectionalLightColourFromDay = *p_AmbientLightColourForFrame;
    }
}
void SetLightsWithTimeOfDayColour_Simple(RpWorld* world)
{
    if(*p_pAmbient)
    {
        p_AmbientLightColourForFrame->red = p_CTimeCycle__m_CurrentColours->ambr;
        p_AmbientLightColourForFrame->green = p_CTimeCycle__m_CurrentColours->ambg;
        p_AmbientLightColourForFrame->blue = p_CTimeCycle__m_CurrentColours->ambb;

        p_AmbientLightColourForFrame_PedsCarsAndObjects->red = p_CTimeCycle__m_CurrentColours->ambobjr;
        p_AmbientLightColourForFrame_PedsCarsAndObjects->green = p_CTimeCycle__m_CurrentColours->ambobjg;
        p_AmbientLightColourForFrame_PedsCarsAndObjects->blue = p_CTimeCycle__m_CurrentColours->ambobjb;

        if(*p_CWeather__LightningFlash)
        {
            p_AmbientLightColourForFrame->red   = 1.0f;
            p_AmbientLightColourForFrame->green = 1.0f;
            p_AmbientLightColourForFrame->blue  = 1.0f;

            p_AmbientLightColourForFrame_PedsCarsAndObjects->red   = 1.0f;
            p_AmbientLightColourForFrame_PedsCarsAndObjects->green = 1.0f;
            p_AmbientLightColourForFrame_PedsCarsAndObjects->blue  = 1.0f;
        }

        RpLightSetColor(*p_pAmbient, p_AmbientLightColourForFrame);
    }

    if(*p_pDirect)
    {
        (*p_pDirect)->isMainLight = 1;

        float dirMult = *p_CCoronas__LightsMult * p_CTimeCycle__m_CurrentColours->directionalmult;
        p_DirectionalLightColourForFrame->red   = dirMult;
        p_DirectionalLightColourForFrame->green = dirMult;
        p_DirectionalLightColourForFrame->blue  = dirMult;
        RpLightSetColor(*p_pDirect, p_DirectionalLightColourForFrame);

        RwMatrix mat;
        memset(&mat, 0, sizeof(mat));
        CVector vecsun = *p_CTimeCycle__m_vecDirnLightToSun;
        CVector vec1 = { 0.0f, 0.0f, 1.0f };
        CVector vec2 = CrossProduct(&vec1, &vecsun);
        VectorNormalise(&vec2);
        vec1 = CrossProduct(&vec2, &vecsun);
        mat.at.x = -vecsun.x;
        mat.at.y = -vecsun.y;
        mat.at.z = -vecsun.z;
        mat.right.x = vec1.x;
        mat.right.y = vec1.y;
        mat.right.z = vec1.z;
        mat.up.x = vec2.x;
        mat.up.y = vec2.y;
        mat.up.z = vec2.z;

        RwFrameTransform(RpLightGetParent(*p_pDirect), &mat, rwCOMBINEREPLACE);
        
        /*p_DirectionalLightColourFromDay->red   = p_CTimeCycle__m_CurrentColours->skytopr * 0.0002f;
        p_DirectionalLightColourFromDay->green = p_CTimeCycle__m_CurrentColours->skytopg * 0.0002f;
        p_DirectionalLightColourFromDay->blue  = p_CTimeCycle__m_CurrentColours->skytopb * 0.0002f;*/

        *p_DirectionalLightColourFromDay = *p_AmbientLightColourForFrame;
    }
}
DECL_HOOKv(emuPatch_glLightModelfv, GLenum en, float* v)
{
    if(en == GL_LIGHT_MODEL_AMBIENT)
    {
        if(memcmp(AmbientLightColor, v, 4*sizeof(float)) != 0)
        {
            memcpy(AmbientLightColor, v, 4*sizeof(float));
            *AmbientLightDirty = true;
        }
    }
}
DECL_HOOKv(SetLightsWithTimeOfDayColour, RpWorld *world)
{
    switch(g_nShading)
    {
        case SHADING_PC_PS2: return SetLightsWithTimeOfDayColour_PC_PS2(world);
        case SHADING_NONE: return SetLightsWithTimeOfDayColour_Simple(world);
        default: return SetLightsWithTimeOfDayColour(world);
    }
}

/* Main */
void StartShading()
{
    pCFGShading = cfg->Bind("Shading", SHADING_MOBILE, "Shading");
    ShadingSettingChanged(SHADING_MOBILE, pCFGShading->GetInt(), NULL);
    AddSetting("Shading Style", g_nShading, 0, sizeofA(aShadingSwitch)-1, aShadingSwitch, ShadingSettingChanged, NULL);
    
    //HOOKPLT(_rwOpenGLLightsSetMaterialProperties, pGTASA + BYBIT(0x67381C, 0x845F18)); // useless?
    //HOOKPLT(emuPatch_glLightModelfv, pGTASA + BYBIT(0x66F828, 0x83F740)); // Makes every possible thing alive dark as hell
    HOOKPLT(SetLightsWithTimeOfDayColour, pGTASA + BYBIT(0x674048, 0x846C88));
}