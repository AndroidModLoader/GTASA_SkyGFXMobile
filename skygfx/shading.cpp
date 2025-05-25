#include <externs.h>

/* Variables */
bool g_bPS2Shading = false;
const char* aShadingSwitch[2] = 
{
    "FEM_OFF",
    "FEM_ON",
};

/* Configs */
ConfigEntry* pCFGPS2Shading;

/* Functions */
void ShadingSettingChanged(int oldVal, int newVal, void* data)
{
    pCFGPS2Shading->SetBool(newVal != 0);
    g_bPS2Shading = pCFGPS2Shading->GetBool();
    
    if(g_bPS2Shading)
    {
        aml->PlaceNOP(pGTASA + 0x1C1382 + 0x1);
        aml->PlaceNOP(pGTASA + 0x1C13BA + 0x1);
    }
    else
    {
        aml->Write(pGTASA + 0x1C1382, "\x0B\xD4", 2);
        aml->Write(pGTASA + 0x1C13BA, "\x0B\xD0", 2);
    }

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
    if(!g_bPS2Shading)
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

DECL_HOOKv(SetLightsWithTimeOfDayColour, RpWorld *world)
{
    if(!g_bPS2Shading)
    {
        SetLightsWithTimeOfDayColour(world);
        return;
    }

    (*p_pDirect)->isMainLight = 1;
    if(*p_pAmbient)
    {
        float ambMult = *p_gfLaRiotsLightMult * *p_CCoronas__LightsMult;
        p_AmbientLightColourForFrame->red = p_CTimeCycle__m_CurrentColours->ambr * ambMult;
        p_AmbientLightColourForFrame->green = p_CTimeCycle__m_CurrentColours->ambg * ambMult;
        p_AmbientLightColourForFrame->blue = p_CTimeCycle__m_CurrentColours->ambb * ambMult;
        RpLightSetColor(*p_pAmbient, p_AmbientLightColourForFrame);

        float ambObjMult = *p_CCoronas__LightsMult;
        p_AmbientLightColourForFrame_PedsCarsAndObjects->red = p_CTimeCycle__m_CurrentColours->ambobjr * ambObjMult;
        p_AmbientLightColourForFrame_PedsCarsAndObjects->green = p_CTimeCycle__m_CurrentColours->ambobjg * ambObjMult;
        p_AmbientLightColourForFrame_PedsCarsAndObjects->blue = p_CTimeCycle__m_CurrentColours->ambobjb * ambObjMult;

        if(*p_CWeather__LightningFlash)
        {
            p_AmbientLightColourForFrame->red = 1.0f;
            p_AmbientLightColourForFrame->green = 1.0f;
            p_AmbientLightColourForFrame->blue = 1.0f;

            p_AmbientLightColourForFrame_PedsCarsAndObjects->red = 1.0f;
            p_AmbientLightColourForFrame_PedsCarsAndObjects->green = 1.0f;
            p_AmbientLightColourForFrame_PedsCarsAndObjects->blue = 1.0f;
        }
        // this is used by objects with alpha test for whatever reason
        *p_DirectionalLightColourFromDay = *p_AmbientLightColourForFrame;
    }

    if(*p_pDirect)
    {
        float dirMult = 1.00392156863f * *p_CCoronas__LightsMult;
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
    }
}

/* Main */
void StartShading()
{
    pCFGPS2Shading = cfg->Bind("PS2Shading", false, "Shading");
    g_bPS2Shading = pCFGPS2Shading->GetBool();
    if(g_bPS2Shading) ShadingSettingChanged(0, 1, NULL);
    
    HOOKPLT(_rwOpenGLLightsSetMaterialProperties, pGTASA + BYBIT(0x67381C, 0x845F18));
    HOOKPLT(SetLightsWithTimeOfDayColour, pGTASA + BYBIT(0x674048, 0x846C88));

    AddSetting("PS2 Shading", g_bPS2Shading, 0, sizeofA(aShadingSwitch)-1, aShadingSwitch, ShadingSettingChanged, NULL);
}