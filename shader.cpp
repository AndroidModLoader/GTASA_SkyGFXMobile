#include <string>
#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <gtasa_things.h>

#include <fastcat.h>
#include <shader.h>

int* deviceChip;
int* RasterExtOffset;
int* detailTexturesStorage;
int* textureDetail;
float *openglAmbientLight;
float _rwOpenGLOpaqueBlack[4];
RwInt32 *p_rwOpenGLColorMaterialEnabled;
CColourSet *p_CTimeCycle__m_CurrentColours;
CVector *p_CTimeCycle__m_vecDirnLightToSun;
float *p_gfLaRiotsLightMult;
float *p_CCoronas__LightsMult;
uint8_t *p_CWeather__LightningFlash;
float *skin_map;
int *skin_dirty;
int *skin_num;
int (* GetMobileEffectSetting)();

//extern ConfigEntry* pBonesOptimization;
//extern ConfigEntry* pMVPOptimization;
extern ConfigEntry* pDisablePedSpec;
extern ConfigEntry* pPS2Sun;
extern ConfigEntry* pPS2Shading;
extern ConfigEntry* pPS2Reflections;
extern ConfigEntry* pExponentialFog;

char pxlbuf[8192], vtxbuf[8192];

RQCapabilities *RQCaps;
int *RQMaxBones;

int OS_SystemChip()
{
    return *deviceChip;
}

