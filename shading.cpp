#include <gtasa_things.h>
#include <string> // memset

#include <shading.h>

RpLight **p_pDirect;
RpLight **p_pAmbient;
RwRGBAReal *p_AmbientLightColourForFrame;
RwRGBAReal *p_AmbientLightColourForFrame_PedsCarsAndObjects;
RwRGBAReal *p_DirectionalLightColourForFrame;
RwRGBAReal *p_DirectionalLightColourFromDay;

RwFrame *(*RwFrameTransform)(RwFrame * frame, const RwMatrix * m, RwOpCombineType combine);
RpLight *(*RpLightSetColor)(RpLight *light, const RwRGBAReal *color);
void (*emu_glLightModelfv)(GLenum pname, const GLfloat *params);
void (*emu_glMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
void (*emu_glColorMaterial)(GLenum face, GLenum mode);
void (*emu_glEnable)(GLenum cap);
void (*emu_glDisable)(GLenum cap);

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

void _rwOpenGLLightsSetMaterialProperties(const RpMaterial *mat, RwUInt32 flags)
{
    float surfProps[4];
    float colorScale[4];
    surfProps[0] = mat->surfaceProps.ambient;
    surfProps[1] = mat->surfaceProps.diffuse;
    // could use for env and spec data perhaps
    surfProps[2] = 0.0f;
    surfProps[3] = 0.0f;
    colorScale[0] = 0.00392156862f * mat->color.red;
    colorScale[1] = 0.00392156862f * mat->color.green;
    colorScale[2] = 0.00392156862f * mat->color.blue;
    colorScale[3] = 0.00392156862f * mat->color.alpha;
    // repurposing material colors here
    emu_glMaterialfv(GL_FRONT, GL_AMBIENT, surfProps);
    emu_glMaterialfv(GL_FRONT, GL_DIFFUSE, colorScale);

    // multiplied by 1.5 internally, let's undo that
    float ambHack[4];
    ambHack[0] = 0.66666666667f * openglAmbientLight[0];
    ambHack[1] = 0.66666666667f * openglAmbientLight[1];
    ambHack[2] = 0.66666666667f * openglAmbientLight[2];
    ambHack[3] = 0.66666666667f * openglAmbientLight[3];
    emu_glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambHack);

    if(flags & rxGEOMETRY_PRELIT) // prelight
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

void SetLightsWithTimeOfDayColour(void *world)
{
    (*p_pDirect)->spec = 1;
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