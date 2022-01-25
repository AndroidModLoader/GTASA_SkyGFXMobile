#pragma once

#include <stdint.h>
#include <cmath>


#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

typedef uint8_t RwUInt8;
typedef uint16_t RwUInt16;
typedef uint32_t RwUInt32;
typedef int8_t RwInt8;
typedef int16_t RwInt16;
typedef int32_t RwInt32;
typedef float RwReal;

typedef struct RwV3d RwV3d;
struct RwV3d
{
    RwReal x;   /**< X value */
    RwReal y;   /**< Y value */
    RwReal z;   /**< Z value */
};

typedef struct RwMatrix RwMatrix;
struct RwMatrix
{
    /* These are padded to be 16 byte quantities per line */
    RwV3d               right;
    RwUInt32            flags;
    RwV3d               up;
    RwUInt32            pad1;
    RwV3d               at;
    RwUInt32            pad2;
    RwV3d               pos;
    RwUInt32            pad3;
};

//typedef enum RwOpCombineType RwOpCombineType; // forbids forward references to 'enum' types :(
enum RwOpCombineType
{
    rwCOMBINEREPLACE = 0,   /**<Replace - 
                all previous transformations are lost */
    rwCOMBINEPRECONCAT,     /**<Pre-concatenation - 
                the given transformation is applied 
                before all others */
    rwCOMBINEPOSTCONCAT,    /**<Post-concatenation - 
                the given transformation is applied 
                after all others */
};

typedef struct RwRGBAReal RwRGBAReal;
struct RwRGBAReal
{
    RwReal red;     /**< red component */
    RwReal green;   /**< green component */
    RwReal blue;    /**< blue component */
    RwReal alpha;   /**< alpha component */
};

typedef struct RwRGBA RwRGBA;
struct RwRGBA
{
    RwUInt8 red;    /**< red component */
    RwUInt8 green;  /**< green component */
    RwUInt8 blue;   /**< blue component */
    RwUInt8 alpha;  /**< alpha component */
};

typedef struct RwLLLink RwLLLink;
struct RwLLLink
{
    RwLLLink *next;
    RwLLLink *prev;
};

typedef struct RwLinkList RwLinkList;
struct RwLinkList
{
    RwLLLink link;
};

typedef struct RwObject RwObject;
struct RwObject
{
    RwUInt8 type;                /**< Internal Use */
    RwUInt8 subType;             /**< Internal Use */
    RwUInt8 flags;               /**< Internal Use */
    RwUInt8 privateFlags;        /**< Internal Use */
    void   *parent;              /**< Internal Use */
                                     /* Often a Frame  */
};
#define rwObjectGetParent(object)           (((const RwObject *)(object))->parent)

typedef struct RwFrame RwFrame;
struct RwFrame
{
    RwObject            object;
    
    RwLLLink            inDirtyListLink;
    
    /* Put embedded matrices here to ensure they remain 16-byte aligned */
    RwMatrix            modelling;
    RwMatrix            ltm;
    
    RwLinkList          objectList; /* List of objects connected to a frame */
    
    struct RwFrame      *child;
    struct RwFrame      *next;
    struct RwFrame      *root;   /* Root of the tree */
};

typedef struct RwObjectHasFrame RwObjectHasFrame;
typedef RwObjectHasFrame * (*RwObjectHasFrameSyncFunction)(RwObjectHasFrame *object);
struct RwObjectHasFrame
{
    RwObject                     object;
    RwLLLink                     lFrame;
    RwObjectHasFrameSyncFunction sync;
};

typedef struct RpLight RpLight;
struct RpLight
{
    RwObjectHasFrame    object; /**< object */
    RwReal              radius; /**< radius */
    RwRGBAReal          color; /**< color */  /* Light color */
    RwReal              minusCosAngle; /**< minusCosAngle */  
    RwLinkList          WorldSectorsInLight; /**< WorldSectorsInLight */
    RwLLLink            inWorld; /**< inWorld */
    RwUInt16            lightFrame; /**< lightFrame */
    // war drum (?) change
    RwUInt8             spec;
    RwUInt8             pad;
//    RwUInt16            pad;
};
#define RpLightGetParent(light) ((RwFrame*)rwObjectGetParent(light))

typedef struct RwTexture RwTexture;
typedef struct RxPipeline RxPipeline;

typedef struct RwSurfaceProperties RwSurfaceProperties;
struct RwSurfaceProperties
{
    RwReal ambient;   /**< ambient reflection coefficient */
    RwReal specular;  /**< specular reflection coefficient */
    RwReal diffuse;   /**< reflection coefficient */
};

typedef struct RpMaterial RpMaterial;
struct RpMaterial
{
    RwTexture           *texture; /**< texture */
    RwRGBA              color; /**< color */              
    RxPipeline          *pipeline; /**< pipeline */     
    RwSurfaceProperties surfaceProps; /**< surfaceProps */
    RwInt16             refCount;          /* C.f. rwsdk/world/bageomet.h:RpGeometry */
    RwInt16             pad;
};

typedef struct CVector CVector;
struct CVector { float x, y, z; };

inline CVector CrossProduct(CVector *v1, CVector *v2)
{
    CVector cross;
    cross.x = v1->y*v2->z - v1->z*v2->y;
    cross.y = v1->z*v2->x - v1->x*v2->z;
    cross.z = v1->x*v2->y - v1->y*v2->x;
    return cross;
}

inline void VectorNormalise(CVector *vec)
{
    float lensq = vec->x*vec->x + vec->y*vec->y + vec->z*vec->z;
    if(lensq > 0.0f){
        float invsqrt = 1.0f/sqrtf(lensq);
        vec->x *= invsqrt;
        vec->y *= invsqrt;
        vec->z *= invsqrt;
    }else
        vec->x = 1.0f;
}



/* main.cpp */
#include <GLES2/gl2.h>
#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_SPECULAR 0x1202
#define GL_EMISSION 0x1600
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#define GL_COLOR_MATERIAL 0x0B57

extern RwFrame *(*RwFrameTransform)(RwFrame * frame, const RwMatrix * m, RwOpCombineType combine);
extern RpLight *(*RpLightSetColor)(RpLight *light, const RwRGBAReal *color);
extern int (* GetMobileEffectSetting)();
extern int* deviceChip;
extern void* pGTASA;
extern uintptr_t pGTASAAddr;
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
extern RpLight **p_pDirect;
extern RpLight **p_pAmbient;
extern RwRGBAReal *p_AmbientLightColourForFrame;
extern RwRGBAReal *p_AmbientLightColourForFrame_PedsCarsAndObjects;
extern RwRGBAReal *p_DirectionalLightColourForFrame;
extern RwRGBAReal *p_DirectionalLightColourFromDay;
extern int nColorFilter;

void SetLightsWithTimeOfDayColour(void *world);
void _rwOpenGLLightsSetMaterialProperties(const RpMaterial *mat, RwUInt32 flags);
void ColorFilter_stub(void);
void ColorfilterChanged(int oldVal, int newVal);