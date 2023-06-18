#pragma once

#define FLAG_ALPHA_TEST           0x01
#define FLAG_LIGHTING             0x02
#define FLAG_ALPHA_MODULATE       0x04
#define FLAG_COLOR_EMISSIVE       0x08
#define FLAG_COLOR                0x10
#define FLAG_TEX0                 0x20
#define FLAG_ENVMAP               0x40          // normal envmap
#define FLAG_BONE3                0x80
#define FLAG_BONE4                0x100
#define FLAG_CAMERA_BASED_NORMALS 0x200
#define FLAG_FOG                  0x400
#define FLAG_TEXBIAS              0x800
#define FLAG_BACKLIGHT            0x1000
#define FLAG_LIGHT1               0x2000
#define FLAG_LIGHT2               0x4000
#define FLAG_LIGHT3               0x8000
#define FLAG_DETAILMAP            0x10000
#define FLAG_COMPRESSED_TEXCOORD  0x20000
#define FLAG_PROJECT_TEXCOORD     0x40000
#define FLAG_WATER                0x80000
#define FLAG_COLOR2               0x100000
#define FLAG_SPHERE_XFORM         0x800000      // this renders the scene as a sphere map for vehicle reflections
#define FLAG_SPHERE_ENVMAP        0x1000000     // spherical real-time envmap
#define FLAG_TEXMATRIX            0x2000000
#define FLAG_GAMMA                0x4000000

//#define PXL_EMIT(__v) strcat(pxlbuf, __v "\n")
//#define VTX_EMIT(__v) strcat(vtxbuf, __v "\n")
//#define PXL_EMIT_V(...)                      \
//  do {                                       \
//    snprintf(tmp, sizeof(tmp), __VA_ARGS__); \
//    strcat(pxlbuf, tmp);                     \
//    strcat(pxlbuf, "\n");                    \
//  } while (0)
//
//
//#define VTX_EMIT_V(...)                      \
//  do {                                       \
//    snprintf(tmp, sizeof(tmp), __VA_ARGS__); \
//    strcat(vtxbuf, tmp);                     \
//    strcat(vtxbuf, "\n");                    \
//  } while (0)

#define PXL_EMIT(__v) t = concatf(t, __v "\n", sizeof(__v)+1)
#define VTX_EMIT(__v) t = concatf(t, __v "\n", sizeof(__v)+1)

#define PXL_EMIT_V(...)                      \
  do {                                       \
    snprintf(tmp, sizeof(tmp), __VA_ARGS__); \
    t=concatf(t, tmp, strlen(tmp)+1);        \
    t=concatf(t, "\n", 2);                   \
  } while (0)


#define VTX_EMIT_V(...)                      \
  do {                                       \
    snprintf(tmp, sizeof(tmp), __VA_ARGS__); \
    t=concatf(t, tmp, strlen(tmp)+1);        \
    t=concatf(t, "\n", 2);                   \
  } while (0)

void BuildVertexSource_Reversed(int flags);
void BuildPixelSource_Reversed(int flags);

void BuildVertexSource_SkyGfx(int flags);
void BuildPixelSource_SkyGfx(int flags);

void PatchShaders();