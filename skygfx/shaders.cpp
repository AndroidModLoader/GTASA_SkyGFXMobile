#include <externs.h>

/* Variables */
EmuShader FakeShaderContainer;
ES2Shader* pForcedShader = NULL;
BasicEvent<SimpleVoidFn> shadercreation;

/* Functions */
ES2Shader* CreateCustomShaderAlloc(uint32_t flags, const char* pxlsrc, const char* vtxsrc, size_t pxllen, size_t vtxlen)
{
    if(!pxllen) pxllen = strlen(pxlsrc);
    if(!vtxlen) vtxlen = strlen(vtxsrc);

    // Original game code is freeing them
    // because originally it is created procedurely
    char* pxl = new char[pxllen + 1]; sprintf(pxl, "%s", pxlsrc);
    char* vtx = new char[vtxlen + 1]; sprintf(vtx, "%s", vtxsrc);

    return RQCreateShader(pxl, vtx, flags);
}

/* Hooks */
DECL_HOOKv(InitialiseGame)
{
    InitialiseGame();
    shadercreation.Call();
}
DECL_HOOKv(AssignEmuShader, bool hasNormals)
{
    if(!pForcedShader) AssignEmuShader(hasNormals);
    else
    {
        FakeShaderContainer.shader = pForcedShader;
        FakeShaderContainer.programFlags = pForcedShader->flags;
        SelectEmuShader(&FakeShaderContainer, true); // TRUE if needs to update uniforms
    }
}

/* Main */
void StartShaders()
{
    // Fog wall fix
    // 110 max including terminator
    const char* sFogPart = "Out_FogAmt=clamp((length(WorldPos.xyz-CameraPosition.xyz)-0.9*FogDistances.x)*FogDistances.z*1.4,0.0,1.0);";
    aml->Write(pGTASA + BYBIT(0x5EB972, 0x71202E), sFogPart, strlen(sFogPart)+1);

    // Specular light on vehicles
    // 99 max including terminator
    const char* sSpecPart = "float specAmt=max(pow(dot(reflVector,DirLightDirection),64.0),0.0)*EnvMapCoefficient*1.4;";
    aml->Write(pGTASA + BYBIT(0x5EBF0F, 0x7125CD), sSpecPart, strlen(sSpecPart)+1);

    // Water Sun Reflections are now affected by the fog (TODO: breaking it)
  #ifdef AML32
    //aml->Write8(pGTASA + 0x5A36B2, 0x01);
  #else
    //aml->Write32(pGTASA + 0x6C6F00, 0xD2800021);
    //aml->Write32(pGTASA + 0x6C6F18, 0x52800161); // src
    //aml->Write32(pGTASA + 0x6C6F24, 0x52800161); // dst
  #endif

    // Fixing a sh*tty greenish reflections for Mali
    // 28 max including terminator
    const char* sOutSpecPart1 = "varying lowp vec3 I;";
    aml->Write(pGTASA + BYBIT(0x5EAA4C, 0x711098), sOutSpecPart1, strlen(sOutSpecPart1)+1);
    // 43 max including terminator
    const char* sOutSpecPart2 = "I = specAmt * DirLightDiffuseColor;";
    aml->Write(pGTASA + BYBIT(0x5EBF72, 0x712630), sOutSpecPart2, strlen(sOutSpecPart2)+1);
    // 24 max including terminator
    const char* sOutSpecPart3 = "fcolor.xyz+=min(I,0.8);";
    aml->Write(pGTASA + BYBIT(0x5EAEBC, 0x711508), sOutSpecPart3, strlen(sOutSpecPart3)+1);

    // Additional code for vertex shader
    const char* sMainVertex = ""
        "\n\nvoid main() {"; // max is 511
  #ifdef AML32
    aml->WriteAddr(pGTASA + 0x1CF8A0, (uintptr_t)sMainVertex - pGTASA - 0x1CEF56);
  #else
    aml->Write32(pGTASA + 0x264314, 0xF9451842);
    aml->WriteAddr(pGTASA + 0x711A30, (uintptr_t)sMainVertex);
  #endif

    // Additional code for pixel shader
    const char* sMainPixel = ""
        "\n\nvoid main()"; // max is 511
  #ifdef AML32
    aml->WriteAddr(pGTASA + 0x1CE8FC, (uintptr_t)sMainPixel - pGTASA - 0x1CE3B0);
  #else
    aml->Write32(pGTASA + 0x2638DC, 0x91054042);
    aml->WriteAddr(pGTASA + 0x711150, (uintptr_t)sMainPixel);
  #endif

    // Just a few important hooks
    HOOKPLT(InitialiseGame, pGTASA + BYBIT(0x6740A4, 0x846D20));
    HOOKPLT(AssignEmuShader, pGTASA + BYBIT(0x674170, 0x846E68));
}

/* Backup */
#if 0
///////////////////////////////////////////////////
///////////////////   REVERSED   //////////////////
///////////////////////////////////////////////////
void BuildPixelSource_Reversed(int flags)
{
    char tmp[512];
    char* t = pxlbuf;
    int ped_spec = (FLAG_BONE3 | FLAG_BONE4);
    
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
        
    if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
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
    int ped_spec = (FLAG_BONE3 | FLAG_BONE4);
    
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
        VTX_EMIT("Out_FogAmt = clamp((length(WorldPos.xyz - CameraPosition.xyz) - FogDistances.x) * FogDistances.z, 0.0, 1.00);");
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
#endif
