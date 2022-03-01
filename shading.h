#include "GTASA_STRUCTS.h"

#include <GLES2/gl2.h>
#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_SPECULAR 0x1202
#define GL_EMISSION 0x1600
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#define GL_COLOR_MATERIAL 0x0B57

extern RpLight **p_pAmbient;
extern RpLight **p_pDirect;
extern RwRGBAReal *p_AmbientLightColourForFrame;
extern RwRGBAReal *p_AmbientLightColourForFrame_PedsCarsAndObjects;
extern RwRGBAReal *p_DirectionalLightColourForFrame;
extern RwRGBAReal *p_DirectionalLightColourFromDay;
extern RwFrame *(*RwFrameTransform)(RwFrame * frame, const RwMatrix * m, RwOpCombineType combine);
extern RpLight *(*RpLightSetColor)(RpLight *light, const RwRGBAReal *color);
extern float *openglAmbientLight;
extern float _rwOpenGLOpaqueBlack[4];
extern RwInt32 *p_rwOpenGLColorMaterialEnabled;
extern void (*emu_glLightModelfv)(GLenum pname, const GLfloat *params);
extern void (*emu_glMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
extern void (*emu_glColorMaterial)(GLenum face, GLenum mode);
extern void (*emu_glEnable)(GLenum cap);
extern void (*emu_glDisable)(GLenum cap);
extern CColourSet *p_CTimeCycle__m_CurrentColours;
extern CVector *p_CTimeCycle__m_vecDirnLightToSun;
extern float *p_gfLaRiotsLightMult;
extern float *p_CCoronas__LightsMult;
extern uint8_t *p_CWeather__LightningFlash;

extern RwFrame *(*RwFrameTransform)(RwFrame * frame, const RwMatrix * m, RwOpCombineType combine);
extern RpLight *(*RpLightSetColor)(RpLight *light, const RwRGBAReal *color);
extern void (*emu_glLightModelfv)(GLenum pname, const GLfloat *params);
extern void (*emu_glMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
extern void (*emu_glColorMaterial)(GLenum face, GLenum mode);
extern void (*emu_glEnable)(GLenum cap);
extern void (*emu_glDisable)(GLenum cap);

void _rwOpenGLLightsSetMaterialProperties(const RpMaterial *mat, RwUInt32 flags);
void SetLightsWithTimeOfDayColour(void *world);