void BuildPixelSource_SkyGfx(int flags)
{
    char tmp[512];
    char* t = pxlbuf;
    int ped_spec = pDisablePedSpec->GetBool() ? 0 : (FLAG_BONE3 | FLAG_BONE4);
    
    PXL_EMIT("#version 100");
    PXL_EMIT("precision mediump float;");
    if(flags & FLAG_TEX0)
    {
        PXL_EMIT("uniform sampler2D Diffuse;\nvarying mediump vec2 Out_Tex0;");
    }
    if(flags & (FLAG_SPHERE_ENVMAP | FLAG_ENVMAP))
    {
        PXL_EMIT("uniform sampler2D EnvMap;\nuniform lowp float EnvMapCoefficient;");
        if(flags & FLAG_ENVMAP)
        {
            PXL_EMIT("varying mediump vec2 Out_Tex1;");
        }
        else
        {
            PXL_EMIT("varying mediump vec3 Out_Refl;");
        }
    }
    else if(flags & FLAG_DETAILMAP)
    {
        PXL_EMIT("uniform sampler2D EnvMap;\nuniform float DetailTiling;");
    }
    
    if(flags & FLAG_FOG)
    {
        PXL_EMIT("varying mediump float Out_FogAmt;\nuniform lowp vec3 FogColor;");
    }
        
    if(flags & (FLAG_LIGHTING | FLAG_COLOR))
    {
        PXL_EMIT("varying lowp vec4 Out_Color;");
    }
        
    if ((flags & FLAG_LIGHT1) || (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
    {
        PXL_EMIT("varying lowp vec4 Out_Spec;");
    }

    if(flags & FLAG_ALPHA_MODULATE)
    {
        PXL_EMIT("uniform lowp float AlphaModulate;");
    }

    if(flags & FLAG_WATER)
        PXL_EMIT("varying mediump vec2 Out_WaterDetail;\nvarying mediump vec3 Out_WaterDetail2;");

    ///////////////////////////////////////
    //////////////// START ////////////////
    ///////////////////////////////////////
    PXL_EMIT("void main() {");

    PXL_EMIT("\tlowp vec4 fcolor;");
    if(flags & FLAG_TEX0)
    {
        if(flags & FLAG_TEXBIAS)
        {
            PXL_EMIT("\tlowp vec4 diffuseColor = texture2D(Diffuse, Out_Tex0, -1.5);");
        }
        else if(RQCaps->isSlowGPU != 0)
        {
            PXL_EMIT("\tlowp vec4 diffuseColor = texture2D(Diffuse, Out_Tex0);");
        }
        else
        {
            PXL_EMIT("\tlowp vec4 diffuseColor = texture2D(Diffuse, Out_Tex0, -0.5);");
        }
        PXL_EMIT("\tfcolor = diffuseColor;");

        if(flags & (FLAG_COLOR | FLAG_LIGHTING))
        {
            if(flags & FLAG_DETAILMAP)
            {
                if(!(flags & FLAG_WATER))
                {
                    PXL_EMIT("\tfcolor *= vec4(Out_Color.xyz * texture2D(EnvMap, Out_Tex0.xy * DetailTiling, -0.5).xyz * 2.0, Out_Color.w);");
                    if(flags & FLAG_ENVMAP)
                    {
                        PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
                    }
                    goto LABEL_45; // CHECK
                }
                PXL_EMIT("\tfloat waterDetail = texture2D(EnvMap, Out_WaterDetail, -1.0).x + texture2D(EnvMap, Out_WaterDetail2.xy, -1.0).x;");
                PXL_EMIT("\tfcolor *= vec4(Out_Color.xyz * waterDetail * 1.1, Out_Color.w);");
                PXL_EMIT("\tfcolor.a += Out_WaterDetail2.z;");
                if(flags & FLAG_ENVMAP)
                {
                    PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
                }
                goto LABEL_45; // CHECK
            }
            PXL_EMIT("\tfcolor *= Out_Color;");
        }
        if(!(flags & FLAG_WATER))
        {
            if(flags & FLAG_ENVMAP)
            {
                PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
            }
            goto LABEL_45; // CHECK
        }
        PXL_EMIT("\tfcolor.a += Out_WaterDetail2.z;");
        if(flags & FLAG_ENVMAP)
        {
            PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
        }
        goto LABEL_45; // CHECK
    }

    if(flags & (FLAG_COLOR | FLAG_LIGHTING))
    {
        PXL_EMIT("\tfcolor = Out_Color;");
    }
    else
    {
        PXL_EMIT("\tfcolor = 0.0;");
    }
    if(flags & FLAG_ENVMAP)
    {
        PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
    }

LABEL_45:
    if (!RQCaps->unk_08)
    {
        if ((flags & FLAG_LIGHT1))
        {
            // Env map
            if (flags & FLAG_ENVMAP)
            {
                PXL_EMIT("fcolor.xyz += texture2D(EnvMap, Out_Tex1).xyz * Out_Spec.w;");
            }
            else if (flags & FLAG_SPHERE_ENVMAP)
            {
                PXL_EMIT("lowp vec2 ReflPos = normalize(Out_Refl.xy) * (Out_Refl.z * 0.5 + 0.5);"); // DID: lowp
                PXL_EMIT("ReflPos = (ReflPos * 0.5) + 0.5;");
                PXL_EMIT("lowp vec4 ReflTexture = texture2D(EnvMap, ReflPos);");
                PXL_EMIT("fcolor.xyz = mix(fcolor.xyz, ReflTexture.xyz, EnvMapCoefficient);");
                PXL_EMIT("fcolor.w += ReflTexture.b * 0.125;");
            }

            // Spec light
            if(flags & FLAG_ENVMAP)
            {
                // PS2-style specdot
                // We don't actually have the texture. so simulate it
                PXL_EMIT("lowp vec2 unpack = (Out_Spec.xy-0.5)*2.0;"); // DID: lowp
                PXL_EMIT("lowp vec3 specColor = vec3(Out_Spec.z, Out_Spec.z, Out_Spec.z);"); // DID: lowp
                PXL_EMIT("lowp float dist = unpack.x*unpack.x + unpack.y*unpack.y;"); // DID: lowp
                // outside the dot
                PXL_EMIT("if(dist > 0.69*0.69) specColor *= 0.0;");
                // smooth the edge
                PXL_EMIT("else if(dist > 0.67*0.67) specColor *= (0.69*0.69 - dist)/(0.69*0.69 - 0.67*0.67);");
                PXL_EMIT("fcolor.xyz += specColor;");
            }
            else if(flags & (FLAG_SPHERE_ENVMAP | ped_spec))
            {
                // Out_Spec is actually light
                PXL_EMIT("fcolor.xyz += Out_Spec.xyz;");
            }
        }
        if (flags & FLAG_FOG)
            PXL_EMIT("fcolor.xyz = mix(fcolor.xyz, FogColor, Out_FogAmt);");
    }

    if(flags & FLAG_GAMMA)
    {
        PXL_EMIT("\tfcolor.xyz += fcolor.xyz * 0.5;");
    }

    PXL_EMIT("\tgl_FragColor = fcolor;");
    if(flags & FLAG_ALPHA_TEST)
    {
        PXL_EMIT("/*ATBEGIN*/");
        if ((OS_SystemChip() == 13) && (flags & FLAG_TEX0))
        {
            if (flags & FLAG_TEXBIAS)
            {
                PXL_EMIT("\tif (diffuseColor.a < 0.8) { discard; }");
            }
            else
            {
                if (!(flags & FLAG_CAMERA_BASED_NORMALS))
                {
                    PXL_EMIT("\tif (diffuseColor.a < 0.2) { discard; }");
                }
                else
                {
                    PXL_EMIT("\tgl_FragColor.a = Out_Color.a;");
                    PXL_EMIT("\tif (diffuseColor.a < 0.5) { discard; }");
                }
            }
        }
        else
        {
            if (flags & FLAG_TEXBIAS)
            {
                PXL_EMIT("\tif (diffuseColor.a < 0.8) { discard; }");
            }
            else
            {
                if (!(flags & FLAG_CAMERA_BASED_NORMALS))
                {
                    PXL_EMIT("\tif (diffuseColor.a < 0.2) { discard; }");
                }
                else
                {
                    PXL_EMIT("\tif (diffuseColor.a < 0.5) { discard; }");
                    PXL_EMIT("\tgl_FragColor.a = Out_Color.a;");
                }
            }
        }
        PXL_EMIT("/*ATEND*/");
    }

    if(flags & FLAG_ALPHA_MODULATE)
        PXL_EMIT("\tgl_FragColor.a *= AlphaModulate;");

    PXL_EMIT("}");
    ///////////////////////////////////////
    ////////////////  END  ////////////////
    ///////////////////////////////////////
}

void BuildVertexSource_SkyGfx(int flags)
{
    char tmp[512];
    char* t = vtxbuf;
    int ped_spec = pDisablePedSpec->GetBool() ? 0 : (FLAG_BONE3 | FLAG_BONE4);
    
    VTX_EMIT("#version 100");
    VTX_EMIT("precision highp float;\nuniform mat4 ProjMatrix;\nuniform mat4 ViewMatrix;\nuniform mat4 ObjMatrix;");
    if(flags & FLAG_LIGHTING)
    {
        VTX_EMIT("uniform lowp vec3 AmbientLightColor;\nuniform lowp vec4 MaterialEmissive;");
        VTX_EMIT("uniform lowp vec4 MaterialAmbient;\nuniform lowp vec4 MaterialDiffuse;");
        if(flags & FLAG_LIGHT1)
        {
            VTX_EMIT("uniform lowp vec3 DirLightDiffuseColor;\nuniform mediump vec3 DirLightDirection;"); // Optimized DirLightDirection with mediump
            if (GetMobileEffectSetting() == 3 && (flags & (FLAG_BACKLIGHT | FLAG_BONE3 | FLAG_BONE4)))
            {
                VTX_EMIT("uniform mediump vec3 DirBackLightDirection;"); // Optimized DirBackLightDirection with mediump
            }
        }
        if(flags & FLAG_LIGHT2)
        {
            VTX_EMIT("uniform lowp vec3 DirLight2DiffuseColor;\nuniform mediump vec3 DirLight2Direction;"); // Optimized DirLight2Direction with mediump
        }
        if(flags & FLAG_LIGHT3)
        {
            VTX_EMIT("uniform lowp vec3 DirLight3DiffuseColor;\nuniform mediump vec3 DirLight3Direction;"); // Optimized DirLight3Direction with mediump
        }
    }
    
    VTX_EMIT("attribute vec3 Position;\nattribute vec3 Normal;");
    if(flags & FLAG_TEX0)
    {
        if(flags & FLAG_PROJECT_TEXCOORD)
            VTX_EMIT("attribute vec4 TexCoord0;");
        else
            VTX_EMIT("attribute vec2 TexCoord0;");
    }

    VTX_EMIT("attribute vec4 GlobalColor;");
    
    if(flags & (FLAG_BONE3 | FLAG_BONE4))
    {
        VTX_EMIT("attribute vec4 BoneWeight;\nattribute vec4 BoneIndices;");
        /*if(pBonesOptimization->GetBool())
            VTX_EMIT("uniform highp mat4 Bones[96];");
        else*/
            VTX_EMIT_V("uniform highp vec4 Bones[%d];", 3 * *RQMaxBones);
    }
    
    if(flags & FLAG_TEX0)
    {
        VTX_EMIT("varying mediump vec2 Out_Tex0;");
    }
    
    if(flags & FLAG_TEXMATRIX)
    {
        VTX_EMIT("uniform mat3 NormalMatrix;");
    }

    if(flags & (FLAG_SPHERE_ENVMAP | FLAG_ENVMAP))
    {
        if(flags & FLAG_ENVMAP)
            VTX_EMIT("varying mediump vec2 Out_Tex1;");
        else
            VTX_EMIT("varying mediump vec3 Out_Refl;");
        VTX_EMIT("uniform lowp float EnvMapCoefficient;");
    }

    if(flags & (FLAG_SPHERE_ENVMAP | FLAG_SPHERE_XFORM | FLAG_WATER | FLAG_FOG | FLAG_CAMERA_BASED_NORMALS | FLAG_ENVMAP | ped_spec))
    {
        VTX_EMIT("uniform vec3 CameraPosition;");
    }

    if(flags & FLAG_FOG)
    {
        VTX_EMIT("varying mediump float Out_FogAmt;");
        VTX_EMIT("uniform vec3 FogDistances;");
    }

    if(flags & FLAG_WATER)
    {
        VTX_EMIT("uniform vec3 WaterSpecs;");
        VTX_EMIT("varying mediump vec2 Out_WaterDetail;");
        VTX_EMIT("varying mediump vec3 Out_WaterDetail2;"); // Optimized (became vec3)
        //VTX_EMIT("varying mediump float Out_WaterAlphaBlend;"); // Optimized
    }

    if(flags & FLAG_COLOR2)
    {
        VTX_EMIT("attribute vec4 Color2;");
        VTX_EMIT("uniform lowp float ColorInterp;");
    }

    if(flags & (FLAG_COLOR | FLAG_LIGHTING))
    {
        VTX_EMIT("varying lowp vec4 Out_Color;");
    }

    if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
    {
        VTX_EMIT("varying lowp vec4 Out_Spec;");
    }

    VTX_EMIT("#define SurfAmb (MaterialAmbient.x)");
    VTX_EMIT("#define SurfDiff (MaterialAmbient.y)");
    VTX_EMIT("#define ColorScale (MaterialDiffuse)");

    ///////////////////////////////////////
    //////////////// START ////////////////
    ///////////////////////////////////////
    VTX_EMIT("void main() {");

    if(flags & (FLAG_BONE4 | FLAG_BONE3))
    {
        VTX_EMIT("\tivec4 BlendIndexArray = ivec4(BoneIndices);");
        VTX_EMIT("\tmat4 BoneToLocal;");

        /*if(pBonesOptimization->GetBool())
        {
            VTX_EMIT("BoneToLocal = Bones[BlendIndexArray.x] * BoneWeight.x;");
            VTX_EMIT("BoneToLocal += Bones[BlendIndexArray.y] * BoneWeight.y;");
            VTX_EMIT("BoneToLocal += Bones[BlendIndexArray.z] * BoneWeight.z;");
            if (flags & FLAG_BONE4)
                VTX_EMIT("BoneToLocal += Bones[BlendIndexArray.w] * BoneWeight.w;");
            VTX_EMIT("BoneToLocal[0][3] = 0.0;");
            VTX_EMIT("BoneToLocal[1][3] = 0.0;");
            VTX_EMIT("BoneToLocal[2][3] = 0.0;");
            VTX_EMIT("BoneToLocal[3][3] = 1.0;");
        }
        else*/
        {
            VTX_EMIT("\tBoneToLocal[0] = Bones[BlendIndexArray.x*3] * BoneWeight.x;");
            VTX_EMIT("\tBoneToLocal[1] = Bones[BlendIndexArray.x*3+1] * BoneWeight.x;");
            VTX_EMIT("\tBoneToLocal[2] = Bones[BlendIndexArray.x*3+2] * BoneWeight.x;");
            VTX_EMIT("\tBoneToLocal[3] = vec4(0.0,0.0,0.0,1.0);");
            VTX_EMIT("\tBoneToLocal[0] += Bones[BlendIndexArray.y*3] * BoneWeight.y;");
            VTX_EMIT("\tBoneToLocal[1] += Bones[BlendIndexArray.y*3+1] * BoneWeight.y;");
            VTX_EMIT("\tBoneToLocal[2] += Bones[BlendIndexArray.y*3+2] * BoneWeight.y;");
            VTX_EMIT("\tBoneToLocal[0] += Bones[BlendIndexArray.z*3] * BoneWeight.z;");
            VTX_EMIT("\tBoneToLocal[1] += Bones[BlendIndexArray.z*3+1] * BoneWeight.z;");
            VTX_EMIT("\tBoneToLocal[2] += Bones[BlendIndexArray.z*3+2] * BoneWeight.z;");

            if(flags & FLAG_BONE4)
            {
                VTX_EMIT("\tBoneToLocal[0] += Bones[BlendIndexArray.w*3] * BoneWeight.w;");
                VTX_EMIT("\tBoneToLocal[1] += Bones[BlendIndexArray.w*3+1] * BoneWeight.w;");
                VTX_EMIT("\tBoneToLocal[2] += Bones[BlendIndexArray.w*3+2] * BoneWeight.w;");
            }
        }

        VTX_EMIT("\tvec4 WorldPos = ObjMatrix * (vec4(Position, 1.0) * BoneToLocal);");
    }
    else
    {
        VTX_EMIT("\tvec4 WorldPos = ObjMatrix * vec4(Position, 1.0);");
    }

    if(flags & FLAG_SPHERE_XFORM)
    {
        VTX_EMIT("\tvec3 ReflVector = WorldPos.xyz - CameraPosition.xyz;");
        VTX_EMIT("\tvec3 ReflPos = normalize(ReflVector);");
        VTX_EMIT("\tReflPos.xy = normalize(ReflPos.xy) * (ReflPos.z * 0.5 + 0.5);");
        VTX_EMIT("\tgl_Position = vec4(ReflPos.xy, length(ReflVector) * 0.002, 1.0);");
    }
    else
    {
        VTX_EMIT("\tvec4 ViewPos = ViewMatrix * WorldPos;");
        VTX_EMIT("\tgl_Position = ProjMatrix * ViewPos;");
    }

    if(flags & FLAG_LIGHTING)
    {
        if((flags & (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST)) == (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST) && (flags & (FLAG_LIGHT3 | FLAG_LIGHT2 | FLAG_LIGHT1)))
        {
            VTX_EMIT("vec3 WorldNormal = normalize(vec3(WorldPos.xy - CameraPosition.xy, 0.0001)) * 0.85;");
        }
        else if(flags & (FLAG_BONE4 | FLAG_BONE3))
        {
            VTX_EMIT("vec3 WorldNormal = mat3(ObjMatrix) * (Normal * mat3(BoneToLocal));");
        }
        else
        {
            VTX_EMIT("vec3 WorldNormal = (ObjMatrix * vec4(Normal, 0.0)).xyz;"); // Optimized below
            //VTX_EMIT("vec3 WorldNormal = Normal * mat3(ObjMatrix);");
        }
    }
    else
    {
        if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
            VTX_EMIT("vec3 WorldNormal = vec3(0.0, 0.0, 0.0);");
    }

    if(!RQCaps->unk_08 && (flags & FLAG_FOG))
    {
        if(pExponentialFog->GetBool()) VTX_EMIT("float fogDist = length(WorldPos.xyz - CameraPosition.xyz);\nOut_FogAmt = clamp(1.0 - exp2(-1.7 * FogDistances.z*FogDistances.z * fogDist*fogDist * 1.442695), 0.0, 1.0);");
        else VTX_EMIT("Out_FogAmt = clamp((length(WorldPos.xyz - CameraPosition.xyz) - FogDistances.x) * FogDistances.z, 0.0, 0.90);");
    }

    const char* _tmp;
    if(flags & FLAG_TEX0)
    {
        if(flags & FLAG_PROJECT_TEXCOORD)
        {
            _tmp = "TexCoord0.xy / TexCoord0.w";
        }
        else if(flags & FLAG_COMPRESSED_TEXCOORD)
        {
            _tmp = "TexCoord0 / 512.0";
        }
        else
        {
            _tmp = "TexCoord0";
        }

        if(flags & FLAG_TEXMATRIX)
        {
            VTX_EMIT_V("Out_Tex0 = (NormalMatrix * vec3(%s, 1.0)).xy;", _tmp); // Optimized below
            //VTX_EMIT_V("Out_Tex0 = %s * mat2(NormalMatrix);", _tmp);
        }
        else
        {
            VTX_EMIT_V("Out_Tex0 = %s;", _tmp);
        }
    }

    if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
    {
        VTX_EMIT("vec3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
        VTX_EMIT("reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
        if(flags & FLAG_SPHERE_ENVMAP)
        {
            VTX_EMIT("Out_Refl = reflVector;");
        }
        else
        {
            VTX_EMIT("Out_Tex1 = vec2(length(reflVector.xy), (reflVector.z * 0.5) + 0.25);");
        }
    }

    if(flags & FLAG_COLOR2)
    {
        VTX_EMIT("lowp vec4 InterpColor = mix(GlobalColor, Color2, ColorInterp);");
        _tmp = "InterpColor";
    }
    else
    {
        _tmp = "GlobalColor";
    }

    // LIGHTING (NEW_LIGHTING)
    if(flags & FLAG_LIGHTING)
    {
        VTX_EMIT("vec3 AmbEmissLight, DiffLight;");
        if(flags & FLAG_COLOR_EMISSIVE)
        {
            if(flags & FLAG_CAMERA_BASED_NORMALS)
            {
                VTX_EMIT_V("AmbEmissLight += clamp(%s.xyz, 0.0, 0.5);", _tmp);
            }
            else
            {
                VTX_EMIT_V("AmbEmissLight += %s.xyz;", _tmp);
            }
        }
        VTX_EMIT("AmbEmissLight += AmbientLightColor * SurfAmb;");

        // DIFFUSE
        if(flags & (FLAG_LIGHT3 | FLAG_LIGHT2 | FLAG_LIGHT1))
        {
            if(flags & FLAG_LIGHT1)
            {
                if(GetMobileEffectSetting() == 3 && (flags & (FLAG_BACKLIGHT | FLAG_BONE4 | FLAG_BONE3)))
                {
                    VTX_EMIT("DiffLight += (max(dot(DirLightDirection, WorldNormal), 0.0) + max(dot(DirBackLightDirection, WorldNormal), 0.0)) * DirLightDiffuseColor;");
                }
                else
                {
                    VTX_EMIT("DiffLight += max(dot(DirLightDirection, WorldNormal), 0.0) * DirLightDiffuseColor;");
                }
            }
            if(flags & FLAG_LIGHT2)
            {
                VTX_EMIT("DiffLight += max(dot(DirLight2Direction, WorldNormal), 0.0) * DirLight2DiffuseColor;");
            }
            if(flags & FLAG_LIGHT3)
            {
                VTX_EMIT("DiffLight += max(dot(DirLight3Direction, WorldNormal), 0.0) * DirLight3DiffuseColor;");
            }
            VTX_EMIT("DiffLight *= SurfDiff;");
        }

        // FINAL COLOR
        if(flags & (FLAG_COLOR | FLAG_LIGHTING))
        {
            VTX_EMIT("Out_Color.xyz = AmbEmissLight + DiffLight;");
            if(flags & FLAG_COLOR2)
                VTX_EMIT("Out_Color.w = Color2.w;");
            else
                VTX_EMIT("Out_Color.w = GlobalColor.w;");
            VTX_EMIT("Out_Color = clamp(Out_Color * ColorScale, 0.0, 1.0);");
        }
    }
    else if (flags & (FLAG_COLOR | FLAG_LIGHTING))
    {
        VTX_EMIT_V("Out_Color = %s;", _tmp);
    }

    // SPECULAR LIGHT
    if (!RQCaps->unk_08 && (flags & FLAG_LIGHT1))
    {
        if(pPS2Reflections->GetBool())
        {
            if (flags & FLAG_ENVMAP)
            {
                // Low quality setting -- PS2 style
                // PS2 specdot - reflect in view space
                VTX_EMIT("vec3 ViewNormal = WorldNormal * mat3(ViewMatrix);");
                VTX_EMIT("vec3 ViewLight = DirLightDirection * mat3(ViewMatrix);");
                VTX_EMIT("vec3 V = ViewLight - 2.0*ViewNormal*dot(ViewNormal, ViewLight);");
                // find some nice specular value -- not the real thing unfortunately
                VTX_EMIT("float specAmt = 1.0 * EnvMapCoefficient * DirLightDiffuseColor.x;");
                // NB: this is not a color here!!
                VTX_EMIT("Out_Spec.xyz = (V + vec3(1.0, 1.0, 0.0))/2.0;");
                VTX_EMIT("if(Out_Spec.z < 0.0) Out_Spec.z = specAmt; else Out_Spec.z = 0.0;");
                // need the light multiplier from here
                VTX_EMIT("Out_Spec.w = EnvMapCoefficient * DirLightDiffuseColor.x;");
            }
            else if (flags & FLAG_SPHERE_ENVMAP)
            {
                // Detailed & Max quality setting - original android (for now)
                //VTX_EMIT_V("float specAmt = pow(max(dot(reflVector, DirLightDirection), 0.0), %.1f) * EnvMapCoefficient * 2.0;", RQCaps->isMaliChip ? 9.0f : 10.0f); // Broken! It's always 9 or 10!
                VTX_EMIT_V("float specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * EnvMapCoefficient * 2.0;", RQCaps->isMaliChip ? 9.0f : 10.0f);
                VTX_EMIT("Out_Spec.xyz = specAmt * DirLightDiffuseColor;");
                VTX_EMIT("Out_Spec.w = EnvMapCoefficient * DirLightDiffuseColor.x;");
            }
            else if (flags & ped_spec)
            {
                VTX_EMIT("vec3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
                VTX_EMIT("reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
                VTX_EMIT_V("float specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * 0.125;", RQCaps->isMaliChip ? 5.0f : 4.0f);
                VTX_EMIT("Out_Spec.xyz = specAmt * DirLightDiffuseColor;");
                VTX_EMIT("Out_Spec.w = 0.0;");
            }
        }
        else
        {
            // Default Reflections
            if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
            {
                VTX_EMIT_V("float specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * EnvMapCoefficient * 2.0;", RQCaps->isMaliChip ? 9.0f : 10.0f);
            }
            else
            {
                if(!(flags & ped_spec)) goto LABEL_107;
                VTX_EMIT("vec3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
                VTX_EMIT("reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
                VTX_EMIT_V("float specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * 0.125;", RQCaps->isMaliChip ? 5.0f : 4.0f);
            }
            VTX_EMIT("Out_Spec.xyz = specAmt * DirLightDiffuseColor;");
            //VTX_EMIT("Out_Spec.w = EnvMapCoefficient * DirLightDiffuseColor.x;");
        }
    }

LABEL_107:
    if(flags & FLAG_WATER)
    {
        VTX_EMIT("Out_WaterDetail = (Out_Tex0 * 4.0) + vec2(WaterSpecs.x * -0.3, WaterSpecs.x * 0.21);");
        VTX_EMIT("Out_WaterDetail2.xy = (Out_Tex0 * -8.0) + vec2(WaterSpecs.x * 0.12, WaterSpecs.x * -0.05);"); // Optimized (became vec3)
        VTX_EMIT("Out_WaterDetail2.z = distance(WorldPos.xy, CameraPosition.xy) * WaterSpecs.y;"); // Optimized (became vec3)
    }

    VTX_EMIT("}");
    ///////////////////////////////////////
    ////////////////  END  ////////////////
    ///////////////////////////////////////
}














///////////////////////////////////////////////////
///////////////////   REVERSED   //////////////////
///////////////////////////////////////////////////
void BuildPixelSource_Reversed(int flags)
{
    char tmp[512];
    char* t = pxlbuf;
    int ped_spec = pDisablePedSpec->GetBool() ? 0 : (FLAG_BONE3 | FLAG_BONE4);
    
    PXL_EMIT("#version 100");
    PXL_EMIT("precision mediump float;");
    if(flags & FLAG_TEX0)
        PXL_EMIT("uniform sampler2D Diffuse;\nvarying mediump vec2 Out_Tex0;");
    if(flags & (FLAG_SPHERE_ENVMAP | FLAG_ENVMAP))
    {
        PXL_EMIT("uniform sampler2D EnvMap;\nuniform lowp float EnvMapCoefficient;");
        if(flags & FLAG_ENVMAP)
            PXL_EMIT("varying mediump vec2 Out_Tex1;");
        else
            PXL_EMIT("varying mediump vec3 Out_Refl;");
    }
    else if(flags & FLAG_DETAILMAP)
    {
        PXL_EMIT("uniform sampler2D EnvMap;\nuniform float DetailTiling;");
    }
    
    if(flags & FLAG_FOG)
        PXL_EMIT("varying mediump float Out_FogAmt;\nuniform lowp vec3 FogColor;");
        
    if(flags & (FLAG_LIGHTING | FLAG_COLOR))
        PXL_EMIT("varying lowp vec4 Out_Color;");
        
    if ((flags & FLAG_LIGHT1) || (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
        PXL_EMIT("varying lowp vec3 Out_Spec;");

    if(flags & FLAG_ALPHA_MODULATE)
        PXL_EMIT("uniform lowp float AlphaModulate;");

    if(flags & FLAG_WATER)
        PXL_EMIT("varying mediump vec2 Out_WaterDetail;\nvarying mediump vec2 Out_WaterDetail2;\nvarying mediump float Out_WaterAlphaBlend;");

    ///////////////////////////////////////
    //////////////// START ////////////////
    ///////////////////////////////////////
    PXL_EMIT("void main() {");

    PXL_EMIT("\tlowp vec4 fcolor;");
    if(flags & FLAG_TEX0)
    {
        if(flags & FLAG_TEXBIAS)
        {
            PXL_EMIT("\tlowp vec4 diffuseColor = texture2D(Diffuse, Out_Tex0, -1.5);");
        }
        else if(RQCaps->isSlowGPU != 0)
        {
            PXL_EMIT("\tlowp vec4 diffuseColor = texture2D(Diffuse, Out_Tex0);");
        }
        else
        {
            PXL_EMIT("\tlowp vec4 diffuseColor = texture2D(Diffuse, Out_Tex0, -0.5);");
        }
        PXL_EMIT("\tfcolor = diffuseColor;");

        if(flags & (FLAG_COLOR | FLAG_LIGHTING))
        {
            if(flags & FLAG_DETAILMAP)
            {
                if(!(flags & FLAG_WATER))
                {
                    PXL_EMIT("\tfcolor *= vec4(Out_Color.xyz * texture2D(EnvMap, Out_Tex0.xy * DetailTiling, -0.5).xyz * 2.0, Out_Color.w);");
                    if(flags & FLAG_ENVMAP)
                    {
                        PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
                    }
                    goto LABEL_45; // CHECK
                }
                PXL_EMIT("\tfloat waterDetail = texture2D(EnvMap, Out_WaterDetail, -1.0).x + texture2D(EnvMap, Out_WaterDetail2, -1.0).x;");
                PXL_EMIT("\tfcolor *= vec4(Out_Color.xyz * waterDetail * 1.1, Out_Color.w);");
                PXL_EMIT("\tfcolor.a += Out_WaterAlphaBlend;");
                if(flags & FLAG_ENVMAP)
                {
                    PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
                }
                goto LABEL_45; // CHECK
            }
            PXL_EMIT("\tfcolor *= Out_Color;");
        }
        if(!(flags & FLAG_WATER))
        {
            if(flags & FLAG_ENVMAP)
            {
                PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
            }
            goto LABEL_45; // CHECK
        }
        PXL_EMIT("\tfcolor.a += Out_WaterAlphaBlend;");
        if(flags & FLAG_ENVMAP)
        {
            PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
        }
        goto LABEL_45; // CHECK
    }

    if(flags & (FLAG_COLOR | FLAG_LIGHTING))
    {
        PXL_EMIT("\tfcolor = Out_Color;");
    }
    else
    {
        PXL_EMIT("\tfcolor = 0.0;");
    }
    if(flags & FLAG_ENVMAP)
    {
        PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");
    }

LABEL_45:
    if(flags & FLAG_SPHERE_ENVMAP)
    {
        PXL_EMIT("\tvec2 ReflPos = normalize(Out_Refl.xy) * (Out_Refl.z * 0.5 + 0.5);");
        PXL_EMIT("\tReflPos = (ReflPos * vec2(0.5,0.5)) + vec2(0.5,0.5);");
        PXL_EMIT("\tlowp vec4 ReflTexture = texture2D(EnvMap, ReflPos);");
        PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz,ReflTexture.xyz, EnvMapCoefficient);");
        PXL_EMIT("\tfcolor.w += ReflTexture.b * 0.125;");
    }

    if(!RQCaps->unk_08)
    {
        if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
        {
            PXL_EMIT("\tfcolor.xyz += Out_Spec;");
        }
        if(flags & FLAG_FOG)
            PXL_EMIT("\tfcolor.xyz = mix(fcolor.xyz, FogColor, Out_FogAmt);");
    }

    if(flags & FLAG_GAMMA)
    {
        PXL_EMIT("\tfcolor.xyz += fcolor.xyz * 0.5;");
    }

    PXL_EMIT("\tgl_FragColor = fcolor;");
    if(flags & FLAG_ALPHA_TEST)
    {
        PXL_EMIT("/*ATBEGIN*/");
        if ((OS_SystemChip() == 13) && (flags & FLAG_TEX0))
        {
            if (flags & FLAG_TEXBIAS)
            {
                PXL_EMIT("\tif (diffuseColor.a < 0.8) { discard; }");
            }
            else
            {
                if (!(flags & FLAG_CAMERA_BASED_NORMALS))
                {
                    PXL_EMIT("\tif (diffuseColor.a < 0.2) { discard; }");
                }
                else
                {
                    PXL_EMIT("\tgl_FragColor.a = Out_Color.a;");
                    PXL_EMIT("\tif (diffuseColor.a < 0.5) { discard; }");
                }
            }
        }
        else
        {
            if (flags & FLAG_TEXBIAS)
            {
                PXL_EMIT("\tif (diffuseColor.a < 0.8) { discard; }");
            }
            else
            {
                if (!(flags & FLAG_CAMERA_BASED_NORMALS))
                {
                    PXL_EMIT("\tif (diffuseColor.a < 0.2) { discard; }");
                }
                else
                {
                    PXL_EMIT("\tif (diffuseColor.a < 0.5) { discard; }");
                    PXL_EMIT("\tgl_FragColor.a = Out_Color.a;");
                }
            }
        }
        PXL_EMIT("/*ATEND*/");
    }

    if(flags & FLAG_ALPHA_MODULATE)
        PXL_EMIT("\tgl_FragColor.a *= AlphaModulate;");

    PXL_EMIT("}");
    ///////////////////////////////////////
    ////////////////  END  ////////////////
    ///////////////////////////////////////
}

void BuildVertexSource_Reversed(int flags)
{
    char tmp[512];
    char* t = vtxbuf;
    int ped_spec = pDisablePedSpec->GetBool() ? 0 : (FLAG_BONE3 | FLAG_BONE4);
    
    VTX_EMIT("#version 100");
    VTX_EMIT("precision highp float;\nuniform mat4 ProjMatrix;\nuniform mat4 ViewMatrix;\nuniform mat4 ObjMatrix;");
    if(flags & FLAG_LIGHTING)
    {
        VTX_EMIT("uniform lowp vec3 AmbientLightColor;\nuniform lowp vec4 MaterialEmissive;");
        VTX_EMIT("uniform lowp vec4 MaterialAmbient;\nuniform lowp vec4 MaterialDiffuse;");
        if(flags & FLAG_LIGHT1)
        {
            VTX_EMIT("uniform lowp vec3 DirLightDiffuseColor;\nuniform vec3 DirLightDirection;");
            if (GetMobileEffectSetting() == 3 && (flags & (FLAG_BACKLIGHT | FLAG_BONE3 | FLAG_BONE4)))
            {
                VTX_EMIT("uniform vec3 DirBackLightDirection;");
            }
        }
        if(flags & FLAG_LIGHT2)
        {
            VTX_EMIT("uniform lowp vec3 DirLight2DiffuseColor;\nuniform vec3 DirLight2Direction;");
        }
        if(flags & FLAG_LIGHT3)
        {
            VTX_EMIT("uniform lowp vec3 DirLight3DiffuseColor;\nuniform vec3 DirLight3Direction;");
        }
    }
    
    VTX_EMIT("attribute vec3 Position;\nattribute vec3 Normal;");
    if(flags & FLAG_TEX0)
    {
        if(flags & FLAG_PROJECT_TEXCOORD)
            VTX_EMIT("attribute vec4 TexCoord0;");
        else
            VTX_EMIT("attribute vec2 TexCoord0;");
    }

    VTX_EMIT("attribute vec4 GlobalColor;");
    
    if(flags & (FLAG_BONE3 | FLAG_BONE4))
    {
        VTX_EMIT("attribute vec4 BoneWeight;\nattribute vec4 BoneIndices;");
        VTX_EMIT_V("uniform highp vec4 Bones[%d];", 3 * *RQMaxBones);
    }
    
    if(flags & FLAG_TEX0)
    {
        VTX_EMIT("varying mediump vec2 Out_Tex0;");
    }
    
    if(flags & FLAG_TEXMATRIX)
    {
        VTX_EMIT("uniform mat3 NormalMatrix;");
    }

    if(flags & (FLAG_SPHERE_ENVMAP | FLAG_ENVMAP))
    {
        if(flags & FLAG_ENVMAP)
            VTX_EMIT("varying mediump vec2 Out_Tex1;");
        else
            VTX_EMIT("varying mediump vec3 Out_Refl;");
        VTX_EMIT("uniform lowp float EnvMapCoefficient;");
    }

    if(flags & (FLAG_SPHERE_ENVMAP | FLAG_SPHERE_XFORM | FLAG_WATER | FLAG_FOG | FLAG_CAMERA_BASED_NORMALS | FLAG_ENVMAP | ped_spec))
    {
        VTX_EMIT("uniform vec3 CameraPosition;");
    }

    if(flags & FLAG_FOG)
    {
        VTX_EMIT("varying mediump float Out_FogAmt;");
        VTX_EMIT("uniform vec3 FogDistances;");
    }

    if(flags & FLAG_WATER)
    {
        VTX_EMIT("uniform vec3 WaterSpecs;");
        VTX_EMIT("varying mediump vec2 Out_WaterDetail;");
        VTX_EMIT("varying mediump vec2 Out_WaterDetail2;");
        VTX_EMIT("varying mediump float Out_WaterAlphaBlend;");
    }

    if(flags & FLAG_COLOR2)
    {
        VTX_EMIT("attribute vec4 Color2;");
        VTX_EMIT("uniform lowp float ColorInterp;");
    }

    if(flags & (FLAG_COLOR | FLAG_LIGHTING))
    {
        VTX_EMIT("varying lowp vec4 Out_Color;");
    }

    if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
    {
        VTX_EMIT("varying lowp vec3 Out_Spec;");
    }

    ///////////////////////////////////////
    //////////////// START ////////////////
    ///////////////////////////////////////
    VTX_EMIT("void main() {");

    if(flags & (FLAG_BONE4 | FLAG_BONE3))
    {
        VTX_EMIT("\tivec4 BlendIndexArray = ivec4(BoneIndices);");
        VTX_EMIT("\tmat4 BoneToLocal;");
        VTX_EMIT("\tBoneToLocal[0] = Bones[BlendIndexArray.x*3] * BoneWeight.x;");
        VTX_EMIT("\tBoneToLocal[1] = Bones[BlendIndexArray.x*3+1] * BoneWeight.x;");
        VTX_EMIT("\tBoneToLocal[2] = Bones[BlendIndexArray.x*3+2] * BoneWeight.x;");
        VTX_EMIT("\tBoneToLocal[3] = vec4(0.0,0.0,0.0,1.0);");
        VTX_EMIT("\tBoneToLocal[0] += Bones[BlendIndexArray.y*3] * BoneWeight.y;");
        VTX_EMIT("\tBoneToLocal[1] += Bones[BlendIndexArray.y*3+1] * BoneWeight.y;");
        VTX_EMIT("\tBoneToLocal[2] += Bones[BlendIndexArray.y*3+2] * BoneWeight.y;");
        VTX_EMIT("\tBoneToLocal[0] += Bones[BlendIndexArray.z*3] * BoneWeight.z;");
        VTX_EMIT("\tBoneToLocal[1] += Bones[BlendIndexArray.z*3+1] * BoneWeight.z;");
        VTX_EMIT("\tBoneToLocal[2] += Bones[BlendIndexArray.z*3+2] * BoneWeight.z;");

        if(flags & FLAG_BONE4)
        {
            VTX_EMIT("\tBoneToLocal[0] += Bones[BlendIndexArray.w*3] * BoneWeight.w;");
            VTX_EMIT("\tBoneToLocal[1] += Bones[BlendIndexArray.w*3+1] * BoneWeight.w;");
            VTX_EMIT("\tBoneToLocal[2] += Bones[BlendIndexArray.w*3+2] * BoneWeight.w;");
        }

        VTX_EMIT("\tvec4 WorldPos = ObjMatrix * (vec4(Position,1.0) * BoneToLocal);");
    }
    else
    {
        VTX_EMIT("\tvec4 WorldPos = ObjMatrix * vec4(Position,1.0);");
    }

    if(flags & FLAG_SPHERE_XFORM)
    {
        VTX_EMIT("\tvec3 ReflVector = WorldPos.xyz - CameraPosition.xyz;");
        VTX_EMIT("\tvec3 ReflPos = normalize(ReflVector);");
        VTX_EMIT("\tReflPos.xy = normalize(ReflPos.xy) * (ReflPos.z * 0.5 + 0.5);");
        VTX_EMIT("\tgl_Position = vec4(ReflPos.xy, length(ReflVector) * 0.002, 1.0);");
    }
    else
    {
        VTX_EMIT("\tvec4 ViewPos = ViewMatrix * WorldPos;");
        VTX_EMIT("\tgl_Position = ProjMatrix * ViewPos;");
    }

    if(flags & FLAG_LIGHTING)
    {
        if((flags & (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST)) == (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST) && (flags & (FLAG_LIGHT3 | FLAG_LIGHT2 | FLAG_LIGHT1)))
        {
            VTX_EMIT("vec3 WorldNormal = normalize(vec3(WorldPos.xy - CameraPosition.xy, 0.0001)) * 0.85;");
        }
        else if(flags & (FLAG_BONE4 | FLAG_BONE3))
        {
            VTX_EMIT("vec3 WorldNormal = mat3(ObjMatrix) * (Normal * mat3(BoneToLocal));");
        }
        else
        {
            VTX_EMIT("vec3 WorldNormal = (ObjMatrix * vec4(Normal, 0.0)).xyz;");
        }
    }
    else
    {
        if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
            VTX_EMIT("vec3 WorldNormal = vec3(0.0, 0.0, 0.0);");
    }

    if(!RQCaps->unk_08 && (flags & FLAG_FOG))
    {
        if(pExponentialFog->GetBool()) VTX_EMIT("float fogDist = length(WorldPos.xyz - CameraPosition.xyz);\nOut_FogAmt = clamp(1.0 - exp2(-1.7 * FogDistances.z*FogDistances.z * fogDist*fogDist * 1.442695), 0.0, 1.0);");
        else VTX_EMIT("Out_FogAmt = clamp((length(WorldPos.xyz - CameraPosition.xyz) - FogDistances.x) * FogDistances.z, 0.0, 0.90);");
        //VTX_EMIT("Out_FogAmt = clamp((length(WorldPos.xyz - CameraPosition.xyz) - FogDistances.x) * FogDistances.z, 0.0, 0.90);");
    }

    const char* _tmp;
    if(flags & FLAG_TEX0)
    {
        if(flags & FLAG_PROJECT_TEXCOORD)
        {
            _tmp = "TexCoord0.xy / TexCoord0.w";
        }
        else if(flags & FLAG_COMPRESSED_TEXCOORD)
        {
            _tmp = "TexCoord0 / 512.0";
        }
        else
        {
            _tmp = "TexCoord0";
        }

        if(flags & FLAG_TEXMATRIX)
        {
            VTX_EMIT_V("Out_Tex0 = (NormalMatrix * vec3(%s, 1.0)).xy;", _tmp);
        }
        else
        {
            VTX_EMIT_V("Out_Tex0 = %s;", _tmp);
        }
    }

    if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
    {
        VTX_EMIT("vec3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
        VTX_EMIT("reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
        if(flags & FLAG_SPHERE_ENVMAP)
        {
            VTX_EMIT("Out_Refl = reflVector;");
        }
        else
        {
            VTX_EMIT("Out_Tex1 = vec2(length(reflVector.xy), (reflVector.z * 0.5) + 0.25);");
        }
    }

    if(flags & FLAG_COLOR2)
    {
        VTX_EMIT("lowp vec4 InterpColor = mix(GlobalColor, Color2, ColorInterp);");
        _tmp = "InterpColor";
    }
    else
    {
        _tmp = "GlobalColor";
    }

    if(flags & FLAG_LIGHTING)
    {
        VTX_EMIT("vec3 Out_LightingColor;");
        if(flags & FLAG_COLOR_EMISSIVE)
        {
            if(flags & FLAG_CAMERA_BASED_NORMALS)
            {
                VTX_EMIT("Out_LightingColor = AmbientLightColor * MaterialAmbient.xyz * 1.5;");
            }
            else
            {
                VTX_EMIT_V("Out_LightingColor = AmbientLightColor * MaterialAmbient.xyz + %s.xyz;", _tmp);
            }
        }
        else
        {
            VTX_EMIT("Out_LightingColor = AmbientLightColor * MaterialAmbient.xyz + MaterialEmissive.xyz;");
        }

        if(flags & (FLAG_LIGHT3 | FLAG_LIGHT2 | FLAG_LIGHT1))
        {
            if(flags & FLAG_LIGHT1)
            {
                if(GetMobileEffectSetting() == 3 && (flags & (FLAG_BACKLIGHT | FLAG_BONE4 | FLAG_BONE3)))
                {
                    VTX_EMIT("Out_LightingColor += (max(dot(DirLightDirection, WorldNormal), 0.0) + max(dot(DirBackLightDirection, WorldNormal), 0.0)) * DirLightDiffuseColor;");
                }
                else
                {
                    VTX_EMIT("Out_LightingColor += max(dot(DirLightDirection, WorldNormal), 0.0) * DirLightDiffuseColor;");
                }

                if(!(flags & FLAG_LIGHT2))
                {
                    if(flags & FLAG_LIGHT3)
                        VTX_EMIT("Out_LightingColor += max(dot(DirLight3Direction, WorldNormal), 0.0) * DirLight3DiffuseColor;");
                    goto LABEL_89;
                }
            }
            else if(!(flags & FLAG_LIGHT2))
            {
                if(flags & FLAG_LIGHT3)
                    VTX_EMIT("Out_LightingColor += max(dot(DirLight3Direction, WorldNormal), 0.0) * DirLight3DiffuseColor;");
                goto LABEL_89;
            }
            VTX_EMIT("Out_LightingColor += max(dot(DirLight2Direction, WorldNormal), 0.0) * DirLight2DiffuseColor;");
        }
    }

// Finalize
LABEL_89:
    if(flags & (FLAG_COLOR | FLAG_LIGHTING))
    {
        if(flags & FLAG_LIGHTING)
        {
            if(flags & FLAG_COLOR)
                VTX_EMIT_V("Out_Color = vec4((Out_LightingColor.xyz + %s.xyz * 1.5) * MaterialDiffuse.xyz, (MaterialAmbient.w) * %s.w);", _tmp, _tmp);
            else
                VTX_EMIT_V("Out_Color = vec4(Out_LightingColor * MaterialDiffuse.xyz, MaterialAmbient.w * %s.w);", _tmp);
            VTX_EMIT("Out_Color = clamp(Out_Color, 0.0, 1.0);");
        }
        else
        {
            VTX_EMIT_V("Out_Color = %s;", _tmp);
        }
    }

    if(!RQCaps->unk_08 && (flags & FLAG_LIGHT1))
    {
        if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
        {
            VTX_EMIT_V("float specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * EnvMapCoefficient * 2.0;", RQCaps->isMaliChip ? 9.0f : 10.0f);
        }
        else
        {
            if(!(flags & ped_spec)) goto LABEL_107;
            VTX_EMIT("vec3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
            VTX_EMIT("reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
            VTX_EMIT_V("float specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * 0.125;", RQCaps->isMaliChip ? 5.0f : 4.0f);
        }
        VTX_EMIT("Out_Spec = specAmt * DirLightDiffuseColor;");
    }

LABEL_107:
    if(flags & FLAG_WATER)
    {
        VTX_EMIT("Out_WaterDetail = (Out_Tex0 * 4.0) + vec2(WaterSpecs.x * -0.3, WaterSpecs.x * 0.21);");
        VTX_EMIT("Out_WaterDetail2 = (Out_Tex0 * -8.0) + vec2(WaterSpecs.x * 0.12, WaterSpecs.x * -0.05);");
        VTX_EMIT("Out_WaterAlphaBlend = distance(WorldPos.xy, CameraPosition.xy) * WaterSpecs.y;");
    }

    VTX_EMIT("}");
    ///////////////////////////////////////
    ////////////////  END  ////////////////
    ///////////////////////////////////////
}

DECL_HOOK(int, RQShaderBuildSource, int flags, char **pxlsrc, char **vtxsrc)
{
    pxlbuf[0] = '\0'; vtxbuf[0] = '\0';
    if(pPS2Shading->GetBool())
    {
        BuildPixelSource_SkyGfx(flags);
        BuildVertexSource_SkyGfx(flags);
    }
    else
    {
        BuildPixelSource_Reversed(flags);
        BuildVertexSource_Reversed(flags);
    }
    *pxlsrc = pxlbuf; *vtxsrc = vtxbuf;
    return 1;
}

void PatchShaders()
{
    SET_TO(GetMobileEffectSetting,          aml->GetSym(pGTASA, "_Z22GetMobileEffectSettingv"));
    SET_TO(RQCaps,                          aml->GetSym(pGTASA, "RQCaps"));
    SET_TO(RQMaxBones,                      aml->GetSym(pGTASA, "RQMaxBones"));
    SET_TO(deviceChip,                      aml->GetSym(pGTASA, "deviceChip"));
    HOOK(RQShaderBuildSource,               aml->GetSym(pGTASA, "_ZN8RQShader11BuildSourceEjPPKcS2_"));
}