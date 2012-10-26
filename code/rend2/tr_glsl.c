/*
===========================================================================
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_glsl.c
#include "tr_local.h"

void GLSL_BindNullProgram(void);

// FIXME: Do something that isn't this messy
static const char *fallbackGenericShader_vp =
"attribute vec4 attr_Position;\r\nattribute vec4 attr_TexCoord0;\r\nattribut"
"e vec4 attr_TexCoord1;\r\nattribute vec3 attr_Normal;\r\nattribute vec4 att"
"r_Color;\r\n\r\n#if defined(USE_VERTEX_ANIMATION)\r\nattribute vec4 attr_Po"
"sition2;\r\nattribute vec3 attr_Normal2;\r\n#endif\r\n\r\nuniform mat4   u_"
"DiffuseTexMatrix;\r\nuniform vec3   u_ViewOrigin;\r\n\r\n#if defined(USE_TC"
"GEN)\r\nuniform int    u_TCGen0;\r\nuniform vec3   u_TCGen0Vector0;\r\nunif"
"orm vec3   u_TCGen0Vector1;\r\n#endif\r\n\r\n#if defined(USE_FOG)\r\nunifor"
"m vec4   u_FogDistance;\r\nuniform vec4   u_FogDepth;\r\nuniform float  u_F"
"ogEyeT;\r\nuniform vec4   u_FogColorMask;\r\n#endif\r\n\r\n#if defined(USE_"
"DEFORM_VERTEXES)\r\nuniform int    u_DeformGen;\r\nuniform float  u_DeformP"
"arams[5];\r\n#endif\r\n\r\nuniform float  u_Time;\r\n\r\nuniform mat4   u_M"
"odelViewProjectionMatrix;\r\nuniform vec4   u_BaseColor;\r\nuniform vec4   "
"u_VertColor;\r\n\r\n#if defined(USE_RGBAGEN)\r\nuniform int    u_ColorGen;"
"\r\nuniform int    u_AlphaGen;\r\nuniform vec3   u_AmbientLight;\r\nuniform"
" vec3   u_DirectedLight;\r\nuniform vec4   u_LightOrigin;\r\nuniform float "
" u_PortalRange;\r\n#endif\r\n\r\n#if defined(USE_VERTEX_ANIMATION)\r\nunifo"
"rm float  u_VertexLerp;\r\n#endif\r\n\r\nvarying vec2   var_DiffuseTex;\r\n"
"#if defined(USE_LIGHTMAP)\r\nvarying vec2   var_LightTex;\r\n#endif\r\nvary"
"ing vec4   var_Color;\r\n\r\nvec2 DoTexMatrix(vec2 st, vec3 position, mat4 "
"texMatrix)\r\n{\r\n\tfloat amplitude = texMatrix[3][0];\r\n\tfloat phase = "
"texMatrix[3][1];\r\n\tvec2 st2 = (texMatrix * vec4(st, 1.0, 0.0)).st;\r\n\r"
"\n\tvec3 offsetPos = position.xyz / 1024.0;\r\n\toffsetPos.x += offsetPos.z"
";\r\n\r\n\tvec2 texOffset = sin((offsetPos.xy + vec2(phase)) * 2.0 * M_PI);"
"\r\n\t\r\n\treturn st2 + texOffset * amplitude;\r\n}\r\n\r\n#if defined(USE"
"_DEFORM_VERTEXES)\r\nfloat triangle(float x)\r\n{\r\n\treturn max(1.0 - abs"
"(x), 0);\r\n}\r\n\r\nfloat sawtooth(float x)\r\n{\r\n\treturn x - floor(x);"
"\r\n}\r\n\r\nvec4 DeformPosition(const vec4 pos, const vec3 normal, const v"
"ec2 st)\r\n{\r\n\tfloat base =      u_DeformParams[0];\r\n\tfloat amplitude"
" = u_DeformParams[1];\r\n\tfloat phase =     u_DeformParams[2];\r\n\tfloat "
"frequency = u_DeformParams[3];\r\n\tfloat spread =    u_DeformParams[4];\r"
"\n\t\r\n\tif (u_DeformGen == DGEN_BULGE)\r\n\t{\r\n\t\tphase *= M_PI * 0.25"
" * st.x;\r\n\t}\r\n\telse // if (u_DeformGen <= DGEN_WAVE_INVERSE_SAWTOOTH)"
"\r\n\t{\r\n\t\tphase += (pos.x + pos.y + pos.z) * spread;\r\n\t}\r\n\r\n\tf"
"loat value = phase + (u_Time * frequency);\r\n\tfloat func;\r\n\r\n\tif (u_"
"DeformGen == DGEN_WAVE_SIN)\r\n\t{\r\n\t\tfunc = sin(value * 2.0 * M_PI);\r"
"\n\t}\r\n\telse if (u_DeformGen == DGEN_WAVE_SQUARE)\r\n\t{\r\n\t\tfunc = s"
"ign(sin(value * 2.0 * M_PI));\r\n\t}\r\n\telse if (u_DeformGen == DGEN_WAVE"
"_TRIANGLE)\r\n\t{\r\n\t\tfunc = triangle(value);\r\n\t}\r\n\telse if (u_Def"
"ormGen == DGEN_WAVE_SAWTOOTH)\r\n\t{\r\n\t\tfunc = sawtooth(value);\r\n\t}"
"\r\n\telse if (u_DeformGen == DGEN_WAVE_INVERSE_SAWTOOTH)\r\n\t{\r\n\t\tfun"
"c = (1.0 - sawtooth(value));\r\n\t}\r\n\telse if (u_DeformGen == DGEN_BULGE"
")\r\n\t{\r\n\t\tfunc = sin(value);\r\n\t}\r\n\t\r\n\tvec4 deformed = pos;\r"
"\n\tdeformed.xyz += normal * (base + func * amplitude);\r\n\r\n\treturn def"
"ormed;\r\n}\r\n#endif\r\n\r\n#if defined(USE_TCGEN)\r\nvec2 GenTexCoords(in"
"t TCGen, vec4 position, vec3 normal, vec3 TCGenVector0, vec3 TCGenVector1)"
"\r\n{\r\n\tvec2 tex = attr_TexCoord0.st;\r\n\r\n\tif (TCGen == TCGEN_LIGHTM"
"AP)\r\n\t{\r\n\t\ttex = attr_TexCoord1.st;\r\n\t}\r\n\telse if (TCGen == TC"
"GEN_ENVIRONMENT_MAPPED)\r\n\t{\r\n\t\tvec3 viewer = normalize(u_ViewOrigin "
"- position.xyz);\r\n\t\tvec3 reflected = normal * 2.0 * dot(normal, viewer)"
" - viewer;\r\n\r\n\t\ttex = reflected.yz * vec2(0.5, -0.5) + 0.5;\r\n\t}\r"
"\n\telse if (TCGen == TCGEN_VECTOR)\r\n\t{\r\n\t\ttex = vec2(dot(position.x"
"yz, TCGenVector0), dot(position.xyz, TCGenVector1));\r\n\t}\r\n\t\r\n\tretu"
"rn tex;\r\n}\r\n#endif\r\n\r\nvoid main()\r\n{\r\n#if defined(USE_VERTEX_AN"
"IMATION)\r\n\tvec4 position = mix(attr_Position, attr_Position2, u_VertexLe"
"rp);\r\n\tvec3 normal = normalize(mix(attr_Normal, attr_Normal2, u_VertexLe"
"rp));\r\n#else\r\n\tvec4 position = attr_Position;\r\n\tvec3 normal = attr_"
"Normal;\r\n#endif\r\n\r\n#if defined(USE_DEFORM_VERTEXES)\r\n\tposition = D"
"eformPosition(position, normal, attr_TexCoord0.st);\r\n#endif\r\n\r\n\tgl_P"
"osition = u_ModelViewProjectionMatrix * position;\r\n\r\n#if defined(USE_TC"
"GEN)\r\n\tvec2 tex = GenTexCoords(u_TCGen0, position, normal, u_TCGen0Vecto"
"r0, u_TCGen0Vector1);\r\n#else\r\n\tvec2 tex = attr_TexCoord0.st;\r\n#endif"
"\r\n\tvar_DiffuseTex = DoTexMatrix(tex, position.xyz, u_DiffuseTexMatrix);"
"\r\n\r\n#if defined(USE_LIGHTMAP)\r\n\tvar_LightTex = attr_TexCoord1.st;\r"
"\n#endif\r\n\r\n\tvar_Color = u_VertColor * attr_Color + u_BaseColor;\r\n\r"
"\n#if defined(USE_RGBAGEN)\r\n\tif (u_ColorGen == CGEN_LIGHTING_DIFFUSE)\r"
"\n\t{\r\n\t\tfloat incoming = max(dot(normal, u_LightOrigin.xyz), 0.0);\r\n"
"\r\n\t\tvar_Color.rgb = min(u_DirectedLight * incoming + u_AmbientLight, 1."
"0);\r\n\t}\r\n\t\r\n\tvec3 toView = u_ViewOrigin - position.xyz;\r\n\r\n\ti"
"f (u_AlphaGen == AGEN_LIGHTING_SPECULAR)\r\n\t{\r\n\t\tvec3 lightDir = norm"
"alize(vec3(-960.0, -1980.0, 96.0) - position.xyz);\r\n\t\tvec3 viewer = nor"
"malize(toView);\r\n\t\tvec3 halfangle = normalize(lightDir + viewer);\r\n\t"
"\t\r\n\t\tvar_Color.a = pow(max(dot(normal, halfangle), 0.0), 8.0);\r\n\t}"
"\r\n\telse if (u_AlphaGen == AGEN_PORTAL)\r\n\t{\r\n\t\tfloat alpha = lengt"
"h(toView) / u_PortalRange;\r\n\r\n\t\tvar_Color.a = min(alpha, 1.0);\r\n\t}"
"\r\n\telse if (u_AlphaGen == AGEN_FRESNEL)\r\n\t{\r\n\t\tvec3 viewer = norm"
"alize(toView);\r\n\t\t\r\n\t\tvar_Color.a = 0.10 + 0.90 * pow(1.0 - dot(nor"
"mal, viewer), 5);\r\n\t}\r\n#endif\r\n\r\n#if defined (USE_FOG)\r\n\tfloat "
"s = dot(position, u_FogDistance);\r\n\tfloat t = dot(position, u_FogDepth);"
"\r\n\t\r\n\tif (t >= 1.0)\r\n\t{\r\n\t\ts *= t / (t - min(u_FogEyeT, 0.0));"
"\r\n\t}\r\n\telse\r\n\t{\r\n\t\ts *= max(t + sign(u_FogEyeT), 0.0);\r\n\t}"
"\r\n\t\r\n\ts = 1.0 - sqrt(clamp(s * 8.0, 0.0, 1.0));\r\n\t\r\n\tvar_Color "
"*= u_FogColorMask * s + (vec4(1.0) - u_FogColorMask);\r\n#endif\r\n}\r\n";

static const char *fallbackGenericShader_fp =
"uniform sampler2D u_DiffuseMap;\r\n\r\n#if defined(USE_LIGHTMAP)\r\nuniform"
" sampler2D u_LightMap;\r\n#endif\r\n\r\nuniform int       u_Texture1Env;\r"
"\n\r\nvarying vec2      var_DiffuseTex;\r\n\r\n#if defined(USE_LIGHTMAP)\r"
"\nvarying vec2      var_LightTex;\r\n#endif\r\n\r\nvarying vec4      var_Co"
"lor;\r\n\r\n\r\nvoid main()\r\n{\r\n\tvec4 color  = texture2D(u_DiffuseMap,"
" var_DiffuseTex);\r\n#if defined(USE_LIGHTMAP)\r\n\tvec4 color2 = texture2D"
"(u_LightMap, var_LightTex);\r\n  #if defined(RGBE_LIGHTMAP)\r\n\tcolor2.rgb"
" *= exp2(color2.a * 255.0 - 128.0);\r\n\tcolor2.a = 1.0;\r\n  #endif\r\n\r"
"\n\tif (u_Texture1Env == TEXENV_MODULATE)\r\n\t{\r\n\t\tcolor *= color2;\r"
"\n\t}\r\n\telse if (u_Texture1Env == TEXENV_ADD)\r\n\t{\r\n\t\tcolor += col"
"or2;\r\n\t}\r\n\telse if (u_Texture1Env == TEXENV_REPLACE)\r\n\t{\r\n\t\tco"
"lor = color2;\r\n\t}\r\n#endif\r\n\r\n\tgl_FragColor = color * var_Color;\r"
"\n}\r\n";

static const char *fallbackTextureColorShader_vp =
"#version 120\r\n\r\nattribute vec4 attr_Position;\r\nattribute vec4 attr_Te"
"xCoord0;\r\n\r\nuniform mat4   u_ModelViewProjectionMatrix;\r\n\r\nvarying "
"vec2   var_Tex1;\r\n\r\n\r\nvoid main()\r\n{\r\n\tgl_Position = u_ModelView"
"ProjectionMatrix * attr_Position;\r\n\tvar_Tex1 = attr_TexCoord0.st;\r\n}\r"
"\n";

static const char *fallbackTextureColorShader_fp =
"#version 120\r\n\r\nuniform sampler2D u_DiffuseMap;\r\nuniform vec4      u_"
"Color;\r\n\r\nvarying vec2         var_Tex1;\r\n\r\n\r\nvoid main()\r\n{\r"
"\n\tgl_FragColor = texture2D(u_DiffuseMap, var_Tex1) * u_Color;\r\n}\r\n";

static const char *fallbackFogPassShader_vp =
"attribute vec4  attr_Position;\r\nattribute vec3  attr_Normal;\r\nattribute"
" vec4  attr_TexCoord0;\r\n\r\n//#if defined(USE_VERTEX_ANIMATION)\r\nattrib"
"ute vec4  attr_Position2;\r\nattribute vec3  attr_Normal2;\r\n//#endif\r\n"
"\r\nuniform vec4    u_FogDistance;\r\nuniform vec4    u_FogDepth;\r\nunifor"
"m float   u_FogEyeT;\r\n\r\n//#if defined(USE_DEFORM_VERTEXES)\r\nuniform i"
"nt     u_DeformGen;\r\nuniform float   u_DeformParams[5];\r\n//#endif\r\n\r"
"\nuniform float   u_Time;\r\nuniform mat4    u_ModelViewProjectionMatrix;\r"
"\n\r\n//#if defined(USE_VERTEX_ANIMATION)\r\nuniform float   u_VertexLerp;"
"\r\n//#endif\r\n\r\nvarying float   var_Scale;\r\n\r\n\r\nfloat triangle(fl"
"oat x)\r\n{\r\n\treturn max(1.0 - abs(x), 0);\r\n}\r\n\r\nfloat sawtooth(fl"
"oat x)\r\n{\r\n\treturn x - floor(x);\r\n}\r\n\r\nvec4 DeformPosition(const"
" vec4 pos, const vec3 normal, const vec2 st)\r\n{\r\n\tif (u_DeformGen == 0"
")\r\n\t{\r\n\t\treturn pos;\r\n\t}\r\n\r\n\tfloat base =      u_DeformParam"
"s[0];\r\n\tfloat amplitude = u_DeformParams[1];\r\n\tfloat phase =     u_De"
"formParams[2];\r\n\tfloat frequency = u_DeformParams[3];\r\n\tfloat spread "
"=    u_DeformParams[4];\r\n\t\t\r\n\tif (u_DeformGen <= DGEN_WAVE_INVERSE_S"
"AWTOOTH)\r\n\t{\r\n\t\tphase += (pos.x + pos.y + pos.z) * spread;\r\n\t}\r"
"\n\telse if (u_DeformGen == DGEN_BULGE)\r\n\t{\r\n\t\tphase *= M_PI * 0.25 "
"* st.x;\r\n\t}\r\n\r\n\tfloat value = phase + (u_Time * frequency);\r\n\tfl"
"oat func;\r\n\r\n\tif (u_DeformGen == DGEN_WAVE_SIN)\r\n\t{\r\n\t\tfunc = s"
"in(value * 2.0 * M_PI);\r\n\t}\r\n\telse if (u_DeformGen == DGEN_WAVE_SQUAR"
"E)\r\n\t{\r\n\t\tfunc = sign(sin(value * 2.0 * M_PI));\r\n\t}\r\n\telse if "
"(u_DeformGen == DGEN_WAVE_TRIANGLE)\r\n\t{\r\n\t\tfunc = triangle(value);\r"
"\n\t}\r\n\telse if (u_DeformGen == DGEN_WAVE_SAWTOOTH)\r\n\t{\r\n\t\tfunc ="
" sawtooth(value);\r\n\t}\r\n\telse if (u_DeformGen == DGEN_WAVE_INVERSE_SAW"
"TOOTH)\r\n\t{\r\n\t\tfunc = (1.0 - sawtooth(value));\r\n\t}\r\n\telse if (u"
"_DeformGen == DGEN_BULGE)\r\n\t{\r\n\t\tfunc = sin(value);\r\n\t}\r\n\r\n\t"
"vec4 deformed = pos;\r\n\tdeformed.xyz += normal * (base + func * amplitude"
");\r\n\r\n\treturn deformed;\r\n\r\n}\r\n\r\nvoid main()\r\n{\r\n\tvec4 pos"
"ition = mix(attr_Position, attr_Position2, u_VertexLerp);\r\n\tvec3 normal "
"= normalize(mix(attr_Normal, attr_Normal2, u_VertexLerp));\r\n\r\n\tpositio"
"n = DeformPosition(position, normal, attr_TexCoord0.st);\r\n\r\n\tgl_Positi"
"on = u_ModelViewProjectionMatrix * position;\r\n\r\n\tfloat s = dot(positio"
"n, u_FogDistance);\r\n\tfloat t = dot(position, u_FogDepth);\r\n\r\n\tif (t"
" >= 1.0)\r\n\t{\r\n\t\ts *= t / (t - min(u_FogEyeT, 0.0));\r\n\t}\r\n\telse"
"\r\n\t{\r\n\t\ts *= max(t + sign(u_FogEyeT), 0.0);\r\n\t}\r\n\r\n\tvar_Scal"
"e = s * 8.0;\r\n}\r\n";

static const char *fallbackFogPassShader_fp =
"uniform vec4  u_Color;\r\n\r\nvarying float var_Scale;\r\n\r\nvoid main()\r"
"\n{\r\n\tgl_FragColor = u_Color;\r\n\tgl_FragColor.a *= sqrt(clamp(var_Scal"
"e, 0.0, 1.0));\r\n}\r\n";

static const char *fallbackDlightShader_vp =
"attribute vec4 attr_Position;\r\nattribute vec4 attr_TexCoord0;\r\nattribut"
"e vec3 attr_Normal;\r\n\r\nuniform vec4   u_DlightInfo;\r\n\r\nuniform int "
"   u_DeformGen;\r\nuniform float  u_DeformParams[5];\r\n\r\nuniform float  "
"u_Time;\r\nuniform vec4   u_Color;\r\nuniform mat4   u_ModelViewProjectionM"
"atrix;\r\n\r\nvarying vec2   var_Tex1;\r\nvarying vec4   var_Color;\r\n\r\n"
"float triangle(float x)\r\n{\r\n\treturn max(1.0 - abs(x), 0);\r\n}\r\n\r\n"
"float sawtooth(float x)\r\n{\r\n\treturn x - floor(x);\r\n}\r\n\r\nvec4 Def"
"ormPosition(const vec4 pos, const vec3 normal, const vec2 st)\r\n{\r\n\tif "
"(u_DeformGen == 0)\r\n\t{\r\n\t\treturn pos;\r\n\t}\r\n\r\n\tfloat base =  "
"    u_DeformParams[0];\r\n\tfloat amplitude = u_DeformParams[1];\r\n\tfloat"
" phase =     u_DeformParams[2];\r\n\tfloat frequency = u_DeformParams[3];\r"
"\n\tfloat spread =    u_DeformParams[4];\r\n\t\t\r\n\tif (u_DeformGen <= DG"
"EN_WAVE_INVERSE_SAWTOOTH)\r\n\t{\r\n\t\tphase += (pos.x + pos.y + pos.z) * "
"spread;\r\n\t}\r\n\telse if (u_DeformGen == DGEN_BULGE)\r\n\t{\r\n\t\tphase"
" *= M_PI * 0.25 * st.x;\r\n\t}\r\n\r\n\tfloat value = phase + (u_Time * fre"
"quency);\r\n\tfloat func;\r\n\r\n\tif (u_DeformGen == DGEN_WAVE_SIN)\r\n\t{"
"\r\n\t\tfunc = sin(value * 2.0 * M_PI);\r\n\t}\r\n\telse if (u_DeformGen =="
" DGEN_WAVE_SQUARE)\r\n\t{\r\n\t\tfunc = sign(sin(value * 2.0 * M_PI));\r\n"
"\t}\r\n\telse if (u_DeformGen == DGEN_WAVE_TRIANGLE)\r\n\t{\r\n\t\tfunc = t"
"riangle(value);\r\n\t}\r\n\telse if (u_DeformGen == DGEN_WAVE_SAWTOOTH)\r\n"
"\t{\r\n\t\tfunc = sawtooth(value);\r\n\t}\r\n\telse if (u_DeformGen == DGEN"
"_WAVE_INVERSE_SAWTOOTH)\r\n\t{\r\n\t\tfunc = (1.0 - sawtooth(value));\r\n\t"
"}\r\n\telse if (u_DeformGen == DGEN_BULGE)\r\n\t{\r\n\t\tfunc = sin(value);"
"\r\n\t}\r\n\r\n\tvec4 deformed = pos;\r\n\tdeformed.xyz += normal * (base +"
" func * amplitude);\r\n\r\n\treturn deformed;\r\n\r\n}\r\n\r\nvoid main()\r"
"\n{\r\n\tvec4 position = attr_Position;\r\n\tvec3 normal = attr_Normal;\r\n"
"\r\n\tposition = DeformPosition(position, normal, attr_TexCoord0.st);\r\n\r"
"\n\tgl_Position = u_ModelViewProjectionMatrix * position;\r\n\t\t\r\n\tvec3"
" dist = u_DlightInfo.xyz - position.xyz;\t\r\n\r\n\tfloat diffz = abs(dist."
"z);\r\n\tfloat radius = 1.0 / u_DlightInfo.a;\r\n\r\n\tvec2 tex = vec2(0.5)"
" + dist.xy * u_DlightInfo.a;\r\n\tfloat dlightmod = max(sign(dot(dist, norm"
"al)), 0.0);\r\n\tdlightmod *= clamp(2.0 * (radius - diffz) * u_DlightInfo.a"
", 0.0, 1.0);\r\n\r\n\tvar_Tex1 = tex;\r\n\tvar_Color = u_Color;\r\n\tvar_Co"
"lor.rgb *= dlightmod;\r\n}\r\n";

static const char *fallbackDlightShader_fp =
"uniform sampler2D u_DiffuseMap;\r\n\r\nvarying vec2      var_Tex1;\r\nvaryi"
"ng vec4      var_Color;\r\n\r\n\r\nvoid main()\r\n{\r\n\tvec4 color = textu"
"re2D(u_DiffuseMap, var_Tex1);\r\n\r\n\tgl_FragColor = color * var_Color;\r"
"\n}\r\n";

static const char *fallbackLightallShader_vp =
"attribute vec4 attr_TexCoord0;\r\n#if defined(USE_LIGHTMAP)\r\nattribute ve"
"c4 attr_TexCoord1;\r\n#endif\r\nattribute vec4 attr_Color;\r\n\r\nattribute"
" vec4 attr_Position;\r\nattribute vec3 attr_Normal;\r\n\r\n#if defined(USE_"
"VERT_TANGENT_SPACE)\r\nattribute vec3 attr_Tangent;\r\nattribute vec3 attr_"
"Bitangent;\r\n#endif\r\n\r\n#if defined(USE_VERTEX_ANIMATION)\r\nattribute "
"vec4 attr_Position2;\r\nattribute vec3 attr_Normal2;\r\n  #if defined(USE_V"
"ERT_TANGENT_SPACE)\r\nattribute vec3 attr_Tangent2;\r\nattribute vec3 attr_"
"Bitangent2;\r\n  #endif\r\n#endif\r\n\r\n#if defined(USE_LIGHT) && !defined"
"(USE_LIGHT_VECTOR)\r\nattribute vec3 attr_LightDirection;\r\n#endif\r\n\r\n"
"#if defined(TCGEN_ENVIRONMENT) || defined(USE_NORMALMAP) || defined(USE_LIG"
"HT) && !defined(USE_FAST_LIGHT)\r\nuniform vec3   u_ViewOrigin;\r\n#endif\r"
"\n\r\nuniform mat4   u_DiffuseTexMatrix;\r\nuniform mat4   u_ModelViewProje"
"ctionMatrix;\r\nuniform vec4   u_BaseColor;\r\nuniform vec4   u_VertColor;"
"\r\n\r\n#if defined(USE_MODELMATRIX)\r\nuniform mat4   u_ModelMatrix;\r\n#e"
"ndif\r\n\r\n#if defined(USE_VERTEX_ANIMATION)\r\nuniform float  u_VertexLer"
"p;\r\n#endif\r\n\r\n#if defined(USE_LIGHT_VECTOR)\r\nuniform vec4   u_Light"
"Origin;\r\n  #if defined(USE_FAST_LIGHT)\r\nuniform vec3   u_DirectedLight;"
"\r\nuniform vec3   u_AmbientLight;\r\nuniform float  u_LightRadius;\r\n  #e"
"ndif\r\n#endif\r\n\r\nvarying vec2   var_DiffuseTex;\r\n\r\n#if defined(USE"
"_LIGHTMAP)\r\nvarying vec2   var_LightTex;\r\n#endif\r\n\r\n#if defined(USE"
"_NORMALMAP) || defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)\r\nvarying ve"
"c3   var_SampleToView;\r\n#endif\r\n\r\nvarying vec4   var_Color;\r\nvaryin"
"g vec3   var_Position;\r\nvarying vec3   var_Normal;\r\n\r\n#if defined(USE"
"_VERT_TANGENT_SPACE)\r\nvarying vec3   var_Tangent;\r\nvarying vec3   var_B"
"itangent;\r\n#endif\r\n\r\nvarying vec3   var_VertLight;\r\n\r\n#if defined"
"(USE_LIGHT) && !defined(USE_DELUXEMAP)\r\nvarying vec3   var_WorldLight;\r"
"\n#endif\r\n\r\nvarying vec4   var_ScreenPos;\r\n\r\nvec2 DoTexMatrix(vec2 "
"st, vec3 position, mat4 texMatrix)\r\n{\r\n\tvec2 st2 = (texMatrix * vec4(s"
"t, 1, 0)).st;\r\n\r\n\tvec3 offsetPos = position.xyz / 1024.0;\r\n\toffsetP"
"os.x += offsetPos.z;\r\n\r\n\tvec2 texOffset = sin((offsetPos.xy + vec2(tex"
"Matrix[3][1])) * 2.0 * M_PI);\r\n\t\r\n\treturn st2 + texOffset * texMatrix"
"[3][0];\r\n}\r\n\r\nvoid main()\r\n{\r\n#if defined(USE_VERTEX_ANIMATION)\r"
"\n\tvec4 position  = mix(attr_Position, attr_Position2, u_VertexLerp);\r\n"
"\tvec3 normal    = normalize(mix(attr_Normal,    attr_Normal2,    u_VertexL"
"erp));\r\n  #if defined(USE_VERT_TANGENT_SPACE)\r\n\tvec3 tangent   = norma"
"lize(mix(attr_Tangent,   attr_Tangent2,   u_VertexLerp));\r\n\tvec3 bitange"
"nt = normalize(mix(attr_Bitangent, attr_Bitangent2, u_VertexLerp));\r\n  #e"
"ndif\r\n#else\r\n\tvec4 position  = attr_Position;\r\n\tvec3 normal    = at"
"tr_Normal;\r\n  #if defined(USE_VERT_TANGENT_SPACE)\r\n\tvec3 tangent   = a"
"ttr_Tangent;\r\n\tvec3 bitangent = attr_Bitangent;\r\n  #endif\r\n#endif\r"
"\n\r\n\tgl_Position = u_ModelViewProjectionMatrix * position;\r\n\tvar_Scre"
"enPos = gl_Position;\r\n\r\n#if (defined(USE_LIGHTMAP) || defined(USE_LIGHT"
"_VERTEX)) && !defined(USE_DELUXEMAP)\r\n\tvec3 worldLight = attr_LightDirec"
"tion;\r\n#endif\r\n\t\r\n#if defined(USE_MODELMATRIX)\r\n\tposition  = u_Mo"
"delMatrix * position;\r\n\tnormal    = (u_ModelMatrix * vec4(normal, 0.0))."
"xyz;\r\n  #if defined(USE_VERT_TANGENT_SPACE)\r\n\ttangent   = (u_ModelMatr"
"ix * vec4(tangent, 0.0)).xyz;\r\n\tbitangent = (u_ModelMatrix * vec4(bitang"
"ent, 0.0)).xyz;\r\n  #endif\r\n\r\n  #if defined(USE_LIGHTMAP) && !defined("
"USE_DELUXEMAP)\r\n\tworldLight = (u_ModelMatrix * vec4(worldLight, 0.0)).xy"
"z;\r\n  #endif\r\n#endif\r\n\r\n\tvar_Position = position.xyz;\r\n\r\n#if d"
"efined(TCGEN_ENVIRONMENT) || defined(USE_NORMALMAP) || defined(USE_LIGHT) &"
"& !defined(USE_FAST_LIGHT)\r\n\tvec3 SampleToView = u_ViewOrigin - position"
".xyz;\r\n#endif\r\n\r\n#if defined(USE_NORMALMAP) || defined(USE_LIGHT) && "
"!defined(USE_FAST_LIGHT)\r\n\tvar_SampleToView = SampleToView;\r\n#endif\r"
"\n\r\n#if defined(TCGEN_ENVIRONMENT)\r\n\tvec3 viewer = normalize(SampleToV"
"iew);\r\n\tvec3 reflected = normal * 2.0 * dot(normal, viewer) - viewer;\r"
"\n\r\n\tvec2 tex = reflected.yz * vec2(0.5, -0.5) + 0.5;\r\n#else\r\n\tvec2"
" tex = attr_TexCoord0.st;\r\n#endif\r\n\r\n\tvar_DiffuseTex = DoTexMatrix(t"
"ex, position.xyz, u_DiffuseTexMatrix);\r\n\r\n#if defined(USE_LIGHTMAP)\r\n"
"\tvar_LightTex = attr_TexCoord1.st;\r\n#endif\r\n  \r\n\tvar_Normal = norma"
"l;\r\n#if defined(USE_VERT_TANGENT_SPACE)\r\n\tvar_Tangent = tangent;\r\n\t"
"var_Bitangent = bitangent;\r\n#endif\r\n\r\n#if defined(USE_LIGHT) && !defi"
"ned(USE_DELUXEMAP)\r\n  #if defined(USE_LIGHT_VECTOR)\r\n\tvec3 worldLight "
"= u_LightOrigin.xyz - (position.xyz * u_LightOrigin.w);\r\n  #endif\r\n\r\n"
"\tworldLight += normal * 0.0001;\r\n\tvar_WorldLight = worldLight;\r\n#endi"
"f\r\n\t\r\n#if defined(USE_LIGHT_VERTEX)\r\n    var_VertLight = attr_Color."
"rgb;\r\n  #if !defined(USE_FAST_LIGHT)\r\n\tvar_VertLight /= max(dot(normal"
", normalize(worldLight)), 0.004);\r\n  #endif\r\n\tvar_Color.rgb = u_BaseCo"
"lor.rgb;\r\n\tvar_Color.a = u_VertColor.a * attr_Color.a + u_BaseColor.a;\r"
"\n#else\r\n\tvar_Color = u_VertColor * attr_Color + u_BaseColor;\r\n#endif"
"\r\n\r\n#if defined(USE_LIGHT_VECTOR) && defined(USE_FAST_LIGHT)\r\n  #if d"
"efined(USE_INVSQRLIGHT)\r\n\tfloat intensity = 1.0 / dot(worldLight, worldL"
"ight);\r\n  #else\r\n\tfloat intensity = clamp((1.0 - dot(worldLight, world"
"Light) / (u_LightRadius * u_LightRadius)) * 1.07, 0.0, 1.0);\r\n  #endif\r"
"\n\tfloat NL = clamp(dot(normal, normalize(worldLight)), 0.0, 1.0);\r\n\r\n"
"\tvar_VertLight = u_DirectedLight * intensity * NL + u_AmbientLight;\r\n#en"
"dif\r\n}\r\n";

static const char *fallbackLightallShader_fp =
"uniform sampler2D u_DiffuseMap;\r\n\r\n#if defined(USE_LIGHTMAP)\r\nuniform"
" sampler2D u_LightMap;\r\n#endif\r\n\r\n#if defined(USE_NORMALMAP)\r\nunifo"
"rm sampler2D u_NormalMap;\r\n#endif\r\n\r\n#if defined(USE_DELUXEMAP)\r\nun"
"iform sampler2D u_DeluxeMap;\r\n#endif\r\n\r\n#if defined(USE_SPECULARMAP)"
"\r\nuniform sampler2D u_SpecularMap;\r\n#endif\r\n\r\n#if defined(USE_SHADO"
"WMAP)\r\nuniform sampler2D u_ShadowMap;\r\n#endif\r\n\r\nuniform vec3      "
"u_ViewOrigin;\r\n\r\n#if defined(USE_LIGHT_VECTOR)\r\nuniform vec3      u_D"
"irectedLight;\r\nuniform vec3      u_AmbientLight;\r\nuniform float     u_L"
"ightRadius;\r\n#endif\r\n\r\n#if defined(USE_LIGHT)\r\nuniform vec2      u_"
"MaterialInfo;\r\n#endif\r\n\r\nvarying vec2      var_DiffuseTex;\r\n#if def"
"ined(USE_LIGHTMAP)\r\nvarying vec2      var_LightTex;\r\n#endif\r\nvarying "
"vec4      var_Color;\r\nvarying vec3      var_Position;\r\n\r\nvarying vec3"
"      var_SampleToView;\r\n\r\nvarying vec3      var_Normal;\r\n#if defined"
"(USE_VERT_TANGENT_SPACE)\r\nvarying vec3      var_Tangent;\r\nvarying vec3 "
"     var_Bitangent;\r\n#endif\r\n\r\nvarying vec3      var_VertLight;\r\n\r"
"\n#if defined(USE_LIGHT) && !defined(USE_DELUXEMAP)\r\nvarying vec3      va"
"r_WorldLight;\r\n#endif\r\n\r\nvarying vec4   var_ScreenPos;\r\n\r\n#define"
" EPSILON 0.00000001\r\n\r\n#if defined(USE_PARALLAXMAP)\r\nfloat SampleHeig"
"ht(sampler2D normalMap, vec2 t)\r\n{\r\n  #if defined(SWIZZLE_NORMALMAP)\r"
"\n\treturn texture2D(normalMap, t).r;\r\n  #else\r\n\treturn texture2D(norm"
"alMap, t).a;\r\n  #endif\r\n}\r\n\r\nfloat RayIntersectDisplaceMap(vec2 dp,"
" vec2 ds, sampler2D normalMap)\r\n{\r\n\tconst int linearSearchSteps = 16;"
"\r\n\tconst int binarySearchSteps = 6;\r\n\r\n\tfloat depthStep = 1.0 / flo"
"at(linearSearchSteps);\r\n\r\n\t// current size of search window\r\n\tfloat"
" size = depthStep;\r\n\r\n\t// current depth position\r\n\tfloat depth = 0."
"0;\r\n\r\n\t// best match found (starts with last position 1.0)\r\n\tfloat "
"bestDepth = 1.0;\r\n\r\n\t// search front to back for first point inside ob"
"ject\r\n\tfor(int i = 0; i < linearSearchSteps - 1; ++i)\r\n\t{\r\n\t\tdept"
"h += size;\r\n\t\t\r\n\t\tfloat t = 1.0 - SampleHeight(normalMap, dp + ds *"
" depth);\r\n\t\t\r\n\t\tif(bestDepth > 0.996)\t\t// if no depth found yet\r"
"\n\t\t\tif(depth >= t)\r\n\t\t\t\tbestDepth = depth;\t// store best depth\r"
"\n\t}\r\n\r\n\tdepth = bestDepth;\r\n\t\r\n\t// recurse around first point "
"(depth) for closest match\r\n\tfor(int i = 0; i < binarySearchSteps; ++i)\r"
"\n\t{\r\n\t\tsize *= 0.5;\r\n\r\n\t\tfloat t = 1.0 - SampleHeight(normalMap"
", dp + ds * depth);\r\n\t\t\r\n\t\tif(depth >= t)\r\n\t\t{\r\n\t\t\tbestDep"
"th = depth;\r\n\t\t\tdepth -= 2.0 * size;\r\n\t\t}\r\n\r\n\t\tdepth += size"
";\r\n\t}\r\n\r\n\treturn bestDepth;\r\n}\r\n#endif\r\n\r\nfloat CalcDiffuse"
"(vec3 N, vec3 L, vec3 E, float NE, float NL, float fzero, float shininess)"
"\r\n{\r\n  #if defined(USE_OREN_NAYAR) || defined(USE_TRIACE_OREN_NAYAR)\r"
"\n\tfloat gamma = dot(E, L) - NE * NL;\r\n\tfloat B = 2.22222 + 0.1 * shini"
"ness;\r\n\t\t\r\n\t#if defined(USE_OREN_NAYAR)\r\n\tfloat A = 1.0 - 1.0 / ("
"2.0 + 0.33 * shininess);\r\n\tgamma = clamp(gamma, 0.0, 1.0);\r\n\t#endif\r"
"\n\t\r\n\t#if defined(USE_TRIACE_OREN_NAYAR)\r\n\tfloat A = 1.0 - 1.0 / (2."
"0 + 0.65 * shininess);\r\n\r\n\tif (gamma >= 0.0)\r\n\t#endif\r\n\t{\r\n\t"
"\tB *= max(max(NL, NE), EPSILON);\r\n\t}\r\n\r\n\treturn A + gamma / B;\r\n"
"  #else\r\n\treturn 1.0 - fzero;\r\n  #endif\r\n}\r\n\r\n#if defined(USE_SP"
"ECULARMAP)\r\nfloat CalcSpecular(float NH, float NL, float NE, float EH, fl"
"oat fzero, float shininess)\r\n{\r\n  #if defined(USE_BLINN) || defined(USE"
"_TRIACE) || defined(USE_TORRANCE_SPARROW)\r\n\tfloat blinn = pow(NH, shinin"
"ess);\r\n  #endif\r\n\r\n  #if defined(USE_BLINN)\r\n\treturn blinn;\r\n  #"
"endif\r\n\r\n  #if defined(USE_COOK_TORRANCE) || defined (USE_TRIACE) || de"
"fined (USE_TORRANCE_SPARROW)\r\n\tfloat fresnel = fzero + (1.0 - fzero) * p"
"ow(1.0 - EH, 5);\r\n  #endif\r\n\r\n  #if defined(USE_COOK_TORRANCE) || def"
"ined(USE_TORRANCE_SPARROW)\r\n\tfloat geo = 2.0 * NH * min(NE, NL);\r\n\tge"
"o /= max(EH, geo);\r\n  #endif  \r\n\r\n  #if defined(USE_COOK_TORRANCE)\r"
"\n\tfloat m = sqrt(2.0 / max(shininess, EPSILON));\r\n\r\n\tfloat m_sq = m "
"* m;\r\n\tfloat NH_sq = NH * NH;\r\n\tfloat beckmann = exp((NH_sq - 1.0) / "
"max(m_sq * NH_sq, EPSILON)) / max(4.0 * m_sq * NH_sq * NH_sq, EPSILON);\r\n"
"\r\n\treturn fresnel * geo * beckmann / max(NE, EPSILON);\r\n  #endif\r\n\r"
"\n  #if defined(USE_TRIACE)\r\n\tfloat scale = 0.1248582 * shininess + 0.26"
"91817;\r\n\r\n\treturn fresnel * scale * blinn / max(max(NL, NE), EPSILON);"
"\r\n  #endif\r\n  \r\n  #if defined(USE_TORRANCE_SPARROW)\r\n\tfloat scale "
"= 0.125 * shininess + 1.0;\r\n\r\n\treturn fresnel * geo * scale * blinn / "
"max(NE, EPSILON);\r\n  #endif\r\n}\r\n#endif\r\n\r\nvoid main()\r\n{\r\n#if"
" defined(USE_LIGHT) || defined(USE_NORMALMAP)\r\n\tvec3 surfNormal = normal"
"ize(var_Normal);\r\n#endif\r\n\r\n#if defined(USE_DELUXEMAP)\r\n\tvec3 worl"
"dLight = 2.0 * texture2D(u_DeluxeMap, var_LightTex).xyz - vec3(1.0);\r\n\t/"
"/worldLight += var_WorldLight * 0.0001;\r\n#elif defined(USE_LIGHT)\r\n\tve"
"c3 worldLight = var_WorldLight;\r\n#endif\r\n\r\n#if defined(USE_LIGHTMAP)"
"\r\n\tvec4 lightSample = texture2D(u_LightMap, var_LightTex).rgba;\r\n  #if"
" defined(RGBE_LIGHTMAP)\r\n\tlightSample.rgb *= exp2(lightSample.a * 255.0 "
"- 128.0);\r\n  #endif\r\n\tvec3 directedLight = lightSample.rgb;\r\n#elif d"
"efined(USE_LIGHT_VECTOR)\r\n  #if defined(USE_FAST_LIGHT)\r\n\tvec3 directe"
"dLight = var_VertLight;\r\n  #else\r\n    #if defined(USE_INVSQRLIGHT)\r\n"
"\tfloat intensity = 1.0 / dot(worldLight, worldLight);\r\n    #else\r\n\tfl"
"oat intensity = clamp((1.0 - dot(worldLight, worldLight) / (u_LightRadius *"
" u_LightRadius)) * 1.07, 0.0, 1.0);\r\n    #endif\r\n\r\n\tvec3 directedLig"
"ht = u_DirectedLight * intensity;\r\n\tvec3 ambientLight  = u_AmbientLight;"
"\r\n  #endif\r\n\r\n  #if defined(USE_SHADOWMAP)\r\n\tvec2 shadowTex = var_"
"ScreenPos.xy / var_ScreenPos.w * 0.5 + 0.5;\r\n\tdirectedLight *= texture2D"
"(u_ShadowMap, shadowTex).r;\r\n  #endif\r\n#elif defined(USE_LIGHT_VERTEX)"
"\r\n\tvec3 directedLight = var_VertLight;\r\n#endif\r\n\t\r\n#if defined(TC"
"GEN_ENVIRONMENT) || defined(USE_NORMALMAP) || (defined(USE_LIGHT) && !defin"
"ed(USE_FAST_LIGHT))\r\n\tvec3 SampleToView = normalize(var_SampleToView);\r"
"\n#endif\r\n\tvec2 tex = var_DiffuseTex;\r\n\r\n\tfloat ambientDiff = 1.0;"
"\r\n\r\n#if defined(USE_NORMALMAP)\r\n  #if defined(USE_VERT_TANGENT_SPACE)"
"\r\n    vec3   tangent = var_Tangent;\r\n\tvec3 bitangent = var_Bitangent;"
"\r\n  #else\r\n\tvec3 q0  = dFdx(var_Position);\r\n\tvec3 q1  = dFdy(var_Po"
"sition);\r\n\tvec2 st0 = dFdx(tex);\r\n\tvec2 st1 = dFdy(tex);\r\n\tfloat d"
"ir = sign(st1.t * st0.s - st0.t * st1.s);\r\n\r\n\tvec3   tangent = normali"
"ze( q0 * st1.t - q1 * st0.t) * dir;\r\n\tvec3 bitangent = -normalize( q0 * "
"st1.s - q1 * st0.s) * dir;\r\n  #endif\r\n\r\n\tmat3 tangentToWorld = mat3("
"tangent, bitangent, var_Normal);\r\n\r\n  #if defined(USE_PARALLAXMAP)\r\n"
"\tvec3 offsetDir = normalize(SampleToView * tangentToWorld);\r\n    #if 0\r"
"\n    float height = SampleHeight(u_NormalMap, tex);\r\n\tfloat pdist = 0.0"
"5 * height - (0.05 / 2.0);\r\n    #else\r\n\toffsetDir.xy *= -0.05 / offset"
"Dir.z;\r\n\tfloat pdist = RayIntersectDisplaceMap(tex, offsetDir.xy, u_Norm"
"alMap);\r\n    #endif\t\r\n\ttex += offsetDir.xy * pdist;\r\n  #endif\r\n  "
"#if defined(SWIZZLE_NORMALMAP)\r\n\tvec3 normal = 2.0 * texture2D(u_NormalM"
"ap, tex).agb - 1.0;\r\n  #else\r\n\tvec3 normal = 2.0 * texture2D(u_NormalM"
"ap, tex).rgb - 1.0;\r\n  #endif\r\n\tnormal.z = sqrt(clamp(1.0 - dot(normal"
".xy, normal.xy), 0.0, 1.0));\r\n\tvec3 worldNormal = tangentToWorld * norma"
"l;\r\n  #if defined(r_normalAmbient)\r\n\tambientDiff = 0.781341 * normal.z"
" + 0.218659;\r\n  #endif\r\n#elif defined(USE_LIGHT)\r\n\tvec3 worldNormal "
"= surfNormal;\r\n#endif\r\n\r\n#if (defined(USE_LIGHT) && !defined(USE_FAST"
"_LIGHT)) || (defined(TCGEN_ENVIRONMENT) && defined(USE_NORMALMAP))\r\n\twor"
"ldNormal = normalize(worldNormal);\r\n#endif\r\n\r\n#if defined(TCGEN_ENVIR"
"ONMENT) && defined(USE_NORMALMAP)\r\n\tvec3 reflected = worldNormal * 2.0 *"
" dot(worldNormal, SampleToView) - SampleToView;\r\n\r\n\ttex = reflected.yz"
" * vec2(0.5, -0.5) + 0.5;\r\n#endif\r\n\r\n\tvec4 diffuse = texture2D(u_Dif"
"fuseMap, tex);\r\n\r\n#if defined(USE_LIGHT) && defined(USE_FAST_LIGHT)\r\n"
"\tdiffuse.rgb *= directedLight;\r\n#elif defined(USE_LIGHT)\r\n\tworldLight"
" = normalize(worldLight);\r\n\r\n  #if defined(USE_LIGHTMAP)\r\n\tdirectedL"
"ight /= max(dot(surfNormal, worldLight), 0.004);\r\n  #endif\r\n\r\n  #if d"
"efined(USE_LIGHTMAP) || defined(USE_LIGHT_VERTEX)\r\n\t#if defined(r_normal"
"Ambient)\r\n\tvec3 ambientLight = directedLight * r_normalAmbient;\r\n\tdir"
"ectedLight -= ambientLight;\r\n    #else\r\n\tvec3 ambientLight = vec3(0);"
"\r\n    #endif\r\n  #endif\r\n\r\n\tfloat NL = clamp(dot(worldNormal,  worl"
"dLight),   0.0, 1.0);\r\n\tfloat surfNL = clamp(dot(surfNormal,  worldLight"
"),   0.0, 1.0);\r\n\tNL = min(NL, surfNL * 2.0);\r\n\tfloat NE = clamp(dot("
"worldNormal,  SampleToView), 0.0, 1.0);\r\n\t\r\n\tfloat fzero = u_Material"
"Info.x;\r\n\tfloat shininess = u_MaterialInfo.y;\r\n  #if defined(USE_SPECU"
"LARMAP)\r\n\tvec4 specular = texture2D(u_SpecularMap, tex);\r\n\t//specular"
".rgb = clamp(specular.rgb - diffuse.rgb, 0.0, 1.0);\r\n\tshininess *= specu"
"lar.a;\r\n  #endif\r\n\tfloat directedDiff = NL * CalcDiffuse(worldNormal, "
"worldLight, SampleToView, NE, NL, fzero, shininess);\r\n\tdiffuse.rgb *= di"
"rectedLight * directedDiff + ambientDiff * ambientLight;\r\n  \r\n  #if def"
"ined(USE_SPECULARMAP)\r\n\tvec3 halfAngle = normalize(worldLight + SampleTo"
"View);\r\n\r\n\tfloat EH = clamp(dot(SampleToView, halfAngle), 0.0, 1.0);\r"
"\n\tfloat NH = clamp(dot(worldNormal,  halfAngle), 0.0, 1.0);\r\n\r\n\tfloa"
"t directedSpec = NL * CalcSpecular(NH, NL, NE, EH, fzero, shininess);\r\n  "
"\r\n    #if defined(r_normalAmbient)\r\n\tvec3 ambientHalf = normalize(surf"
"Normal + SampleToView);\r\n\tfloat ambientSpec = max(dot(ambientHalf, world"
"Normal) + 0.5, 0.0);\r\n\tambientSpec *= ambientSpec * 0.44;\r\n\tambientSp"
"ec = pow(ambientSpec, shininess) * fzero;\r\n\tspecular.rgb *= directedSpec"
" * directedLight + ambientSpec * ambientLight;\r\n    #else\r\n\tspecular.r"
"gb *= directedSpec * directedLight;\r\n    #endif\r\n  #endif\r\n#endif\r\n"
"\r\n\tgl_FragColor = diffuse;\r\n\r\n#if defined(USE_SPECULARMAP) && define"
"d(USE_LIGHT) && !defined(USE_FAST_LIGHT)\r\n\tgl_FragColor.rgb += specular."
"rgb;\r\n#endif\r\n\r\n\tgl_FragColor *= var_Color;\r\n}\r\n";

static const char *fallbackShadowfillShader_vp =
"attribute vec4  attr_Position;\r\nattribute vec3  attr_Normal;\r\nattribute"
" vec4  attr_TexCoord0;\r\n\r\n//#if defined(USE_VERTEX_ANIMATION)\r\nattrib"
"ute vec4  attr_Position2;\r\nattribute vec3  attr_Normal2;\r\n//#endif\r\n"
"\r\n//#if defined(USE_DEFORM_VERTEXES)\r\nuniform int     u_DeformGen;\r\nu"
"niform float    u_DeformParams[5];\r\n//#endif\r\n\r\nuniform float   u_Tim"
"e;\r\nuniform mat4    u_ModelViewProjectionMatrix;\r\n\r\nuniform mat4   u_"
"ModelMatrix;\r\n\r\n//#if defined(USE_VERTEX_ANIMATION)\r\nuniform float   "
"u_VertexLerp;\r\n//#endif\r\n\r\nvarying vec3    var_Position;\r\n\r\nfloat"
" triangle(float x)\r\n{\r\n\treturn max(1.0 - abs(x), 0);\r\n}\r\n\r\nfloat"
" sawtooth(float x)\r\n{\r\n\treturn x - floor(x);\r\n}\r\n\r\nvec4 DeformPo"
"sition(const vec4 pos, const vec3 normal, const vec2 st)\r\n{\r\n\tif (u_De"
"formGen == 0)\r\n\t{\r\n\t\treturn pos;\r\n\t}\r\n\r\n\tfloat base =      u"
"_DeformParams[0];\r\n\tfloat amplitude = u_DeformParams[1];\r\n\tfloat phas"
"e =     u_DeformParams[2];\r\n\tfloat frequency = u_DeformParams[3];\r\n\tf"
"loat spread =    u_DeformParams[4];\r\n\t\t\r\n\tif (u_DeformGen <= DGEN_WA"
"VE_INVERSE_SAWTOOTH)\r\n\t{\r\n\t\tphase += (pos.x + pos.y + pos.z) * sprea"
"d;\r\n\t}\r\n\telse if (u_DeformGen == DGEN_BULGE)\r\n\t{\r\n\t\tphase *= M"
"_PI * 0.25 * st.x;\r\n\t}\r\n\r\n\tfloat value = phase + (u_Time * frequenc"
"y);\r\n\tfloat func;\r\n\r\n\tif (u_DeformGen == DGEN_WAVE_SIN)\r\n\t{\r\n"
"\t\tfunc = sin(value * 2.0 * M_PI);\r\n\t}\r\n\telse if (u_DeformGen == DGE"
"N_WAVE_SQUARE)\r\n\t{\r\n\t\tfunc = sign(sin(value * 2.0 * M_PI));\r\n\t}\r"
"\n\telse if (u_DeformGen == DGEN_WAVE_TRIANGLE)\r\n\t{\r\n\t\tfunc = triang"
"le(value);\r\n\t}\r\n\telse if (u_DeformGen == DGEN_WAVE_SAWTOOTH)\r\n\t{\r"
"\n\t\tfunc = sawtooth(value);\r\n\t}\r\n\telse if (u_DeformGen == DGEN_WAVE"
"_INVERSE_SAWTOOTH)\r\n\t{\r\n\t\tfunc = (1.0 - sawtooth(value));\r\n\t}\r\n"
"\telse if (u_DeformGen == DGEN_BULGE)\r\n\t{\r\n\t\tfunc = sin(value);\r\n"
"\t}\r\n\r\n\tvec4 deformed = pos;\r\n\tdeformed.xyz += normal * (base + fun"
"c * amplitude);\r\n\r\n\treturn deformed;\r\n\r\n}\r\n\r\n\r\nvoid main()\r"
"\n{\r\n\tvec4 position = mix(attr_Position, attr_Position2, u_VertexLerp);"
"\r\n\tvec3 normal = normalize(mix(attr_Normal, attr_Normal2, u_VertexLerp))"
";\r\n\r\n\tposition = DeformPosition(position, normal, attr_TexCoord0.st);"
"\r\n\r\n\tgl_Position = u_ModelViewProjectionMatrix * position;\r\n\t\r\n\t"
"var_Position  = (u_ModelMatrix * position).xyz;\r\n}\r\n";

static const char *fallbackShadowfillShader_fp =
"uniform vec4  u_LightOrigin;\r\nuniform float u_LightRadius;\r\n\r\nvarying"
" vec3  var_Position;\r\n\r\nvoid main()\r\n{\r\n#if defined(USE_DEPTH)\r\n"
"\tfloat depth = length(u_LightOrigin.xyz - var_Position) / u_LightRadius;\r"
"\n #if 0\r\n\t// 32 bit precision\r\n\tconst vec4 bitSh = vec4( 256 * 256 *"
" 256,   256 * 256,         256,           1);\r\n\tconst vec4 bitMsk = vec4"
"(              0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0);\r\n\t\r\n\tvec4 c"
"omp;\r\n\tcomp = depth * bitSh;\r\n\tcomp.xyz = fract(comp.xyz);\r\n\tcomp "
"-= comp.xxyz * bitMsk;\r\n\tgl_FragColor = comp;\r\n #endif\r\n\r\n #if 1\r"
"\n\t// 24 bit precision\r\n\tconst vec3 bitSh = vec3( 256 * 256,         25"
"6,           1);\r\n\tconst vec3 bitMsk = vec3(        0, 1.0 / 256.0, 1.0 "
"/ 256.0);\r\n\t\r\n\tvec3 comp;\r\n\tcomp = depth * bitSh;\r\n\tcomp.xy = f"
"ract(comp.xy);\r\n\tcomp -= comp.xxy * bitMsk;\r\n\tgl_FragColor = vec4(com"
"p, 1.0);\r\n #endif\r\n\r\n #if 0\r\n\t// 8 bit precision\r\n\tgl_FragColor"
" = vec4(depth, depth, depth, 1);\r\n #endif\r\n#else\r\n\tgl_FragColor = ve"
"c4(0, 0, 0, 1);\r\n#endif\r\n}\r\n";

static const char *fallbackPshadowShader_vp =
"attribute vec4 attr_Position;\r\nattribute vec3 attr_Normal;\r\n\r\nuniform"
" mat4   u_ModelViewProjectionMatrix;\r\nvarying vec3   var_Position;\r\nvar"
"ying vec3   var_Normal;\r\n\r\n\r\nvoid main()\r\n{\r\n\tvec4 position  = a"
"ttr_Position;\r\n\r\n\tgl_Position = u_ModelViewProjectionMatrix * position"
";\r\n\r\n\tvar_Position  = position.xyz;\r\n\tvar_Normal    = attr_Normal;"
"\r\n}\r\n";

static const char *fallbackPshadowShader_fp =
"uniform sampler2D u_ShadowMap;\r\n\r\nuniform vec3      u_LightForward;\r\n"
"uniform vec3      u_LightUp;\r\nuniform vec3      u_LightRight;\r\nuniform "
"vec4      u_LightOrigin;\r\nuniform float     u_LightRadius;\r\nvarying vec"
"3      var_Position;\r\nvarying vec3      var_Normal;\r\n\r\nfloat sampleDi"
"stMap(sampler2D texMap, vec2 uv, float scale)\r\n{\r\n\tvec3 distv = textur"
"e2D(texMap, uv).xyz;\r\n\treturn dot(distv, vec3(1.0 / (256.0 * 256.0), 1.0"
" / 256.0, 1.0)) * scale;\r\n}\r\n\r\nvoid main()\r\n{\r\n\tvec3 lightToPos "
"= var_Position - u_LightOrigin.xyz;\r\n\tvec2 st = vec2(-dot(u_LightRight, "
"lightToPos), dot(u_LightUp, lightToPos));\r\n\t\r\n\tfloat fade = length(st"
");\r\n\t\r\n#if defined(USE_DISCARD)\r\n\tif (fade >= 1.0)\r\n\t{\r\n\t\tdi"
"scard;\r\n\t}\r\n#endif\r\n\r\n\tfade = clamp(8.0 - fade * 8.0, 0.0, 1.0);"
"\r\n\t\r\n\tst = st * 0.5 + vec2(0.5);\r\n\r\n#if defined(USE_SOLID_PSHADOW"
"S)\r\n\tfloat intensity = max(sign(u_LightRadius - length(lightToPos)), 0.0"
");\r\n#else\r\n\tfloat intensity = clamp((1.0 - dot(lightToPos, lightToPos)"
" / (u_LightRadius * u_LightRadius)) * 2.0, 0.0, 1.0);\r\n#endif\r\n\t\r\n\t"
"float lightDist = length(lightToPos);\r\n\tfloat dist;\r\n\r\n#if defined(U"
"SE_DISCARD)\r\n\tif (dot(u_LightForward, lightToPos) <= 0.0)\r\n\t{\r\n\t\t"
"discard;\r\n\t}\r\n\r\n\tif (dot(var_Normal, lightToPos) > 0.0)\r\n\t{\r\n"
"\t\tdiscard;\r\n\t}\r\n#else\r\n\tintensity *= max(sign(dot(u_LightForward,"
" lightToPos)), 0.0);\r\n\tintensity *= max(sign(-dot(var_Normal, lightToPos"
")), 0.0);\r\n#endif\r\n\r\n\tintensity *= fade;\r\n#if defined(USE_PCF)\r\n"
"\tfloat part;\r\n\t\r\n\tdist = sampleDistMap(u_ShadowMap, st + vec2(-1.0/5"
"12.0, -1.0/512.0), u_LightRadius);\r\n\tpart =  max(sign(lightDist - dist),"
" 0.0);\r\n\r\n\tdist = sampleDistMap(u_ShadowMap, st + vec2( 1.0/512.0, -1."
"0/512.0), u_LightRadius);\r\n\tpart += max(sign(lightDist - dist), 0.0);\r"
"\n\r\n\tdist = sampleDistMap(u_ShadowMap, st + vec2(-1.0/512.0,  1.0/512.0)"
", u_LightRadius);\r\n\tpart += max(sign(lightDist - dist), 0.0);\r\n\r\n\td"
"ist = sampleDistMap(u_ShadowMap, st + vec2( 1.0/512.0,  1.0/512.0), u_Light"
"Radius);\r\n\tpart += max(sign(lightDist - dist), 0.0);\r\n\r\n  #if define"
"d(USE_DISCARD)\r\n\tif (part <= 0.0)\r\n\t{\r\n\t\tdiscard;\r\n\t}\r\n  #en"
"dif\r\n\r\n\tintensity *= part * 0.25;\r\n#else\r\n\tdist = sampleDistMap(u"
"_ShadowMap, st, u_LightRadius);\r\n\r\n  #if defined(USE_DISCARD)\r\n\tif ("
"lightDist - dist <= 0.0)\r\n\t{\r\n\t\tdiscard;\r\n\t}\r\n  #endif\r\n\t\t"
"\t\r\n\tintensity *= max(sign(lightDist - dist), 0.0);\r\n#endif\r\n\t\t\r"
"\n\tgl_FragColor.rgb = vec3(0);\r\n\tgl_FragColor.a = clamp(intensity, 0.0,"
" 0.75);\r\n}\r\n";

static const char *fallbackDown4xShader_vp =
"attribute vec4 attr_Position;\r\nattribute vec4 attr_TexCoord0;\r\n\r\nunif"
"orm mat4   u_ModelViewProjectionMatrix;\r\n\r\nvarying vec2   var_TexCoords"
";\r\n\r\n\r\nvoid main()\r\n{\r\n\tgl_Position = u_ModelViewProjectionMatri"
"x * attr_Position;\r\n\tvar_TexCoords = attr_TexCoord0.st;\r\n}\r\n";

static const char *fallbackDown4xShader_fp =
"uniform sampler2D u_TextureMap;\r\n\r\nuniform vec2      u_InvTexRes;\r\nva"
"rying vec2      var_TexCoords;\r\n\r\nvoid main()\r\n{\r\n\tvec4 color;\r\n"
"\tvec2 tc;\r\n\t\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(-1.5, -1.5); "
" color  = texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRe"
"s * vec2(-0.5, -1.5);  color += texture2D(u_TextureMap, tc);\r\n\ttc = var_"
"TexCoords + u_InvTexRes * vec2( 0.5, -1.5);  color += texture2D(u_TextureMa"
"p, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2( 1.5, -1.5);  color +="
" texture2D(u_TextureMap, tc);\r\n\r\n\ttc = var_TexCoords + u_InvTexRes * v"
"ec2(-1.5, -0.5); color += texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoo"
"rds + u_InvTexRes * vec2(-0.5, -0.5); color += texture2D(u_TextureMap, tc);"
"\r\n\ttc = var_TexCoords + u_InvTexRes * vec2( 0.5, -0.5); color += texture"
"2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2( 1.5, -0"
".5); color += texture2D(u_TextureMap, tc);\r\n\r\n\ttc = var_TexCoords + u_"
"InvTexRes * vec2(-1.5,  0.5); color += texture2D(u_TextureMap, tc);\r\n\ttc"
" = var_TexCoords + u_InvTexRes * vec2(-0.5,  0.5); color += texture2D(u_Tex"
"tureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2( 0.5,  0.5); col"
"or += texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * "
"vec2( 1.5,  0.5); color += texture2D(u_TextureMap, tc);\r\n\r\n\ttc = var_T"
"exCoords + u_InvTexRes * vec2(-1.5,  1.5);  color += texture2D(u_TextureMap"
", tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(-0.5,  1.5);  color += "
"texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2( "
"0.5,  1.5);  color += texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords "
"+ u_InvTexRes * vec2( 1.5,  1.5);  color += texture2D(u_TextureMap, tc);\r"
"\n\t\r\n\tcolor *= 0.0625;\r\n\t\r\n\tgl_FragColor = color;\r\n}\r\n";

static const char *fallbackBokehShader_vp =
"attribute vec4 attr_Position;\r\nattribute vec4 attr_TexCoord0;\r\n\r\nunif"
"orm mat4   u_ModelViewProjectionMatrix;\r\n\r\nvarying vec2   var_TexCoords"
";\r\n\r\n\r\nvoid main()\r\n{\r\n\tgl_Position = u_ModelViewProjectionMatri"
"x * attr_Position;\r\n\tvar_TexCoords = attr_TexCoord0.st;\r\n}\r\n";

static const char *fallbackBokehShader_fp =
"uniform sampler2D u_TextureMap;\r\n\r\nuniform vec4      u_Color;\r\n\r\nun"
"iform vec2      u_InvTexRes;\r\nvarying vec2      var_TexCoords;\r\n\r\nvoi"
"d main()\r\n{\r\n\tvec4 color;\r\n\tvec2 tc;\r\n\r\n#if 0\r\n\tfloat c[7] ="
" float[7](1.0, 0.9659258263, 0.8660254038, 0.7071067812, 0.5, 0.2588190451,"
" 0.0);\r\n\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(  c[0],  c[6]);  co"
"lor =  texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes *"
" vec2(  c[1],  c[5]);  color += texture2D(u_TextureMap, tc);\r\n\ttc = var_"
"TexCoords + u_InvTexRes * vec2(  c[2],  c[4]);  color += texture2D(u_Textur"
"eMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(  c[3],  c[3]);  co"
"lor += texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes *"
" vec2(  c[4],  c[2]);  color += texture2D(u_TextureMap, tc);\r\n\ttc = var_"
"TexCoords + u_InvTexRes * vec2(  c[5],  c[1]);  color += texture2D(u_Textur"
"eMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(  c[6],  c[0]);  co"
"lor += texture2D(u_TextureMap, tc);\r\n\r\n\ttc = var_TexCoords + u_InvTexR"
"es * vec2(  c[1], -c[5]);  color += texture2D(u_TextureMap, tc);\r\n\ttc = "
"var_TexCoords + u_InvTexRes * vec2(  c[2], -c[4]);  color += texture2D(u_Te"
"xtureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(  c[3], -c[3]);"
"  color += texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexR"
"es * vec2(  c[4], -c[2]);  color += texture2D(u_TextureMap, tc);\r\n\ttc = "
"var_TexCoords + u_InvTexRes * vec2(  c[5], -c[1]);  color += texture2D(u_Te"
"xtureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(  c[6], -c[0]);"
"  color += texture2D(u_TextureMap, tc);\r\n\r\n\ttc = var_TexCoords + u_Inv"
"TexRes * vec2( -c[0],  c[6]);  color += texture2D(u_TextureMap, tc);\r\n\tt"
"c = var_TexCoords + u_InvTexRes * vec2( -c[1],  c[5]);  color += texture2D("
"u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2( -c[2],  c["
"4]);  color += texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_Inv"
"TexRes * vec2( -c[3],  c[3]);  color += texture2D(u_TextureMap, tc);\r\n\tt"
"c = var_TexCoords + u_InvTexRes * vec2( -c[4],  c[2]);  color += texture2D("
"u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2( -c[5],  c["
"1]);  color += texture2D(u_TextureMap, tc);\r\n\r\n\ttc = var_TexCoords + u"
"_InvTexRes * vec2( -c[1], -c[5]);  color += texture2D(u_TextureMap, tc);\r"
"\n\ttc = var_TexCoords + u_InvTexRes * vec2( -c[2], -c[4]);  color += textu"
"re2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2( -c[3]"
", -c[3]);  color += texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + "
"u_InvTexRes * vec2( -c[4], -c[2]);  color += texture2D(u_TextureMap, tc);\r"
"\n\ttc = var_TexCoords + u_InvTexRes * vec2( -c[5], -c[1]);  color += textu"
"re2D(u_TextureMap, tc);\r\n\t\r\n\tgl_FragColor = color * 0.04166667 * u_Co"
"lor;\r\n#endif\r\n\r\n\tfloat c[5] = float[5](1.0, 0.9238795325, 0.70710678"
"12, 0.3826834324, 0.0);\r\n\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(  "
"c[0],  c[4]);  color =  texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoord"
"s + u_InvTexRes * vec2(  c[1],  c[3]);  color += texture2D(u_TextureMap, tc"
");\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(  c[2],  c[2]);  color += t"
"exture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(  "
"c[3],  c[1]);  color += texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoord"
"s + u_InvTexRes * vec2(  c[4],  c[0]);  color += texture2D(u_TextureMap, tc"
");\r\n\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(  c[1], -c[3]);  color "
"+= texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec"
"2(  c[2], -c[2]);  color += texture2D(u_TextureMap, tc);\r\n\ttc = var_TexC"
"oords + u_InvTexRes * vec2(  c[3], -c[1]);  color += texture2D(u_TextureMap"
", tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2(  c[4], -c[0]);  color "
"+= texture2D(u_TextureMap, tc);\r\n\r\n\ttc = var_TexCoords + u_InvTexRes *"
" vec2( -c[0],  c[4]);  color += texture2D(u_TextureMap, tc);\r\n\ttc = var_"
"TexCoords + u_InvTexRes * vec2( -c[1],  c[3]);  color += texture2D(u_Textur"
"eMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2( -c[2],  c[2]);  co"
"lor += texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes *"
" vec2( -c[3],  c[1]);  color += texture2D(u_TextureMap, tc);\r\n\r\n\ttc = "
"var_TexCoords + u_InvTexRes * vec2( -c[1], -c[3]);  color += texture2D(u_Te"
"xtureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexRes * vec2( -c[2], -c[2]);"
"  color += texture2D(u_TextureMap, tc);\r\n\ttc = var_TexCoords + u_InvTexR"
"es * vec2( -c[3], -c[1]);  color += texture2D(u_TextureMap, tc);\r\n\t\r\n"
"\tgl_FragColor = color * 0.0625 * u_Color;\r\n}\r\n";

static const char *fallbackToneMapShader_vp =
"attribute vec4 attr_Position;\r\nattribute vec4 attr_TexCoord0;\r\n\r\nunif"
"orm mat4   u_ModelViewProjectionMatrix;\r\n\r\nvarying vec2   var_TexCoords"
";\r\n\r\n\r\nvoid main()\r\n{\r\n\tgl_Position = u_ModelViewProjectionMatri"
"x * attr_Position;\r\n\tvar_TexCoords = attr_TexCoord0.st;\r\n}\r\n";

static const char *fallbackToneMapShader_fp =
"uniform sampler2D u_TextureMap;\r\nuniform sampler2D u_LevelsMap;\r\n\r\nun"
"iform vec4      u_Color;\r\n\r\nuniform vec2      u_AutoExposureMinMax;\r\n"
"uniform vec3      u_ToneMinAvgMaxLinear;\r\n\r\nvarying vec2      var_TexCo"
"ords;\r\n\r\nconst vec3  LUMINANCE_VECTOR =   vec3(0.2125, 0.7154, 0.0721);"
" //vec3(0.299, 0.587, 0.114);\r\n\r\nvec3 FilmicTonemap(vec3 x)\r\n{\r\n\tc"
"onst float SS  = 0.22; // Shoulder Strength\r\n\tconst float LS  = 0.30; //"
" Linear Strength\r\n\tconst float LA  = 0.10; // Linear Angle\r\n\tconst fl"
"oat TS  = 0.20; // Toe Strength\r\n\tconst float TAN = 0.01; // Toe Angle N"
"umerator\r\n\tconst float TAD = 0.30; // Toe Angle Denominator\r\n\t\r\n\tv"
"ec3 SSxx = SS * x * x;\r\n\tvec3 LSx = LS * x;\r\n\tvec3 LALSx = LSx * LA;"
"\r\n\t\r\n\treturn ((SSxx + LALSx + TS * TAN) / (SSxx + LSx + TS * TAD)) - "
"TAN / TAD;\r\n\r\n\t//return ((x*(SS*x+LA*LS)+TS*TAN)/(x*(SS*x+LS)+TS*TAD))"
" - TAN/TAD;\r\n\r\n}\r\n\r\nvoid main()\r\n{\r\n\tvec4 color = texture2D(u_"
"TextureMap, var_TexCoords) * u_Color;\r\n\tvec3 minAvgMax = texture2D(u_Lev"
"elsMap, var_TexCoords).rgb;\r\n\tvec3 logMinAvgMaxLum = clamp(minAvgMax * 2"
"0.0 - 10.0, -u_AutoExposureMinMax.y, -u_AutoExposureMinMax.x);\r\n\t\t\r\n"
"\tfloat avgLum = exp2(logMinAvgMaxLum.y);\r\n\t//float maxLum = exp2(logMin"
"AvgMaxLum.z);\r\n\r\n\tcolor.rgb *= u_ToneMinAvgMaxLinear.y / avgLum;\r\n\t"
"color.rgb = max(vec3(0.0), color.rgb - vec3(u_ToneMinAvgMaxLinear.x));\r\n"
"\r\n\tvec3 fWhite = 1.0 / FilmicTonemap(vec3(u_ToneMinAvgMaxLinear.z - u_To"
"neMinAvgMaxLinear.x));\r\n\tcolor.rgb = FilmicTonemap(color.rgb) * fWhite;"
"\r\n\t\r\n\tgl_FragColor = clamp(color, 0.0, 1.0);\r\n}\r\n";

static const char *fallbackCalcLevels4xShader_vp =
"attribute vec4 attr_Position;\r\nattribute vec4 attr_TexCoord0;\r\n\r\nunif"
"orm mat4   u_ModelViewProjectionMatrix;\r\n\r\nvarying vec2   var_TexCoords"
";\r\n\r\n\r\nvoid main()\r\n{\r\n\tgl_Position = u_ModelViewProjectionMatri"
"x * attr_Position;\r\n\tvar_TexCoords = attr_TexCoord0.st;\r\n}\r\n";

static const char *fallbackCalcLevels4xShader_fp =
"uniform sampler2D u_TextureMap;\r\n\r\nuniform vec4      u_Color;\r\n\r\nun"
"iform vec2      u_InvTexRes;\r\nvarying vec2      var_TexCoords;\r\n\r\ncon"
"st vec3  LUMINANCE_VECTOR =   vec3(0.2125, 0.7154, 0.0721); //vec3(0.299, 0"
".587, 0.114);\r\n\r\nvec3 GetValues(vec2 offset, vec3 current)\r\n{\r\n\tve"
"c3 minAvgMax;\r\n\tvec2 tc = var_TexCoords + u_InvTexRes * offset; minAvgMa"
"x = texture2D(u_TextureMap, tc).rgb;\r\n\r\n#ifdef FIRST_PASS\r\n\tfloat lu"
"mi = max(dot(LUMINANCE_VECTOR, minAvgMax), 0.000001);\r\n\tfloat loglumi = "
"clamp(log2(lumi), -10.0, 10.0);\r\n\tminAvgMax = vec3(loglumi * 0.05 + 0.5)"
";\r\n#endif\r\n\r\n\treturn vec3(min(current.x, minAvgMax.x), current.y + m"
"inAvgMax.y, max(current.z, minAvgMax.z));\r\n}\r\n\r\nvoid main()\r\n{\r\n"
"\tvec3 current = vec3(1.0, 0.0, 0.0);\r\n\r\n#ifdef FIRST_PASS\r\n\tcurrent"
" = GetValues(vec2( 0.0,  0.0), current);\r\n#else\r\n\tcurrent = GetValues("
"vec2(-1.5, -1.5), current);\r\n\tcurrent = GetValues(vec2(-0.5, -1.5), curr"
"ent);\r\n\tcurrent = GetValues(vec2( 0.5, -1.5), current);\r\n\tcurrent = G"
"etValues(vec2( 1.5, -1.5), current);\r\n\t\r\n\tcurrent = GetValues(vec2(-1"
".5, -0.5), current);\r\n\tcurrent = GetValues(vec2(-0.5, -0.5), current);\r"
"\n\tcurrent = GetValues(vec2( 0.5, -0.5), current);\r\n\tcurrent = GetValue"
"s(vec2( 1.5, -0.5), current);\r\n\t\r\n\tcurrent = GetValues(vec2(-1.5,  0."
"5), current);\r\n\tcurrent = GetValues(vec2(-0.5,  0.5), current);\r\n\tcur"
"rent = GetValues(vec2( 0.5,  0.5), current);\r\n\tcurrent = GetValues(vec2("
" 1.5,  0.5), current);\r\n\r\n\tcurrent = GetValues(vec2(-1.5,  1.5), curre"
"nt);\r\n\tcurrent = GetValues(vec2(-0.5,  1.5), current);\r\n\tcurrent = Ge"
"tValues(vec2( 0.5,  1.5), current);\r\n\tcurrent = GetValues(vec2( 1.5,  1."
"5), current);\r\n\r\n\tcurrent.y *= 0.0625;\r\n#endif\r\n\r\n\tgl_FragColor"
" = vec4(current, 1.0f);\r\n}\r\n";

static const char *fallbackShadowmaskShader_vp =
"attribute vec4 attr_Position;\r\nattribute vec4 attr_TexCoord0;\r\n\r\nunif"
"orm vec3   u_ViewForward;\r\nuniform vec3   u_ViewLeft;\r\nuniform vec3   u"
"_ViewUp;\r\nuniform vec4   u_ViewInfo; // zfar / znear\r\n\r\nvarying vec2 "
"  var_ScreenTex;\r\nvarying vec3   var_ViewDir;\r\n\r\nvoid main()\r\n{\r\n"
"\tgl_Position = attr_Position;\r\n\t//vec2 screenCoords = gl_Position.xy / "
"gl_Position.w;\r\n\t//var_ScreenTex = screenCoords * 0.5 + 0.5;\r\n\tvar_Sc"
"reenTex = attr_TexCoord0.xy;\r\n\tvec2 screenCoords = attr_TexCoord0.xy * 2"
".0 - 1.0;\r\n\tvar_ViewDir = u_ViewForward + u_ViewLeft * -screenCoords.x +"
" u_ViewUp * screenCoords.y;\r\n}\r\n";

static const char *fallbackShadowmaskShader_fp =
"uniform sampler2D u_ScreenDepthMap;\r\n\r\nuniform sampler2D u_ShadowMap;\r"
"\n#if defined(USE_SHADOW_CASCADE)\r\nuniform sampler2D u_ShadowMap2;\r\nuni"
"form sampler2D u_ShadowMap3;\r\n#endif\r\n\r\nuniform mat4      u_ShadowMvp"
";\r\n#if defined(USE_SHADOW_CASCADE)\r\nuniform mat4      u_ShadowMvp2;\r\n"
"uniform mat4      u_ShadowMvp3;\r\n#endif\r\n\r\nuniform vec3   u_ViewOrigi"
"n;\r\nuniform vec4   u_ViewInfo; // zfar / znear, zfar\r\n\r\nvarying vec2 "
"  var_ScreenTex;\r\nvarying vec3   var_ViewDir;\r\n\r\n// Input: It uses te"
"xture coords as the random number seed.\r\n// Output: Random number: [0,1),"
" that is between 0.0 and 0.999999... inclusive.\r\n// Author: Michael Pohor"
"eski\r\n// Copyright: Copyleft 2012 :-)\r\n// Source: http://stackoverflow."
"com/questions/5149544/can-i-generate-a-random-number-inside-a-pixel-shader"
"\r\n\r\nfloat random( const vec2 p )\r\n{\r\n  // We need irrationals for p"
"seudo randomness.\r\n  // Most (all?) known transcendental numbers will (ge"
"nerally) work.\r\n  const vec2 r = vec2(\r\n    23.1406926327792690,  // e^"
"pi (Gelfond's constant)\r\n     2.6651441426902251); // 2^sqrt(2) (Gelfond–"
"Schneider constant)\r\n  //return fract( cos( mod( 123456789., 1e-7 + 256. "
"* dot(p,r) ) ) );\r\n  return mod( 123456789., 1e-7 + 256. * dot(p,r) );  "
"\r\n}\r\n\r\nfloat PCF(const sampler2D shadowmap, const vec2 st, const floa"
"t dist)\r\n{\r\n\tfloat mult;\r\n\tfloat scale = 2.0 / r_shadowMapSize;\r\n"
"\t\t\r\n#if defined(USE_SHADOW_FILTER)\r\n\tfloat r = random(var_ScreenTex."
"xy);\r\n\tfloat sinr = sin(r) * scale;\r\n\tfloat cosr = cos(r) * scale;\r"
"\n\tmat2 rmat = mat2(cosr, sinr, -sinr, cosr);\r\n\r\n\tmult =  step(dist, "
"texture2D(shadowmap, st + rmat * vec2(-0.7055767, 0.196515)).r);\r\n\tmult "
"+= step(dist, texture2D(shadowmap, st + rmat * vec2(0.3524343, -0.7791386))"
".r);\r\n\tmult += step(dist, texture2D(shadowmap, st + rmat * vec2(0.239105"
"6, 0.9189604)).r);\r\n  #if defined(USE_SHADOW_FILTER2)\r\n\tmult += step(d"
"ist, texture2D(shadowmap, st + rmat * vec2(-0.07580382, -0.09224417)).r);\r"
"\n\tmult += step(dist, texture2D(shadowmap, st + rmat * vec2(0.5784913, -0."
"002528916)).r);\r\n\tmult += step(dist, texture2D(shadowmap, st + rmat * ve"
"c2(0.192888, 0.4064181)).r);\r\n\tmult += step(dist, texture2D(shadowmap, s"
"t + rmat * vec2(-0.6335801, -0.5247476)).r);\r\n\tmult += step(dist, textur"
"e2D(shadowmap, st + rmat * vec2(-0.5579782, 0.7491854)).r);\r\n\tmult += st"
"ep(dist, texture2D(shadowmap, st + rmat * vec2(0.7320465, 0.6317794)).r);\r"
"\n\r\n\tmult *= 0.11111;\r\n  #else\r\n    mult *= 0.33333;\r\n  #endif\r\n"
"#else\r\n\tmult = step(dist, texture2D(shadowmap, st).r);\r\n#endif\r\n\t\t"
"\r\n\treturn mult;\r\n}\r\n\r\nfloat getLinearDepth(sampler2D depthMap, vec"
"2 tex, float zFarDivZNear)\r\n{\r\n\t\tfloat sampleZDivW = texture2D(depthM"
"ap, tex).r;\r\n\t\treturn 1.0 / mix(zFarDivZNear, 1.0, sampleZDivW);\r\n}\r"
"\n\r\nvoid main()\r\n{\r\n\tfloat result;\r\n\t\r\n\tfloat depth = getLinea"
"rDepth(u_ScreenDepthMap, var_ScreenTex, u_ViewInfo.x);\r\n\tfloat sampleZ ="
" u_ViewInfo.y * depth;\r\n\r\n\tvec4 biasPos = vec4(u_ViewOrigin + var_View"
"Dir * depth * 0.99, 1.0);\r\n\t\r\n\tvec4 shadowpos = u_ShadowMvp * biasPos"
";\r\n\t\r\n#if defined(USE_SHADOW_CASCADE)\r\n\tconst float fadeTo = 0.5;\r"
"\n\tresult = fadeTo;\r\n#else\r\n\tresult = 0.0;\r\n#endif\r\n\r\n\tif (all"
"(lessThanEqual(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))\r\n\t{\r\n\t\t"
"shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;\r\n\t\tresult = PC"
"F(u_ShadowMap, shadowpos.xy, shadowpos.z);\r\n\t}\r\n#if defined(USE_SHADOW"
"_CASCADE)\r\n\telse\r\n\t{\r\n\t\tshadowpos = u_ShadowMvp2 * biasPos;\r\n\r"
"\n\t\tif (all(lessThanEqual(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))\r"
"\n\t\t{\r\n\t\t\tshadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;\r"
"\n\t\t\tresult = PCF(u_ShadowMap2, shadowpos.xy, shadowpos.z);\r\n\t\t}\r\n"
"\t\telse\r\n\t\t{\r\n\t\t\tshadowpos = u_ShadowMvp3 * biasPos;\r\n\r\n\t\t"
"\tif (all(lessThanEqual(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))\r\n\t"
"\t\t{\r\n\t\t\t\tshadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;\r"
"\n\t\t\t\tresult = PCF(u_ShadowMap3, shadowpos.xy, shadowpos.z);\r\n\r\n\t"
"\t\t\tfloat fade = clamp(sampleZ / r_shadowCascadeZFar * 10.0 - 9.0, 0.0, 1"
".0);\r\n\t\t\t\tresult = mix(result, fadeTo, fade);\r\n\t\t\t}\r\n\t\t}\r\n"
"\t}\r\n#endif\r\n\t\t\r\n\tgl_FragColor = vec4(vec3(result), 1.0);\r\n}\r\n";

static const char *fallbackSsaoShader_vp =
"attribute vec4 attr_Position;\r\nattribute vec4 attr_TexCoord0;\r\n\r\nvary"
"ing vec2   var_ScreenTex;\r\n\r\nvoid main()\r\n{\r\n\tgl_Position = attr_P"
"osition;\r\n\tvar_ScreenTex = attr_TexCoord0.xy;\r\n\t//vec2 screenCoords ="
" gl_Position.xy / gl_Position.w;\r\n\t//var_ScreenTex = screenCoords * 0.5 "
"+ 0.5;\r\n}\r\n";

static const char *fallbackSsaoShader_fp =
"uniform sampler2D u_ScreenDepthMap;\r\n\r\nuniform vec4   u_ViewInfo; // zf"
"ar / znear, zfar\r\n\r\nvarying vec2   var_ScreenTex;\r\n\r\nvec2 poissonDi"
"sc[9] = vec2[9](\r\nvec2(-0.7055767, 0.196515),    vec2(0.3524343, -0.77913"
"86),\r\nvec2(0.2391056, 0.9189604),    vec2(-0.07580382, -0.09224417),\r\nv"
"ec2(0.5784913, -0.002528916), vec2(0.192888, 0.4064181),\r\nvec2(-0.6335801"
", -0.5247476),  vec2(-0.5579782, 0.7491854),\r\nvec2(0.7320465, 0.6317794)"
"\r\n);\r\n\r\n// Input: It uses texture coords as the random number seed.\r"
"\n// Output: Random number: [0,1), that is between 0.0 and 0.999999... incl"
"usive.\r\n// Author: Michael Pohoreski\r\n// Copyright: Copyleft 2012 :-)\r"
"\n// Source: http://stackoverflow.com/questions/5149544/can-i-generate-a-ra"
"ndom-number-inside-a-pixel-shader\r\n\r\nfloat random( const vec2 p )\r\n{"
"\r\n  // We need irrationals for pseudo randomness.\r\n  // Most (all?) kno"
"wn transcendental numbers will (generally) work.\r\n  const vec2 r = vec2("
"\r\n    23.1406926327792690,  // e^pi (Gelfond's constant)\r\n     2.665144"
"1426902251); // 2^sqrt(2) (Gelfond–Schneider constant)\r\n  //return fract("
" cos( mod( 123456789., 1e-7 + 256. * dot(p,r) ) ) );\r\n  return mod( 12345"
"6789., 1e-7 + 256. * dot(p,r) );  \r\n}\r\n\r\nmat2 randomRotation( const v"
"ec2 p )\r\n{\r\n\tfloat r = random(p);\r\n\tfloat sinr = sin(r);\r\n\tfloat"
" cosr = cos(r);\r\n\treturn mat2(cosr, sinr, -sinr, cosr);\r\n}\r\n\r\nfloa"
"t getLinearDepth(sampler2D depthMap, const vec2 tex, const float zFarDivZNe"
"ar)\r\n{\r\n\t\tfloat sampleZDivW = texture2D(depthMap, tex).r;\r\n\t\tretu"
"rn 1.0 / mix(zFarDivZNear, 1.0, sampleZDivW);\r\n}\r\n\r\nfloat ambientOccl"
"usion(sampler2D depthMap, const vec2 tex, const float zFarDivZNear, const f"
"loat zFar)\r\n{\r\n\tfloat result = 0;\r\n\r\n\tfloat sampleZ = zFar * getL"
"inearDepth(depthMap, tex, zFarDivZNear);\r\n\r\n\tvec2 expectedSlope = vec2"
"(dFdx(sampleZ), dFdy(sampleZ)) / vec2(dFdx(tex.x), dFdy(tex.y));\r\n\t\r\n"
"\tif (length(expectedSlope) > 5000.0)\r\n\t\treturn 1.0;\r\n\t\r\n\tvec2 of"
"fsetScale = vec2(3.0 / sampleZ);\r\n\t\r\n\tmat2 rmat = randomRotation(tex)"
";\r\n\t\t\r\n\tint i;\r\n\tfor (i = 0; i < 3; i++)\r\n\t{\r\n\t\tvec2 offse"
"t = rmat * poissonDisc[i] * offsetScale;\r\n\t\tfloat sampleZ2 = zFar * get"
"LinearDepth(depthMap, tex + offset, zFarDivZNear);\r\n\r\n\t\tif (abs(sampl"
"eZ - sampleZ2) > 20.0)\r\n\t\t\tresult += 1.0;\r\n\t\telse\r\n\t\t{\r\n\t\t"
"\tfloat expectedZ = sampleZ + dot(expectedSlope, offset);\r\n\t\t\tresult +"
"= step(expectedZ - 1.0, sampleZ2);\r\n\t\t}\r\n\t}\r\n\t\r\n\tresult *= 0.3"
"3333;\r\n\t\r\n\treturn result;\r\n}\r\n\r\nvoid main()\r\n{\r\n\tfloat res"
"ult = ambientOcclusion(u_ScreenDepthMap, var_ScreenTex, u_ViewInfo.x, u_Vie"
"wInfo.y);\r\n\t\t\t\r\n\tgl_FragColor = vec4(vec3(result), 1.0);\r\n}\r\n";

static const char *fallbackDepthBlurShader_vp =
"attribute vec4 attr_Position;\r\nattribute vec4 attr_TexCoord0;\r\n\r\nvary"
"ing vec2   var_ScreenTex;\r\n\r\nvoid main()\r\n{\r\n\tgl_Position = attr_P"
"osition;\r\n\tvar_ScreenTex = attr_TexCoord0.xy;\r\n\t//vec2 screenCoords ="
" gl_Position.xy / gl_Position.w;\r\n\t//var_ScreenTex = screenCoords * 0.5 "
"+ 0.5;\r\n}\r\n";

static const char *fallbackDepthBlurShader_fp =
"uniform sampler2D u_ScreenImageMap;\r\nuniform sampler2D u_ScreenDepthMap;"
"\r\n\r\nuniform vec4   u_ViewInfo; // zfar / znear, zfar\r\nvarying vec2   "
"var_ScreenTex;\r\n\r\n//float gauss[5] = float[5](0.30, 0.23, 0.097, 0.024,"
" 0.0033);\r\nfloat gauss[4] = float[4](0.40, 0.24, 0.054, 0.0044);\r\n//flo"
"at gauss[3] = float[3](0.60, 0.19, 0.0066);\r\n#define GAUSS_SIZE 4\r\n\r\n"
"float getLinearDepth(sampler2D depthMap, const vec2 tex, const float zFarDi"
"vZNear)\r\n{\r\n\t\tfloat sampleZDivW = texture2D(depthMap, tex).r;\r\n\t\t"
"return 1.0 / mix(zFarDivZNear, 1.0, sampleZDivW);\r\n}\r\n\r\nvec4 depthGau"
"ssian1D(sampler2D imageMap, sampler2D depthMap, vec2 tex, float zFarDivZNea"
"r, float zFar)\r\n{\r\n\tfloat scale = 1.0 / 256.0;\r\n\r\n#if defined(USE_"
"HORIZONTAL_BLUR)\r\n    vec2 direction = vec2(1.0, 0.0) * scale;\r\n#else /"
"/ if defined(USE_VERTICAL_BLUR)\r\n\tvec2 direction = vec2(0.0, 1.0) * scal"
"e;\r\n#endif\r\n\t\r\n\tfloat depthCenter = zFar * getLinearDepth(depthMap,"
" tex, zFarDivZNear);\r\n\tvec2 centerSlope = vec2(dFdx(depthCenter), dFdy(d"
"epthCenter)) / vec2(dFdx(tex.x), dFdy(tex.y));\r\n\t\t\r\n\tvec4 result = t"
"exture2D(imageMap, tex) * gauss[0];\r\n\tfloat total = gauss[0];\r\n\r\n\ti"
"nt i, j;\r\n\tfor (i = 0; i < 2; i++)\r\n\t{\r\n\t\tfor (j = 1; j < GAUSS_S"
"IZE; j++)\r\n\t\t{\r\n\t\t\tvec2 offset = direction * j;\r\n\t\t\tfloat dep"
"thSample = zFar * getLinearDepth(depthMap, tex + offset, zFarDivZNear);\r\n"
"\t\t\tfloat depthExpected = depthCenter + dot(centerSlope, offset);\r\n\t\t"
"\tif(abs(depthSample - depthExpected) < 5.0)\r\n\t\t\t{\r\n\t\t\t\tresult +"
"= texture2D(imageMap, tex + offset) * gauss[j];\r\n\t\t\t\ttotal += gauss[j"
"];\r\n\t\t\t}\r\n\t\t}\r\n\t\t\r\n\t\tdirection = -direction;\r\n\t}\t\r\n"
"\t\t\r\n\treturn result / total;\r\n}\r\n\r\nvoid main()\r\n{\t\t\r\n\tgl_F"
"ragColor = depthGaussian1D(u_ScreenImageMap, u_ScreenDepthMap, var_ScreenTe"
"x, u_ViewInfo.x, u_ViewInfo.y);\r\n}\r\n";


static void GLSL_PrintInfoLog(GLhandleARB object, qboolean developerOnly)
{
	char           *msg;
	static char     msgPart[1024];
	int             maxLength = 0;
	int             i;
	int             printLevel = developerOnly ? PRINT_DEVELOPER : PRINT_ALL;

	qglGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &maxLength);

	if (maxLength <= 0)
	{
		ri.Printf(printLevel, "No compile log.\n");
		return;
	}

	ri.Printf(printLevel, "compile log:\n");

	if (maxLength < 1023)
	{
		qglGetInfoLogARB(object, maxLength, &maxLength, msgPart);

		msgPart[maxLength + 1] = '\0';

		ri.Printf(printLevel, "%s\n", msgPart);
	}
	else
	{
		msg = ri.Malloc(maxLength);

		qglGetInfoLogARB(object, maxLength, &maxLength, msg);

		for(i = 0; i < maxLength; i += 1024)
		{
			Q_strncpyz(msgPart, msg + i, sizeof(msgPart));

			ri.Printf(printLevel, "%s\n", msgPart);
		}

		ri.Free(msg);
	}
}

static void GLSL_PrintShaderSource(GLhandleARB object)
{
	char           *msg;
	static char     msgPart[1024];
	int             maxLength = 0;
	int             i;

	qglGetObjectParameterivARB(object, GL_OBJECT_SHADER_SOURCE_LENGTH_ARB, &maxLength);

	msg = ri.Malloc(maxLength);

	qglGetShaderSourceARB(object, maxLength, &maxLength, msg);

	for(i = 0; i < maxLength; i += 1024)
	{
		Q_strncpyz(msgPart, msg + i, sizeof(msgPart));
		ri.Printf(PRINT_ALL, "%s\n", msgPart);
	}

	ri.Free(msg);
}

static void GLSL_GetShaderHeader( GLenum shaderType, const GLcharARB *extra, char *dest, int size )
{
	float fbufWidthScale, fbufHeightScale;

	dest[0] = '\0';

	// HACK: abuse the GLSL preprocessor to turn GLSL 1.20 shaders into 1.30 ones
	if(glRefConfig.glslMajorVersion > 1 || (glRefConfig.glslMajorVersion == 1 && glRefConfig.glslMinorVersion >= 30))
	{
		Q_strcat(dest, size, "#version 130\n");

		if(shaderType == GL_VERTEX_SHADER_ARB)
		{
			Q_strcat(dest, size, "#define attribute in\n");
			Q_strcat(dest, size, "#define varying out\n");
		}
		else
		{
			Q_strcat(dest, size, "#define varying in\n");

			Q_strcat(dest, size, "out vec4 out_Color;\n");
			Q_strcat(dest, size, "#define gl_FragColor out_Color\n");
		}
	}
	else
	{
		Q_strcat(dest, size, "#version 120\n");
	}

	// HACK: add some macros to avoid extra uniforms and save speed and code maintenance
	//Q_strcat(dest, size,
	//		 va("#ifndef r_SpecularExponent\n#define r_SpecularExponent %f\n#endif\n", r_specularExponent->value));
	//Q_strcat(dest, size,
	//		 va("#ifndef r_SpecularScale\n#define r_SpecularScale %f\n#endif\n", r_specularScale->value));
	//Q_strcat(dest, size,
	//       va("#ifndef r_NormalScale\n#define r_NormalScale %f\n#endif\n", r_normalScale->value));


	Q_strcat(dest, size, "#ifndef M_PI\n#define M_PI 3.14159265358979323846f\n#endif\n");

	//Q_strcat(dest, size, va("#ifndef MAX_SHADOWMAPS\n#define MAX_SHADOWMAPS %i\n#endif\n", MAX_SHADOWMAPS));

	Q_strcat(dest, size,
					 va("#ifndef deformGen_t\n"
						"#define deformGen_t\n"
						"#define DGEN_WAVE_SIN %i\n"
						"#define DGEN_WAVE_SQUARE %i\n"
						"#define DGEN_WAVE_TRIANGLE %i\n"
						"#define DGEN_WAVE_SAWTOOTH %i\n"
						"#define DGEN_WAVE_INVERSE_SAWTOOTH %i\n"
						"#define DGEN_BULGE %i\n"
						"#define DGEN_MOVE %i\n"
						"#endif\n",
						DGEN_WAVE_SIN,
						DGEN_WAVE_SQUARE,
						DGEN_WAVE_TRIANGLE,
						DGEN_WAVE_SAWTOOTH,
						DGEN_WAVE_INVERSE_SAWTOOTH,
						DGEN_BULGE,
						DGEN_MOVE));

	Q_strcat(dest, size,
					 va("#ifndef tcGen_t\n"
						"#define tcGen_t\n"
						"#define TCGEN_LIGHTMAP %i\n"
						"#define TCGEN_TEXTURE %i\n"
						"#define TCGEN_ENVIRONMENT_MAPPED %i\n"
						"#define TCGEN_FOG %i\n"
						"#define TCGEN_VECTOR %i\n"
						"#endif\n",
						TCGEN_LIGHTMAP,
						TCGEN_TEXTURE,
						TCGEN_ENVIRONMENT_MAPPED,
						TCGEN_FOG,
						TCGEN_VECTOR));

	Q_strcat(dest, size,
					 va("#ifndef colorGen_t\n"
						"#define colorGen_t\n"
						"#define CGEN_LIGHTING_DIFFUSE %i\n"
						"#endif\n",
						CGEN_LIGHTING_DIFFUSE));

	Q_strcat(dest, size,
							 va("#ifndef alphaGen_t\n"
								"#define alphaGen_t\n"
								"#define AGEN_LIGHTING_SPECULAR %i\n"
								"#define AGEN_PORTAL %i\n"
								"#define AGEN_FRESNEL %i\n"
								"#endif\n",
								AGEN_LIGHTING_SPECULAR,
								AGEN_PORTAL,
								AGEN_FRESNEL));

	Q_strcat(dest, size,
							 va("#ifndef texenv_t\n"
								"#define texenv_t\n"
								"#define TEXENV_MODULATE %i\n"
								"#define TEXENV_ADD %i\n"
								"#define TEXENV_REPLACE %i\n"
								"#endif\n",
								GL_MODULATE,
								GL_ADD,
								GL_REPLACE));

	fbufWidthScale = 1.0f / ((float)glConfig.vidWidth);
	fbufHeightScale = 1.0f / ((float)glConfig.vidHeight);
	Q_strcat(dest, size,
			 va("#ifndef r_FBufScale\n#define r_FBufScale vec2(%f, %f)\n#endif\n", fbufWidthScale, fbufHeightScale));

	if (extra)
	{
		Q_strcat(dest, size, extra);
	}

	// OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
	// so we have to reset the line counting
	Q_strcat(dest, size, "#line 0\n");
}

static int GLSL_CompileGPUShader(GLhandleARB program, GLhandleARB *prevShader, const GLcharARB *buffer, int size, GLenum shaderType)
{
	GLint           compiled;
	GLhandleARB     shader;

	shader = qglCreateShaderObjectARB(shaderType);

	qglShaderSourceARB(shader, 1, (const GLcharARB **)&buffer, &size);

	// compile shader
	qglCompileShaderARB(shader);

	// check if shader compiled
	qglGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
	if(!compiled)
	{
		GLSL_PrintShaderSource(shader);
		GLSL_PrintInfoLog(shader, qfalse);
		ri.Error(ERR_DROP, "Couldn't compile shader");
		return 0;
	}

	//GLSL_PrintInfoLog(shader, qtrue);
	//GLSL_PrintShaderSource(shader);

	if (*prevShader)
	{
		qglDetachObjectARB(program, *prevShader);
		qglDeleteObjectARB(*prevShader);
	}

	// attach shader to program
	qglAttachObjectARB(program, shader);

	*prevShader = shader;

	return 1;
}


static void GLSL_DumpText(const char *shaderText, int size, const char *name, GLenum shaderType)
{
	int i, l, inc;
	ri.Printf(PRINT_ALL, "static const char *fallback%sShader_%s =\n\"", name, shaderType == GL_VERTEX_SHADER_ARB ? "vp" : "fp");
	l = 0;

	for (i = 0; i < size; i++)
	{
		switch (shaderText[i])
		{
			case '\a':
			case '\b':
			case '\f':
			case '\n':
			case '\r':
			case '\t':
			case '\v':
			case '"':
			case '\\':
				inc = 2;
				break;
			default:
				inc = 1;
				break;
		}

		l += inc;

		if (l >= 76)
		{
			ri.Printf(PRINT_ALL, "\"\n\"");
			l = inc;
		}

		switch (shaderText[i])
		{
			case '\a':
				ri.Printf(PRINT_ALL, "\\a");
				break;
			case '\b':
				ri.Printf(PRINT_ALL, "\\b");
				break;
			case '\f':
				ri.Printf(PRINT_ALL, "\\f");
				break;
			case '\n':
				ri.Printf(PRINT_ALL, "\\n");
				break;
			case '\r':
				ri.Printf(PRINT_ALL, "\\r");
				break;
			case '\t':
				ri.Printf(PRINT_ALL, "\\t");
				break;
			case '\v':
				ri.Printf(PRINT_ALL, "\\v");
				break;
			case '"':
				ri.Printf(PRINT_ALL, "\\\"");
				break;
			case '\\':
				ri.Printf(PRINT_ALL, "\\\\");
				break;
			default:
				ri.Printf(PRINT_ALL, "%c", shaderText[i]);
				break;
		}
	}
	ri.Printf(PRINT_ALL, "\";\n\n");
}

static int GLSL_LoadGPUShaderText(const char *name, const char *fallback,
	GLenum shaderType, char *dest, int destSize, qboolean dump)
{
	char            filename[MAX_QPATH];
	GLcharARB      *buffer = NULL;
	const GLcharARB *shaderText = NULL;
	int             size;
	int             result;

	if(shaderType == GL_VERTEX_SHADER_ARB)
	{
		Com_sprintf(filename, sizeof(filename), "glsl/%s_vp.glsl", name);
	}
	else
	{
		Com_sprintf(filename, sizeof(filename), "glsl/%s_fp.glsl", name);
	}

	ri.Printf(PRINT_DEVELOPER, "...loading '%s'\n", filename);
	size = ri.FS_ReadFile(filename, (void **)&buffer);
	if(!buffer)
	{
		if (fallback)
		{
			ri.Printf(PRINT_DEVELOPER, "couldn't load, using fallback\n");
			shaderText = fallback;
			size = strlen(shaderText);
		}
		else
		{
			ri.Printf(PRINT_DEVELOPER, "couldn't load!\n");
			return 0;
		}
	}
	else
	{
		shaderText = buffer;
	}

	if (dump)
		GLSL_DumpText(shaderText, size, name, shaderType);

	if (size > destSize)
	{
		result = 0;
	}
	else
	{
		Q_strncpyz(dest, shaderText, size + 1);
		result = 1;
	}

	if (buffer)
	{
		ri.FS_FreeFile(buffer);
	}
	
	return result;
}

static void GLSL_LinkProgram(GLhandleARB program)
{
	GLint           linked;

	qglLinkProgramARB(program);

	qglGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &linked);
	if(!linked)
	{
		GLSL_PrintInfoLog(program, qfalse);
		ri.Error(ERR_DROP, "\nshaders failed to link");
	}
}

static void GLSL_ValidateProgram(GLhandleARB program)
{
	GLint           validated;

	qglValidateProgramARB(program);

	qglGetObjectParameterivARB(program, GL_OBJECT_VALIDATE_STATUS_ARB, &validated);
	if(!validated)
	{
		GLSL_PrintInfoLog(program, qfalse);
		ri.Error(ERR_DROP, "\nshaders failed to validate");
	}
}

static void GLSL_ShowProgramUniforms(GLhandleARB program)
{
	int             i, count, size;
	GLenum			type;
	char            uniformName[1000];

	// install the executables in the program object as part of current state.
	qglUseProgramObjectARB(program);

	// check for GL Errors

	// query the number of active uniforms
	qglGetObjectParameterivARB(program, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &count);

	// Loop over each of the active uniforms, and set their value
	for(i = 0; i < count; i++)
	{
		qglGetActiveUniformARB(program, i, sizeof(uniformName), NULL, &size, &type, uniformName);

		ri.Printf(PRINT_DEVELOPER, "active uniform: '%s'\n", uniformName);
	}

	qglUseProgramObjectARB(0);
}

static int GLSL_InitGPUShader2(shaderProgram_t * program, const char *name, int attribs, const char *vpCode, const char *fpCode, int numUniforms)
{
	ri.Printf(PRINT_DEVELOPER, "------- GPU shader -------\n");

	if(strlen(name) >= MAX_QPATH)
	{
		ri.Error(ERR_DROP, "GLSL_InitGPUShader2: \"%s\" is too long\n", name);
	}

	Q_strncpyz(program->name, name, sizeof(program->name));

	program->program = qglCreateProgramObjectARB();
	program->attribs = attribs;

	if (!(GLSL_CompileGPUShader(program->program, &program->vertexShader, vpCode, strlen(vpCode), GL_VERTEX_SHADER_ARB)))
	{
		ri.Printf(PRINT_ALL, "GLSL_InitGPUShader2: Unable to load \"%s\" as GL_VERTEX_SHADER_ARB\n", name);
		qglDeleteObjectARB(program->program);
		return 0;
	}

	if(fpCode)
	{
		if(!(GLSL_CompileGPUShader(program->program, &program->fragmentShader, fpCode, strlen(fpCode), GL_FRAGMENT_SHADER_ARB)))
		{
			ri.Printf(PRINT_ALL, "GLSL_InitGPUShader2: Unable to load \"%s\" as GL_FRAGMENT_SHADER_ARB\n", name);
			qglDeleteObjectARB(program->program);
			return 0;
		}
	}

	if(attribs & ATTR_POSITION)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_POSITION, "attr_Position");

	if(attribs & ATTR_TEXCOORD)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD0, "attr_TexCoord0");

	if(attribs & ATTR_LIGHTCOORD)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD1, "attr_TexCoord1");

//  if(attribs & ATTR_TEXCOORD2)
//      qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD2, "attr_TexCoord2");

//  if(attribs & ATTR_TEXCOORD3)
//      qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD3, "attr_TexCoord3");

#ifdef USE_VERT_TANGENT_SPACE
	if(attribs & ATTR_TANGENT)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TANGENT, "attr_Tangent");

	if(attribs & ATTR_BITANGENT)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_BITANGENT, "attr_Bitangent");
#endif

	if(attribs & ATTR_NORMAL)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_NORMAL, "attr_Normal");

	if(attribs & ATTR_COLOR)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_COLOR, "attr_Color");

	if(attribs & ATTR_PAINTCOLOR)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_PAINTCOLOR, "attr_PaintColor");

	if(attribs & ATTR_LIGHTDIRECTION)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_LIGHTDIRECTION, "attr_LightDirection");

	if(attribs & ATTR_POSITION2)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_POSITION2, "attr_Position2");

	if(attribs & ATTR_NORMAL2)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_NORMAL2, "attr_Normal2");

#ifdef USE_VERT_TANGENT_SPACE
	if(attribs & ATTR_TANGENT2)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TANGENT2, "attr_Tangent2");

	if(attribs & ATTR_BITANGENT2)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_BITANGENT2, "attr_Bitangent2");
#endif

	GLSL_LinkProgram(program->program);

	program->numUniforms = numUniforms;

	{
		int i, size;

		size = sizeof(*program->uniforms) * numUniforms;
		program->uniforms = ri.Malloc(size);
		for (i = 0; i < numUniforms; i++)
		{
			program->uniforms[i] = -1;
		}

		size = sizeof(*program->uniformTypes) * numUniforms;
		program->uniformTypes = ri.Malloc(size);
		memset(program->uniformTypes, 0, size);

		size = sizeof(*program->uniformBufferOffsets) * numUniforms;
		program->uniformBufferOffsets = ri.Malloc(size);
		memset(program->uniformBufferOffsets, 0, size);
	}

	return 1;
}

static int GLSL_InitGPUShader(shaderProgram_t * program, const char *name,
	int attribs, qboolean fragmentShader, const GLcharARB *extra, qboolean addHeader,
	const char *fallback_vp, const char *fallback_fp, int numUniforms)
{
	char vpCode[32000];
	char fpCode[32000];
	char *postHeader;
	int size;
	int result;

	size = sizeof(vpCode);
	if (addHeader)
	{
		GLSL_GetShaderHeader(GL_VERTEX_SHADER_ARB, extra, vpCode, size);
		postHeader = &vpCode[strlen(vpCode)];
		size -= strlen(vpCode);
	}
	else
	{
		postHeader = &vpCode[0];
	}

	if (!GLSL_LoadGPUShaderText(name, fallback_vp, GL_VERTEX_SHADER_ARB, postHeader, size, qfalse))
	{
		return 0;
	}

	if (fragmentShader)
	{
		size = sizeof(fpCode);
		if (addHeader)
		{
			GLSL_GetShaderHeader(GL_FRAGMENT_SHADER_ARB, extra, fpCode, size);
			postHeader = &fpCode[strlen(fpCode)];
			size -= strlen(fpCode);
		}
		else
		{
			postHeader = &fpCode[0];
		}

		if (!GLSL_LoadGPUShaderText(name, fallback_fp, GL_FRAGMENT_SHADER_ARB, postHeader, size, qfalse))
		{
			return 0;
		}
	}

	result = GLSL_InitGPUShader2(program, name, attribs, vpCode, fragmentShader ? fpCode : NULL, numUniforms);

	return result;
}

// intentionally deceiving the user here, not actually setting the names but getting their indexes.
void GLSL_AddUniform(shaderProgram_t *program, int uniformNum, const char *name, int type)
{
	GLint *uniforms = program->uniforms;

	uniforms[uniformNum] = qglGetUniformLocationARB(program->program, name);
	program->uniformTypes[uniformNum] = type;
}

void GLSL_EndUniforms(shaderProgram_t *program)
{
	if (program->numUniforms)
	{
		int i, size;
		 
		size = 0;
		for (i = 0; i < program->numUniforms; i++)
		{
			if (program->uniforms[i] != -1)
			{
				program->uniformBufferOffsets[i] = size;

				switch(program->uniformTypes[i])
				{
					case GLSL_INT:
						size += sizeof(GLint);
						break;
					case GLSL_FLOAT:
						size += sizeof(GLfloat);
						break;
					case GLSL_FLOAT5:
						size += sizeof(vec_t) * 5;
						break;
					case GLSL_VEC2:
						size += sizeof(vec_t) * 2;
						break;
					case GLSL_VEC3:
						size += sizeof(vec_t) * 3;
						break;
					case GLSL_VEC4:
						size += sizeof(vec_t) * 4;
						break;
					case GLSL_MAT16:
						size += sizeof(vec_t) * 16;
						break;
					default:
						break;
				}
			}
		}

		program->uniformBuffer = ri.Malloc(size);

	}
}

void GLSL_FinishGPUShader(shaderProgram_t *program)
{
	GLSL_ValidateProgram(program->program);
	GLSL_ShowProgramUniforms(program->program);
	GL_CheckErrors();
}

void GLSL_SetUniformInt(shaderProgram_t *program, int uniformNum, GLint value)
{
	GLint *uniforms = program->uniforms;
	GLint *compare = (GLint *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (program->uniformTypes[uniformNum] != GLSL_INT)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformInt: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;

	qglUniform1iARB(uniforms[uniformNum], value);
}

void GLSL_SetUniformFloat(shaderProgram_t *program, int uniformNum, GLfloat value)
{
	GLint *uniforms = program->uniforms;
	GLfloat *compare = (GLfloat *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (program->uniformTypes[uniformNum] != GLSL_FLOAT)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformFloat: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;
	
	qglUniform1fARB(uniforms[uniformNum], value);
}

void GLSL_SetUniformVec2(shaderProgram_t *program, int uniformNum, const vec2_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (program->uniformTypes[uniformNum] != GLSL_VEC2)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformVec2: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (v[0] == compare[0] && v[1] == compare[1])
	{
		return;
	}

	compare[0] = v[0];
	compare[1] = v[1];

	qglUniform2fARB(uniforms[uniformNum], v[0], v[1]);
}

void GLSL_SetUniformVec3(shaderProgram_t *program, int uniformNum, const vec3_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (program->uniformTypes[uniformNum] != GLSL_VEC3)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformVec3: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (VectorCompare(v, compare))
	{
		return;
	}

	VectorCopy(v, compare);

	qglUniform3fARB(uniforms[uniformNum], v[0], v[1], v[2]);
}

void GLSL_SetUniformVec4(shaderProgram_t *program, int uniformNum, const vec4_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (program->uniformTypes[uniformNum] != GLSL_VEC4)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformVec4: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (VectorCompare4(v, compare))
	{
		return;
	}

	VectorCopy4(v, compare);

	qglUniform4fARB(uniforms[uniformNum], v[0], v[1], v[2], v[3]);
}

void GLSL_SetUniformFloat5(shaderProgram_t *program, int uniformNum, const vec5_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
	{
		ri.Printf( PRINT_ALL, "well shit.\n");
		return;
	}

	if (program->uniformTypes[uniformNum] != GLSL_FLOAT5)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformFloat5: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (VectorCompare5(v, compare))
	{
		return;
	}

	VectorCopy5(v, compare);

	qglUniform1fvARB(uniforms[uniformNum], 5, v);
}

void GLSL_SetUniformMatrix16(shaderProgram_t *program, int uniformNum, const matrix_t matrix)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (program->uniformTypes[uniformNum] != GLSL_MAT16)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformMatrix16: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (Matrix16Compare(matrix, compare))
	{
		return;
	}

	Matrix16Copy(matrix, compare);

	qglUniformMatrix4fvARB(uniforms[uniformNum], 1, GL_FALSE, matrix);
}

void GLSL_DeleteGPUShader(shaderProgram_t *program)
{
	if(program->program)
	{
		if (program->vertexShader)
		{
			qglDetachObjectARB(program->program, program->vertexShader);
			qglDeleteObjectARB(program->vertexShader);
		}

		if (program->fragmentShader)
		{
			qglDetachObjectARB(program->program, program->fragmentShader);
			qglDeleteObjectARB(program->fragmentShader);
		}

		qglDeleteObjectARB(program->program);

		if (program->uniforms)
		{
			ri.Free(program->uniforms);
		}

		if (program->uniformTypes)
		{
			ri.Free(program->uniformTypes);
		}
				
		if (program->uniformBuffer)
		{
			ri.Free(program->uniformBuffer);
		}

		if (program->uniformBufferOffsets)
		{
			ri.Free(program->uniformBufferOffsets);
		}

		Com_Memset(program, 0, sizeof(*program));
	}
}

void GLSL_InitGPUShaders(void)
{
	int             startTime, endTime;
	int i;
	char extradefines[1024];
	int attribs;
	int numGenShaders = 0, numLightShaders = 0, numEtcShaders = 0;

	ri.Printf(PRINT_ALL, "------- GLSL_InitGPUShaders -------\n");

	// make sure the render thread is stopped
	R_SyncRenderThread();

	startTime = ri.Milliseconds();

	for (i = 0; i < GENERICDEF_COUNT; i++)
	{	
		attribs = ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_NORMAL | ATTR_COLOR;
		extradefines[0] = '\0';

		if (i & GENERICDEF_USE_DEFORM_VERTEXES)
			Q_strcat(extradefines, 1024, "#define USE_DEFORM_VERTEXES\n");

		if (i & GENERICDEF_USE_TCGEN)
			Q_strcat(extradefines, 1024, "#define USE_TCGEN\n");

		if (i & GENERICDEF_USE_VERTEX_ANIMATION)
		{
			Q_strcat(extradefines, 1024, "#define USE_VERTEX_ANIMATION\n");
			attribs |= ATTR_POSITION2 | ATTR_NORMAL2;
		}

		if (i & GENERICDEF_USE_FOG)
			Q_strcat(extradefines, 1024, "#define USE_FOG\n");

		if (i & GENERICDEF_USE_RGBAGEN)
			Q_strcat(extradefines, 1024, "#define USE_RGBAGEN\n");

		if (i & GENERICDEF_USE_LIGHTMAP)
			Q_strcat(extradefines, 1024, "#define USE_LIGHTMAP\n");

		if (r_hdr->integer && !(glRefConfig.textureFloat && glRefConfig.halfFloatPixel))
			Q_strcat(extradefines, 1024, "#define RGBE_LIGHTMAP\n");

		if (!GLSL_InitGPUShader(&tr.genericShader[i], "generic", attribs, qtrue, extradefines, qtrue, fallbackGenericShader_vp, fallbackGenericShader_fp, GENERIC_UNIFORM_COUNT))
		{
			ri.Error(ERR_FATAL, "Could not load generic shader!\n");
		}

		// There's actually no need to filter these out, since they'll
		// redirect to -1 if nonexistent, but it's more understandable this way.
		
		GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_MODELVIEWPROJECTIONMATRIX, "u_ModelViewProjectionMatrix", GLSL_MAT16);

		GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_BASECOLOR, "u_BaseColor", GLSL_VEC4);
		GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_VERTCOLOR, "u_VertColor", GLSL_VEC4);

		if (i & GENERICDEF_USE_RGBAGEN)
		{
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_COLORGEN,      "u_ColorGen",      GLSL_INT);
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_ALPHAGEN,      "u_AlphaGen",      GLSL_INT);
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_AMBIENTLIGHT,  "u_AmbientLight",  GLSL_VEC3);
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_DIRECTEDLIGHT, "u_DirectedLight", GLSL_VEC3);
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_LIGHTORIGIN,   "u_LightOrigin",   GLSL_VEC4);
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_PORTALRANGE,   "u_PortalRange",   GLSL_FLOAT);
		}

		if (i & GENERICDEF_USE_TCGEN)
		{
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_TCGEN0,        "u_TCGen0",        GLSL_INT);
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_TCGEN0VECTOR0, "u_TCGen0Vector0", GLSL_VEC3);
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_TCGEN0VECTOR1, "u_TCGen0Vector1", GLSL_VEC3);
		}

		if (i & GENERICDEF_USE_FOG)
		{
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_FOGCOLORMASK, "u_FogColorMask", GLSL_VEC4);
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_FOGDISTANCE,  "u_FogDistance",  GLSL_VEC4);
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_FOGDEPTH,     "u_FogDepth",     GLSL_VEC4);
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_FOGEYET,      "u_FogEyeT",      GLSL_FLOAT);
		}

		if (i & GENERICDEF_USE_DEFORM_VERTEXES)
		{
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_DEFORMGEN,    "u_DeformGen",    GLSL_INT);
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_DEFORMPARAMS, "u_DeformParams", GLSL_FLOAT5);
		}

		GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_TIME,             "u_Time",             GLSL_FLOAT);
		GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_VIEWORIGIN,       "u_ViewOrigin",       GLSL_VEC3);
		GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_DIFFUSETEXMATRIX, "u_DiffuseTexMatrix", GLSL_MAT16);
		GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_TEXTURE1ENV,      "u_Texture1Env",      GLSL_INT);

		GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_DIFFUSEMAP,       "u_DiffuseMap",       GLSL_INT);
		GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_LIGHTMAP,         "u_LightMap",         GLSL_INT);


		if (i & GENERICDEF_USE_VERTEX_ANIMATION)
		{
			GLSL_AddUniform(&tr.genericShader[i], GENERIC_UNIFORM_VERTEXLERP, "u_VertexLerp", GLSL_FLOAT);
		}

		GLSL_EndUniforms(&tr.genericShader[i]);

		qglUseProgramObjectARB(tr.genericShader[i].program);
		GLSL_SetUniformInt(&tr.genericShader[i], GENERIC_UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.genericShader[i], GENERIC_UNIFORM_LIGHTMAP,   TB_LIGHTMAP);
		qglUseProgramObjectARB(0);

		GLSL_FinishGPUShader(&tr.genericShader[i]);

		numGenShaders++;
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD;

	if (!GLSL_InitGPUShader(&tr.textureColorShader, "texturecolor", attribs, qtrue, NULL, qfalse, fallbackTextureColorShader_vp, fallbackTextureColorShader_fp, TEXTURECOLOR_UNIFORM_COUNT))
	{
		ri.Error(ERR_FATAL, "Could not load texturecolor shader!\n");
	}
	
	GLSL_AddUniform(&tr.textureColorShader, TEXTURECOLOR_UNIFORM_MODELVIEWPROJECTIONMATRIX, "u_ModelViewProjectionMatrix", GLSL_MAT16);
	GLSL_AddUniform(&tr.textureColorShader, TEXTURECOLOR_UNIFORM_COLOR,                     "u_Color",                     GLSL_VEC4);
	GLSL_AddUniform(&tr.textureColorShader, TEXTURECOLOR_UNIFORM_TEXTUREMAP,                "u_DiffuseMap",                GLSL_INT);

	GLSL_EndUniforms(&tr.textureColorShader);

	qglUseProgramObjectARB(tr.textureColorShader.program);
	GLSL_SetUniformInt(&tr.textureColorShader, TEXTURECOLOR_UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);
	qglUseProgramObjectARB(0);

	GLSL_FinishGPUShader(&tr.textureColorShader);

	numEtcShaders++;


	attribs = ATTR_POSITION | ATTR_POSITION2 | ATTR_NORMAL | ATTR_NORMAL2 | ATTR_TEXCOORD;

	if (!GLSL_InitGPUShader(&tr.fogShader, "fogpass", attribs, qtrue, NULL, qtrue, fallbackFogPassShader_vp, fallbackFogPassShader_fp, FOGPASS_UNIFORM_COUNT))
	{
		ri.Error(ERR_FATAL, "Could not load fogpass shader!\n");
	}

	GLSL_AddUniform(&tr.fogShader, FOGPASS_UNIFORM_FOGDISTANCE,               "u_FogDistance",               GLSL_VEC4);
	GLSL_AddUniform(&tr.fogShader, FOGPASS_UNIFORM_FOGDEPTH,                  "u_FogDepth",                  GLSL_VEC4);
	GLSL_AddUniform(&tr.fogShader, FOGPASS_UNIFORM_FOGEYET,                   "u_FogEyeT",                   GLSL_FLOAT);
	GLSL_AddUniform(&tr.fogShader, FOGPASS_UNIFORM_DEFORMGEN,                 "u_DeformGen",                 GLSL_INT);
	GLSL_AddUniform(&tr.fogShader, FOGPASS_UNIFORM_DEFORMPARAMS,              "u_DeformParams",              GLSL_FLOAT5);
	GLSL_AddUniform(&tr.fogShader, FOGPASS_UNIFORM_TIME,                      "u_Time",                      GLSL_FLOAT);
	GLSL_AddUniform(&tr.fogShader, FOGPASS_UNIFORM_COLOR,                     "u_Color",                     GLSL_VEC4);
	GLSL_AddUniform(&tr.fogShader, FOGPASS_UNIFORM_MODELVIEWPROJECTIONMATRIX, "u_ModelViewProjectionMatrix", GLSL_MAT16);
	GLSL_AddUniform(&tr.fogShader, FOGPASS_UNIFORM_VERTEXLERP,                "u_VertexLerp",                GLSL_FLOAT);
	
	GLSL_EndUniforms(&tr.fogShader);
	GLSL_FinishGPUShader(&tr.fogShader);

	numEtcShaders++;


	attribs = ATTR_POSITION | ATTR_NORMAL | ATTR_TEXCOORD;

	if (!GLSL_InitGPUShader(&tr.dlightallShader, "dlight", attribs, qtrue, NULL, qtrue, fallbackDlightShader_vp, fallbackDlightShader_fp, DLIGHT_UNIFORM_COUNT))
	{
		ri.Error(ERR_FATAL, "Could not load dlight shader!\n");
	}

	GLSL_AddUniform(&tr.dlightallShader, DLIGHT_UNIFORM_DLIGHTINFO,                "u_DlightInfo",                GLSL_VEC4);
	GLSL_AddUniform(&tr.dlightallShader, DLIGHT_UNIFORM_DEFORMGEN,                 "u_DeformGen",                 GLSL_INT);
	GLSL_AddUniform(&tr.dlightallShader, DLIGHT_UNIFORM_DEFORMPARAMS,              "u_DeformParams",              GLSL_FLOAT5);
	GLSL_AddUniform(&tr.dlightallShader, DLIGHT_UNIFORM_TIME,                      "u_Time",                      GLSL_FLOAT);
	GLSL_AddUniform(&tr.dlightallShader, DLIGHT_UNIFORM_COLOR,                     "u_Color",                     GLSL_VEC4);
	GLSL_AddUniform(&tr.dlightallShader, DLIGHT_UNIFORM_MODELVIEWPROJECTIONMATRIX, "u_ModelViewProjectionMatrix", GLSL_MAT16);
	
	GLSL_EndUniforms(&tr.dlightallShader);
	
	qglUseProgramObjectARB(tr.dlightallShader.program);
	GLSL_SetUniformInt(&tr.dlightallShader, DLIGHT_UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	qglUseProgramObjectARB(0);

	GLSL_FinishGPUShader(&tr.dlightallShader);

	numEtcShaders++;


	for (i = 0; i < LIGHTDEF_COUNT; i++)
	{
		// skip impossible combos
		if ((i & LIGHTDEF_USE_NORMALMAP) && !r_normalMapping->integer)
			continue;

		if ((i & LIGHTDEF_USE_PARALLAXMAP) && !r_parallaxMapping->integer)
			continue;

		if ((i & LIGHTDEF_USE_SPECULARMAP) && !r_specularMapping->integer)
			continue;

		if ((i & LIGHTDEF_USE_DELUXEMAP) && !r_deluxeMapping->integer)
			continue;

		if (!((i & LIGHTDEF_LIGHTTYPE_MASK) == LIGHTDEF_USE_LIGHTMAP) && (i & LIGHTDEF_USE_DELUXEMAP))
			continue;

		if (!(i & LIGHTDEF_USE_NORMALMAP) && (i & LIGHTDEF_USE_PARALLAXMAP))
			continue;

		if (!((i & LIGHTDEF_LIGHTTYPE_MASK) == LIGHTDEF_USE_LIGHT_VECTOR))
		{
			if (i & LIGHTDEF_USE_SHADOWMAP)
				continue;
		}

		attribs = ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR | ATTR_NORMAL;

		extradefines[0] = '\0';

		if (r_normalAmbient->value > 0.003f)
			Q_strcat(extradefines, 1024, va("#define r_normalAmbient %f\n", r_normalAmbient->value));

		if (r_dlightMode->integer >= 2)
			Q_strcat(extradefines, 1024, "#define USE_SHADOWMAP\n");

		if (1)
		{
			Q_strcat(extradefines, 1024, "#define SWIZZLE_NORMALMAP\n");
		}

		if (r_hdr->integer && !(glRefConfig.textureFloat && glRefConfig.halfFloatPixel))
			Q_strcat(extradefines, 1024, "#define RGBE_LIGHTMAP\n");

		if (i & LIGHTDEF_LIGHTTYPE_MASK)
		{
			Q_strcat(extradefines, 1024, "#define USE_LIGHT\n");

			if (r_normalMapping->integer == 0 && r_specularMapping->integer == 0)
				Q_strcat(extradefines, 1024, "#define USE_FAST_LIGHT\n");

			switch (i & LIGHTDEF_LIGHTTYPE_MASK)
			{
				case LIGHTDEF_USE_LIGHTMAP:
					Q_strcat(extradefines, 1024, "#define USE_LIGHTMAP\n");
					attribs |= ATTR_LIGHTCOORD | ATTR_LIGHTDIRECTION;
					break;
				case LIGHTDEF_USE_LIGHT_VECTOR:
					Q_strcat(extradefines, 1024, "#define USE_LIGHT_VECTOR\n");
					break;
				case LIGHTDEF_USE_LIGHT_VERTEX:
					Q_strcat(extradefines, 1024, "#define USE_LIGHT_VERTEX\n");
					attribs |= ATTR_LIGHTDIRECTION;
					break;
				default:
					break;
			}
		}

		if ((i & LIGHTDEF_USE_NORMALMAP) && r_normalMapping->integer)
		{
			Q_strcat(extradefines, 1024, "#define USE_NORMALMAP\n");

			if (r_normalMapping->integer == 2)
				Q_strcat(extradefines, 1024, "#define USE_OREN_NAYAR\n");

			if (r_normalMapping->integer == 3)
				Q_strcat(extradefines, 1024, "#define USE_TRIACE_OREN_NAYAR\n");

#ifdef USE_VERT_TANGENT_SPACE
			Q_strcat(extradefines, 1024, "#define USE_VERT_TANGENT_SPACE\n");
			attribs |= ATTR_TANGENT | ATTR_BITANGENT;
#endif
		}

		if ((i & LIGHTDEF_USE_SPECULARMAP) && r_specularMapping->integer)
		{
			Q_strcat(extradefines, 1024, "#define USE_SPECULARMAP\n");

			switch (r_specularMapping->integer)
			{
				case 1:
				default:
					Q_strcat(extradefines, 1024, "#define USE_TRIACE\n");
					break;

				case 2:
					Q_strcat(extradefines, 1024, "#define USE_BLINN\n");
					break;

				case 3:
					Q_strcat(extradefines, 1024, "#define USE_COOK_TORRANCE\n");
					break;

				case 4:
					Q_strcat(extradefines, 1024, "#define USE_TORRANCE_SPARROW\n");
					break;
			}
		}

		if ((i & LIGHTDEF_USE_DELUXEMAP) && r_deluxeMapping->integer)
			Q_strcat(extradefines, 1024, "#define USE_DELUXEMAP\n");

		if ((i & LIGHTDEF_USE_PARALLAXMAP) && !(i & LIGHTDEF_ENTITY) && r_parallaxMapping->integer)
			Q_strcat(extradefines, 1024, "#define USE_PARALLAXMAP\n");

		if (i & LIGHTDEF_USE_SHADOWMAP)
			Q_strcat(extradefines, 1024, "#define USE_SHADOWMAP\n");

		if (i & LIGHTDEF_TCGEN_ENVIRONMENT)
			Q_strcat(extradefines, 1024, "#define TCGEN_ENVIRONMENT\n");

		if (i & LIGHTDEF_ENTITY)
		{
			Q_strcat(extradefines, 1024, "#define USE_VERTEX_ANIMATION\n#define USE_MODELMATRIX\n");
			attribs |= ATTR_POSITION2 | ATTR_NORMAL2;

#ifdef USE_VERT_TANGENT_SPACE
			if (i & LIGHTDEF_USE_NORMALMAP && r_normalMapping->integer)
			{
				attribs |= ATTR_TANGENT2 | ATTR_BITANGENT2;
			}
#endif
		}

		if (!GLSL_InitGPUShader(&tr.lightallShader[i], "lightall", attribs, qtrue, extradefines, qtrue, fallbackLightallShader_vp, fallbackLightallShader_fp, GENERIC_UNIFORM_COUNT))
		{
			ri.Error(ERR_FATAL, "Could not load lightall shader!\n");
		}

		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_MODELVIEWPROJECTIONMATRIX, "u_ModelViewProjectionMatrix", GLSL_MAT16);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_MODELMATRIX,               "u_ModelMatrix",               GLSL_MAT16);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_DIFFUSETEXMATRIX,          "u_DiffuseTexMatrix",          GLSL_MAT16);
		//GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_NORMALTEXMATRIX,           "u_NormalTexMatrix",           GLSL_MAT16);
		//GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_SPECULARTEXMATRIX,         "u_SpecularTexMatrix",         GLSL_MAT16);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_VIEWORIGIN,                "u_ViewOrigin",                GLSL_VEC3);

		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_DIFFUSEMAP,                "u_DiffuseMap",                GLSL_INT);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_LIGHTMAP,                  "u_LightMap",                  GLSL_INT);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_NORMALMAP,                 "u_NormalMap",                 GLSL_INT);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_DELUXEMAP,                 "u_DeluxeMap",                 GLSL_INT);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_SPECULARMAP,               "u_SpecularMap",               GLSL_INT);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_SHADOWMAP,                 "u_ShadowMap",                 GLSL_INT);

		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_AMBIENTLIGHT,              "u_AmbientLight",              GLSL_VEC3);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_DIRECTEDLIGHT,             "u_DirectedLight",             GLSL_VEC3);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_LIGHTORIGIN,               "u_LightOrigin",               GLSL_VEC4);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_LIGHTRADIUS,               "u_LightRadius",               GLSL_FLOAT);

		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_MATERIALINFO,              "u_MaterialInfo",              GLSL_VEC2);

		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_BASECOLOR,                 "u_BaseColor",                 GLSL_VEC4);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_VERTCOLOR,                 "u_VertColor",                 GLSL_VEC4);
		GLSL_AddUniform(&tr.lightallShader[i], GENERIC_UNIFORM_VERTEXLERP,                "u_VertexLerp",                GLSL_FLOAT);
		
		GLSL_EndUniforms(&tr.lightallShader[i]);

		qglUseProgramObjectARB(tr.lightallShader[i].program);
		GLSL_SetUniformInt(&tr.lightallShader[i], GENERIC_UNIFORM_DIFFUSEMAP,  TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.lightallShader[i], GENERIC_UNIFORM_LIGHTMAP,    TB_LIGHTMAP);
		GLSL_SetUniformInt(&tr.lightallShader[i], GENERIC_UNIFORM_NORMALMAP,   TB_NORMALMAP);
		GLSL_SetUniformInt(&tr.lightallShader[i], GENERIC_UNIFORM_DELUXEMAP,   TB_DELUXEMAP);
		GLSL_SetUniformInt(&tr.lightallShader[i], GENERIC_UNIFORM_SPECULARMAP, TB_SPECULARMAP);
		GLSL_SetUniformInt(&tr.lightallShader[i], GENERIC_UNIFORM_SHADOWMAP,   TB_SHADOWMAP);
		qglUseProgramObjectARB(0);

		GLSL_FinishGPUShader(&tr.lightallShader[i]);

		numLightShaders++;
	}
	
	attribs = ATTR_POSITION | ATTR_POSITION2 | ATTR_NORMAL | ATTR_NORMAL2 | ATTR_TEXCOORD;

	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.shadowmapShader, "shadowfill", attribs, qtrue, extradefines, qtrue, fallbackShadowfillShader_vp, fallbackShadowfillShader_fp, GENERIC_UNIFORM_COUNT))
	{
		ri.Error(ERR_FATAL, "Could not load shadowfill shader!\n");
	}

	GLSL_AddUniform(&tr.shadowmapShader, GENERIC_UNIFORM_DEFORMGEN,                 "u_DeformGen",                 GLSL_INT);
	GLSL_AddUniform(&tr.shadowmapShader, GENERIC_UNIFORM_DEFORMPARAMS,              "u_DeformParams",              GLSL_FLOAT5);
	GLSL_AddUniform(&tr.shadowmapShader, GENERIC_UNIFORM_TIME,                      "u_Time",                      GLSL_FLOAT);
	GLSL_AddUniform(&tr.shadowmapShader, GENERIC_UNIFORM_MODELVIEWPROJECTIONMATRIX, "u_ModelViewProjectionMatrix", GLSL_MAT16);
	GLSL_AddUniform(&tr.shadowmapShader, GENERIC_UNIFORM_MODELMATRIX,               "u_ModelMatrix",               GLSL_MAT16);
	GLSL_AddUniform(&tr.shadowmapShader, GENERIC_UNIFORM_VERTEXLERP,                "u_VertexLerp",                GLSL_FLOAT);
	
	GLSL_AddUniform(&tr.shadowmapShader, GENERIC_UNIFORM_LIGHTORIGIN,               "u_LightOrigin",               GLSL_VEC4);
	GLSL_AddUniform(&tr.shadowmapShader, GENERIC_UNIFORM_LIGHTRADIUS,               "u_LightRadius",               GLSL_FLOAT);

	GLSL_EndUniforms(&tr.shadowmapShader);
	GLSL_FinishGPUShader(&tr.shadowmapShader);

	numEtcShaders++;

	attribs = ATTR_POSITION | ATTR_NORMAL;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define USE_PCF\n#define USE_DISCARD\n");

	if (!GLSL_InitGPUShader(&tr.pshadowShader, "pshadow", attribs, qtrue, extradefines, qtrue, fallbackPshadowShader_vp, fallbackPshadowShader_fp, PSHADOW_UNIFORM_COUNT))
	{
		ri.Error(ERR_FATAL, "Could not load pshadow shader!\n");
	}
	
	GLSL_AddUniform(&tr.pshadowShader, PSHADOW_UNIFORM_MODELVIEWPROJECTIONMATRIX, "u_ModelViewProjectionMatrix", GLSL_MAT16);
	GLSL_AddUniform(&tr.pshadowShader, PSHADOW_UNIFORM_LIGHTFORWARD,              "u_LightForward",              GLSL_VEC3);
	GLSL_AddUniform(&tr.pshadowShader, PSHADOW_UNIFORM_LIGHTUP,                   "u_LightUp",                   GLSL_VEC3);
	GLSL_AddUniform(&tr.pshadowShader, PSHADOW_UNIFORM_LIGHTRIGHT,                "u_LightRight",                GLSL_VEC3);
	GLSL_AddUniform(&tr.pshadowShader, PSHADOW_UNIFORM_LIGHTORIGIN,               "u_LightOrigin",               GLSL_VEC4);
	GLSL_AddUniform(&tr.pshadowShader, PSHADOW_UNIFORM_LIGHTRADIUS,               "u_LightRadius",               GLSL_FLOAT);

	GLSL_EndUniforms(&tr.pshadowShader);

	qglUseProgramObjectARB(tr.pshadowShader.program);
	GLSL_SetUniformInt(&tr.pshadowShader, PSHADOW_UNIFORM_SHADOWMAP, TB_DIFFUSEMAP);
	qglUseProgramObjectARB(0);

	GLSL_FinishGPUShader(&tr.pshadowShader);

	numEtcShaders++;


	attribs = ATTR_POSITION | ATTR_TEXCOORD;
	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.down4xShader, "down4x", attribs, qtrue, extradefines, qtrue, fallbackDown4xShader_vp, fallbackDown4xShader_fp, TEXTURECOLOR_UNIFORM_COUNT))
	{
		ri.Error(ERR_FATAL, "Could not load down4x shader!\n");
	}
	
	GLSL_AddUniform(&tr.down4xShader, TEXTURECOLOR_UNIFORM_MODELVIEWPROJECTIONMATRIX, "u_ModelViewProjectionMatrix", GLSL_MAT16);
	GLSL_AddUniform(&tr.down4xShader, TEXTURECOLOR_UNIFORM_INVTEXRES,                 "u_InvTexRes",                 GLSL_VEC2);

	GLSL_AddUniform(&tr.down4xShader, TEXTURECOLOR_UNIFORM_TEXTUREMAP,                "u_TextureMap",                GLSL_INT);

	GLSL_EndUniforms(&tr.down4xShader);

	qglUseProgramObjectARB(tr.down4xShader.program);
	GLSL_SetUniformInt(&tr.down4xShader, TEXTURECOLOR_UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);
	qglUseProgramObjectARB(0);

	GLSL_FinishGPUShader(&tr.down4xShader);

	numEtcShaders++;


	attribs = ATTR_POSITION | ATTR_TEXCOORD;
	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.bokehShader, "bokeh", attribs, qtrue, extradefines, qtrue, fallbackBokehShader_vp, fallbackBokehShader_fp, TEXTURECOLOR_UNIFORM_COUNT))
	{
		ri.Error(ERR_FATAL, "Could not load bokeh shader!\n");
	}
	
	GLSL_AddUniform(&tr.bokehShader, TEXTURECOLOR_UNIFORM_MODELVIEWPROJECTIONMATRIX, "u_ModelViewProjectionMatrix", GLSL_MAT16);
	GLSL_AddUniform(&tr.bokehShader, TEXTURECOLOR_UNIFORM_INVTEXRES,                 "u_InvTexRes",                 GLSL_VEC2);
	GLSL_AddUniform(&tr.bokehShader, TEXTURECOLOR_UNIFORM_COLOR,                     "u_Color",                     GLSL_VEC4);

	GLSL_AddUniform(&tr.bokehShader, TEXTURECOLOR_UNIFORM_TEXTUREMAP,                "u_TextureMap",                GLSL_INT);

	GLSL_EndUniforms(&tr.bokehShader);

	qglUseProgramObjectARB(tr.bokehShader.program);
	GLSL_SetUniformInt(&tr.bokehShader, TEXTURECOLOR_UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);
	qglUseProgramObjectARB(0);

	GLSL_FinishGPUShader(&tr.bokehShader);

	numEtcShaders++;


	attribs = ATTR_POSITION | ATTR_TEXCOORD;
	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.tonemapShader, "tonemap", attribs, qtrue, extradefines, qtrue, fallbackToneMapShader_vp, fallbackToneMapShader_fp, TEXTURECOLOR_UNIFORM_COUNT))
	{
		ri.Error(ERR_FATAL, "Could not load tonemap shader!\n");
	}
	
	GLSL_AddUniform(&tr.tonemapShader, TEXTURECOLOR_UNIFORM_MODELVIEWPROJECTIONMATRIX, "u_ModelViewProjectionMatrix", GLSL_MAT16);
	GLSL_AddUniform(&tr.tonemapShader, TEXTURECOLOR_UNIFORM_INVTEXRES,                 "u_InvTexRes",                 GLSL_VEC2);
	GLSL_AddUniform(&tr.tonemapShader, TEXTURECOLOR_UNIFORM_COLOR,                     "u_Color",                     GLSL_VEC4);
	GLSL_AddUniform(&tr.tonemapShader, TEXTURECOLOR_UNIFORM_AUTOEXPOSUREMINMAX,        "u_AutoExposureMinMax",        GLSL_VEC2);
	GLSL_AddUniform(&tr.tonemapShader, TEXTURECOLOR_UNIFORM_TONEMINAVGMAXLINEAR,       "u_ToneMinAvgMaxLinear",       GLSL_VEC3);
	GLSL_AddUniform(&tr.tonemapShader, TEXTURECOLOR_UNIFORM_TEXTUREMAP,                "u_TextureMap",                GLSL_INT);
	GLSL_AddUniform(&tr.tonemapShader, TEXTURECOLOR_UNIFORM_LEVELSMAP,                 "u_LevelsMap",                 GLSL_INT);

	GLSL_EndUniforms(&tr.tonemapShader);

	qglUseProgramObjectARB(tr.tonemapShader.program);
	GLSL_SetUniformInt(&tr.tonemapShader, TEXTURECOLOR_UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.tonemapShader, TEXTURECOLOR_UNIFORM_LEVELSMAP,  TB_LEVELSMAP);
	qglUseProgramObjectARB(0);

	GLSL_FinishGPUShader(&tr.tonemapShader);

	numEtcShaders++;


	for (i = 0; i < 2; i++)
	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD;
		extradefines[0] = '\0';

		if (!i)
			Q_strcat(extradefines, 1024, "#define FIRST_PASS\n");

		if (!GLSL_InitGPUShader(&tr.calclevels4xShader[i], "calclevels4x", attribs, qtrue, extradefines, qtrue, fallbackCalcLevels4xShader_vp, fallbackCalcLevels4xShader_fp, TEXTURECOLOR_UNIFORM_COUNT))
		{
			ri.Error(ERR_FATAL, "Could not load calclevels4x shader!\n");
		}

		GLSL_AddUniform(&tr.calclevels4xShader[i], TEXTURECOLOR_UNIFORM_MODELVIEWPROJECTIONMATRIX, "u_ModelViewProjectionMatrix", GLSL_MAT16);
		GLSL_AddUniform(&tr.calclevels4xShader[i], TEXTURECOLOR_UNIFORM_INVTEXRES,                 "u_InvTexRes",                 GLSL_VEC2);
		GLSL_AddUniform(&tr.calclevels4xShader[i], TEXTURECOLOR_UNIFORM_COLOR,                     "u_Color",                     GLSL_VEC4);

		GLSL_AddUniform(&tr.calclevels4xShader[i], TEXTURECOLOR_UNIFORM_TEXTUREMAP,                "u_TextureMap",                GLSL_INT);

		GLSL_EndUniforms(&tr.calclevels4xShader[i]);

		qglUseProgramObjectARB(tr.calclevels4xShader[i].program);
		GLSL_SetUniformInt(&tr.calclevels4xShader[i], TEXTURECOLOR_UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);
		qglUseProgramObjectARB(0);

		GLSL_FinishGPUShader(&tr.calclevels4xShader[i]);

		numEtcShaders++;		
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD;
	extradefines[0] = '\0';

	if (r_shadowFilter->integer >= 1)
		Q_strcat(extradefines, 1024, "#define USE_SHADOW_FILTER\n");

	if (r_shadowFilter->integer >= 2)
		Q_strcat(extradefines, 1024, "#define USE_SHADOW_FILTER2\n");

	Q_strcat(extradefines, 1024, "#define USE_SHADOW_CASCADE\n");

	Q_strcat(extradefines, 1024, va("#define r_shadowMapSize %d\n", r_shadowMapSize->integer));
	Q_strcat(extradefines, 1024, va("#define r_shadowCascadeZFar %f\n", r_shadowCascadeZFar->value));


	if (!GLSL_InitGPUShader(&tr.shadowmaskShader, "shadowmask", attribs, qtrue, extradefines, qtrue, fallbackShadowmaskShader_vp, fallbackShadowmaskShader_fp, SHADOWMASK_UNIFORM_COUNT))
	{
		ri.Error(ERR_FATAL, "Could not load shadowmask shader!\n");
	}
	
	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_SHADOWMVP,  "u_ShadowMvp",   GLSL_MAT16);
	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_SHADOWMVP2, "u_ShadowMvp2",  GLSL_MAT16);
	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_SHADOWMVP3, "u_ShadowMvp3",  GLSL_MAT16);
	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_VIEWORIGIN, "u_ViewOrigin",  GLSL_VEC3);
	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_VIEWINFO,   "u_ViewInfo",    GLSL_VEC4);
	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_VIEWFORWARD,"u_ViewForward", GLSL_VEC3);
	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_VIEWLEFT,   "u_ViewLeft",    GLSL_VEC3);
	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_VIEWUP,     "u_ViewUp",      GLSL_VEC3);

	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_SCREENDEPTHMAP, "u_ScreenDepthMap", GLSL_INT);
	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_SHADOWMAP,      "u_ShadowMap",      GLSL_INT);
	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_SHADOWMAP2,     "u_ShadowMap2",     GLSL_INT);
	GLSL_AddUniform(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_SHADOWMAP3,     "u_ShadowMap3",     GLSL_INT);

	GLSL_EndUniforms(&tr.shadowmaskShader);

	qglUseProgramObjectARB(tr.shadowmaskShader.program);
	GLSL_SetUniformInt(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_SCREENDEPTHMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_SHADOWMAP,  TB_SHADOWMAP);
	GLSL_SetUniformInt(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_SHADOWMAP2, TB_SHADOWMAP2);
	GLSL_SetUniformInt(&tr.shadowmaskShader, SHADOWMASK_UNIFORM_SHADOWMAP3, TB_SHADOWMAP3);
	qglUseProgramObjectARB(0);

	GLSL_FinishGPUShader(&tr.shadowmaskShader);

	numEtcShaders++;


	attribs = ATTR_POSITION | ATTR_TEXCOORD;
	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.ssaoShader, "ssao", attribs, qtrue, extradefines, qtrue, fallbackSsaoShader_vp, fallbackSsaoShader_fp, SSAO_UNIFORM_COUNT))
	{
		ri.Error(ERR_FATAL, "Could not load ssao shader!\n");
	}
	
	GLSL_AddUniform(&tr.ssaoShader, SSAO_UNIFORM_VIEWINFO,   "u_ViewInfo",    GLSL_VEC4);

	GLSL_AddUniform(&tr.ssaoShader, SSAO_UNIFORM_SCREENDEPTHMAP, "u_ScreenDepthMap", GLSL_INT);

	GLSL_EndUniforms(&tr.ssaoShader);

	qglUseProgramObjectARB(tr.ssaoShader.program);
	GLSL_SetUniformInt(&tr.ssaoShader, SSAO_UNIFORM_SCREENDEPTHMAP, TB_COLORMAP);
	qglUseProgramObjectARB(0);

	GLSL_FinishGPUShader(&tr.ssaoShader);

	numEtcShaders++;


	for (i = 0; i < 2; i++)
	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD;
		extradefines[0] = '\0';

		if (i & 1)
			Q_strcat(extradefines, 1024, "#define USE_VERTICAL_BLUR\n");
		else
			Q_strcat(extradefines, 1024, "#define USE_HORIZONTAL_BLUR\n");


		if (!GLSL_InitGPUShader(&tr.depthBlurShader[i], "depthBlur", attribs, qtrue, extradefines, qtrue, fallbackDepthBlurShader_vp, fallbackDepthBlurShader_fp, DEPTHBLUR_UNIFORM_COUNT))
		{
			ri.Error(ERR_FATAL, "Could not load depthBlur shader!\n");
		}
		
		GLSL_AddUniform(&tr.depthBlurShader[i], DEPTHBLUR_UNIFORM_VIEWINFO,   "u_ViewInfo",    GLSL_VEC4);

		GLSL_AddUniform(&tr.depthBlurShader[i], DEPTHBLUR_UNIFORM_SCREENIMAGEMAP, "u_ScreenImageMap", GLSL_INT);
		GLSL_AddUniform(&tr.depthBlurShader[i], DEPTHBLUR_UNIFORM_SCREENDEPTHMAP, "u_ScreenDepthMap", GLSL_INT);

		GLSL_EndUniforms(&tr.depthBlurShader[i]);

		qglUseProgramObjectARB(tr.depthBlurShader[i].program);
		GLSL_SetUniformInt(&tr.depthBlurShader[i], DEPTHBLUR_UNIFORM_SCREENIMAGEMAP, TB_COLORMAP);
		GLSL_SetUniformInt(&tr.depthBlurShader[i], DEPTHBLUR_UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
		qglUseProgramObjectARB(0);

		GLSL_FinishGPUShader(&tr.depthBlurShader[i]);

		numEtcShaders++;
	}


	endTime = ri.Milliseconds();

	ri.Printf(PRINT_ALL, "loaded %i GLSL shaders (%i gen %i light %i etc) in %5.2f seconds\n", 
		numGenShaders + numLightShaders + numEtcShaders, numGenShaders, numLightShaders, 
		numEtcShaders, (endTime - startTime) / 1000.0);

	if (0)
	{
		GLSL_LoadGPUShaderText("Generic", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("Generic", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);

		GLSL_LoadGPUShaderText("TextureColor", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("TextureColor", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);

		GLSL_LoadGPUShaderText("FogPass", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("FogPass", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);
		
		GLSL_LoadGPUShaderText("Dlight", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("Dlight", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);

		GLSL_LoadGPUShaderText("Lightall", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("Lightall", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);
		
		GLSL_LoadGPUShaderText("Shadowfill", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("Shadowfill", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);
		
		GLSL_LoadGPUShaderText("Pshadow", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("Pshadow", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);

		GLSL_LoadGPUShaderText("Down4x", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("Down4x", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);

		GLSL_LoadGPUShaderText("Bokeh", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("Bokeh", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);

		GLSL_LoadGPUShaderText("ToneMap", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("ToneMap", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);

		GLSL_LoadGPUShaderText("CalcLevels4x", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("CalcLevels4x", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);

		GLSL_LoadGPUShaderText("Shadowmask", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("Shadowmask", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);

		GLSL_LoadGPUShaderText("Ssao", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("Ssao", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);

		GLSL_LoadGPUShaderText("DepthBlur", NULL, GL_VERTEX_SHADER_ARB,   NULL, 0, qtrue);
		GLSL_LoadGPUShaderText("DepthBlur", NULL, GL_FRAGMENT_SHADER_ARB, NULL, 0, qtrue);
	}

}

void GLSL_ShutdownGPUShaders(void)
{
	int i;

	ri.Printf(PRINT_ALL, "------- GLSL_ShutdownGPUShaders -------\n");

	qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD0);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD1);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_POSITION);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_POSITION2);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_NORMAL);
#ifdef USE_VERT_TANGENT_SPACE
	qglDisableVertexAttribArrayARB(ATTR_INDEX_TANGENT);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_BITANGENT);
#endif
	qglDisableVertexAttribArrayARB(ATTR_INDEX_NORMAL2);
#ifdef USE_VERT_TANGENT_SPACE
	qglDisableVertexAttribArrayARB(ATTR_INDEX_TANGENT2);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_BITANGENT2);
#endif
	qglDisableVertexAttribArrayARB(ATTR_INDEX_COLOR);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_LIGHTDIRECTION);
	GLSL_BindNullProgram();

	for ( i = 0; i < GENERICDEF_COUNT; i++)
		GLSL_DeleteGPUShader(&tr.genericShader[i]);

	GLSL_DeleteGPUShader(&tr.textureColorShader);
	GLSL_DeleteGPUShader(&tr.fogShader);
	GLSL_DeleteGPUShader(&tr.dlightallShader);

	for ( i = 0; i < LIGHTDEF_COUNT; i++)
		GLSL_DeleteGPUShader(&tr.lightallShader[i]);

	GLSL_DeleteGPUShader(&tr.shadowmapShader);
	GLSL_DeleteGPUShader(&tr.pshadowShader);
	GLSL_DeleteGPUShader(&tr.down4xShader);
	
	for ( i = 0; i < 2; i++)
		GLSL_DeleteGPUShader(&tr.calclevels4xShader[i]);

	glState.currentProgram = 0;
	qglUseProgramObjectARB(0);
}


void GLSL_BindProgram(shaderProgram_t * program)
{
	if(!program)
	{
		GLSL_BindNullProgram();
		return;
	}

	if(r_logFile->integer)
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment(va("--- GL_BindProgram( %s ) ---\n", program->name));
	}

	if(glState.currentProgram != program)
	{
		qglUseProgramObjectARB(program->program);
		glState.currentProgram = program;
		backEnd.pc.c_glslShaderBinds++;
	}
}


void GLSL_BindNullProgram(void)
{
	if(r_logFile->integer)
	{
		GLimp_LogComment("--- GL_BindNullProgram ---\n");
	}

	if(glState.currentProgram)
	{
		qglUseProgramObjectARB(0);
		glState.currentProgram = NULL;
	}
}


void GLSL_VertexAttribsState(uint32_t stateBits)
{
	uint32_t		diff;

	GLSL_VertexAttribPointers(stateBits);

	diff = stateBits ^ glState.vertexAttribsState;
	if(!diff)
	{
		return;
	}

	if(diff & ATTR_POSITION)
	{
		if(stateBits & ATTR_POSITION)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_POSITION )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_POSITION);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_POSITION )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_POSITION);
		}
	}

	if(diff & ATTR_TEXCOORD)
	{
		if(stateBits & ATTR_TEXCOORD)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_TEXCOORD )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD0);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_TEXCOORD )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD0);
		}
	}

	if(diff & ATTR_LIGHTCOORD)
	{
		if(stateBits & ATTR_LIGHTCOORD)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_LIGHTCOORD )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD1);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_LIGHTCOORD )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD1);
		}
	}

	if(diff & ATTR_NORMAL)
	{
		if(stateBits & ATTR_NORMAL)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_NORMAL )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_NORMAL);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_NORMAL )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_NORMAL);
		}
	}

#ifdef USE_VERT_TANGENT_SPACE
	if(diff & ATTR_TANGENT)
	{
		if(stateBits & ATTR_TANGENT)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_TANGENT )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_TANGENT);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_TANGENT )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_TANGENT);
		}
	}

	if(diff & ATTR_BITANGENT)
	{
		if(stateBits & ATTR_BITANGENT)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_BITANGENT )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_BITANGENT);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_BITANGENT )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_BITANGENT);
		}
	}
#endif

	if(diff & ATTR_COLOR)
	{
		if(stateBits & ATTR_COLOR)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_COLOR )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_COLOR);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_COLOR )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_COLOR);
		}
	}

	if(diff & ATTR_LIGHTDIRECTION)
	{
		if(stateBits & ATTR_LIGHTDIRECTION)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_LIGHTDIRECTION )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_LIGHTDIRECTION);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_LIGHTDIRECTION )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_LIGHTDIRECTION);
		}
	}

	if(diff & ATTR_POSITION2)
	{
		if(stateBits & ATTR_POSITION2)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_POSITION2 )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_POSITION2);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_POSITION2 )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_POSITION2);
		}
	}

	if(diff & ATTR_NORMAL2)
	{
		if(stateBits & ATTR_NORMAL2)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_NORMAL2 )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_NORMAL2);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_NORMAL2 )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_NORMAL2);
		}
	}

#ifdef USE_VERT_TANGENT_SPACE
	if(diff & ATTR_TANGENT2)
	{
		if(stateBits & ATTR_TANGENT2)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_TANGENT2 )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_TANGENT2);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_TANGENT2 )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_TANGENT2);
		}
	}

	if(diff & ATTR_BITANGENT2)
	{
		if(stateBits & ATTR_BITANGENT2)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_BITANGENT2 )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_BITANGENT2);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_BITANGENT2 )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_BITANGENT2);
		}
	}
#endif

	glState.vertexAttribsState = stateBits;
}

void GLSL_VertexAttribPointers(uint32_t attribBits)
{
	if(!glState.currentVBO)
	{
		ri.Error(ERR_FATAL, "GL_VertexAttribPointers: no VBO bound");
		return;
	}

	// don't just call LogComment, or we will get a call to va() every frame!
	GLimp_LogComment(va("--- GL_VertexAttribPointers( %s ) ---\n", glState.currentVBO->name));

	if((attribBits & ATTR_POSITION) && !(glState.vertexAttribPointersSet & ATTR_POSITION))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_POSITION )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_POSITION, 3, GL_FLOAT, 0, glState.currentVBO->stride_xyz, BUFFER_OFFSET(glState.currentVBO->ofs_xyz + glState.vertexAttribsNewFrame * glState.currentVBO->size_xyz));
		glState.vertexAttribPointersSet |= ATTR_POSITION;
	}

	if((attribBits & ATTR_TEXCOORD) && !(glState.vertexAttribPointersSet & ATTR_TEXCOORD))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_TEXCOORD )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_TEXCOORD0, 2, GL_FLOAT, 0, glState.currentVBO->stride_st, BUFFER_OFFSET(glState.currentVBO->ofs_st));
		glState.vertexAttribPointersSet |= ATTR_TEXCOORD;
	}

	if((attribBits & ATTR_LIGHTCOORD) && !(glState.vertexAttribPointersSet & ATTR_LIGHTCOORD))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_LIGHTCOORD )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_TEXCOORD1, 2, GL_FLOAT, 0, glState.currentVBO->stride_lightmap, BUFFER_OFFSET(glState.currentVBO->ofs_lightmap));
		glState.vertexAttribPointersSet |= ATTR_LIGHTCOORD;
	}

	if((attribBits & ATTR_NORMAL) && !(glState.vertexAttribPointersSet & ATTR_NORMAL))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_NORMAL )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_NORMAL, 3, GL_FLOAT, 0, glState.currentVBO->stride_normal, BUFFER_OFFSET(glState.currentVBO->ofs_normal + glState.vertexAttribsNewFrame * glState.currentVBO->size_normal));
		glState.vertexAttribPointersSet |= ATTR_NORMAL;
	}

#ifdef USE_VERT_TANGENT_SPACE
	if((attribBits & ATTR_TANGENT) && !(glState.vertexAttribPointersSet & ATTR_TANGENT))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_TANGENT )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_TANGENT, 3, GL_FLOAT, 0, glState.currentVBO->stride_tangent, BUFFER_OFFSET(glState.currentVBO->ofs_tangent + glState.vertexAttribsNewFrame * glState.currentVBO->size_normal)); // FIXME
		glState.vertexAttribPointersSet |= ATTR_TANGENT;
	}

	if((attribBits & ATTR_BITANGENT) && !(glState.vertexAttribPointersSet & ATTR_BITANGENT))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_BITANGENT )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_BITANGENT, 3, GL_FLOAT, 0, glState.currentVBO->stride_bitangent, BUFFER_OFFSET(glState.currentVBO->ofs_bitangent + glState.vertexAttribsNewFrame * glState.currentVBO->size_normal)); // FIXME
		glState.vertexAttribPointersSet |= ATTR_BITANGENT;
	}
#endif

	if((attribBits & ATTR_COLOR) && !(glState.vertexAttribPointersSet & ATTR_COLOR))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_COLOR )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_COLOR, 4, GL_FLOAT, 0, glState.currentVBO->stride_vertexcolor, BUFFER_OFFSET(glState.currentVBO->ofs_vertexcolor));
		glState.vertexAttribPointersSet |= ATTR_COLOR;
	}

	if((attribBits & ATTR_LIGHTDIRECTION) && !(glState.vertexAttribPointersSet & ATTR_LIGHTDIRECTION))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_LIGHTDIRECTION )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_LIGHTDIRECTION, 3, GL_FLOAT, 0, glState.currentVBO->stride_lightdir, BUFFER_OFFSET(glState.currentVBO->ofs_lightdir));
		glState.vertexAttribPointersSet |= ATTR_LIGHTDIRECTION;
	}

	if((attribBits & ATTR_POSITION2) && !(glState.vertexAttribPointersSet & ATTR_POSITION2))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_POSITION2 )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_POSITION2, 3, GL_FLOAT, 0, glState.currentVBO->stride_xyz, BUFFER_OFFSET(glState.currentVBO->ofs_xyz + glState.vertexAttribsOldFrame * glState.currentVBO->size_xyz));
		glState.vertexAttribPointersSet |= ATTR_POSITION2;
	}

	if((attribBits & ATTR_NORMAL2) && !(glState.vertexAttribPointersSet & ATTR_NORMAL2))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_NORMAL2 )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_NORMAL2, 3, GL_FLOAT, 0, glState.currentVBO->stride_normal, BUFFER_OFFSET(glState.currentVBO->ofs_normal + glState.vertexAttribsOldFrame * glState.currentVBO->size_normal));
		glState.vertexAttribPointersSet |= ATTR_NORMAL2;
	}

#ifdef USE_VERT_TANGENT_SPACE
	if((attribBits & ATTR_TANGENT2) && !(glState.vertexAttribPointersSet & ATTR_TANGENT2))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_TANGENT2 )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_TANGENT2, 3, GL_FLOAT, 0, glState.currentVBO->stride_tangent, BUFFER_OFFSET(glState.currentVBO->ofs_tangent + glState.vertexAttribsOldFrame * glState.currentVBO->size_normal)); // FIXME
		glState.vertexAttribPointersSet |= ATTR_TANGENT2;
	}

	if((attribBits & ATTR_BITANGENT2) && !(glState.vertexAttribPointersSet & ATTR_BITANGENT2))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_BITANGENT2 )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_BITANGENT2, 3, GL_FLOAT, 0, glState.currentVBO->stride_bitangent, BUFFER_OFFSET(glState.currentVBO->ofs_bitangent + glState.vertexAttribsOldFrame * glState.currentVBO->size_normal)); // FIXME
		glState.vertexAttribPointersSet |= ATTR_BITANGENT2;
	}
#endif

}

shaderProgram_t *GLSL_GetGenericShaderProgram(int stage)
{
	shaderStage_t *pStage = tess.xstages[stage];
	int shaderAttribs = 0;

	if (tess.fogNum && pStage->adjustColorsForFog)
	{
		shaderAttribs |= GENERICDEF_USE_FOG;
	}

	if (pStage->bundle[1].image[0] && tess.shader->multitextureEnv)
	{
		shaderAttribs |= GENERICDEF_USE_LIGHTMAP;
	}

	switch (pStage->rgbGen)
	{
		case CGEN_LIGHTING_DIFFUSE:
			shaderAttribs |= GENERICDEF_USE_RGBAGEN;
			break;
		default:
			break;
	}

	switch (pStage->alphaGen)
	{
		case AGEN_LIGHTING_SPECULAR:
		case AGEN_PORTAL:
		case AGEN_FRESNEL:
			shaderAttribs |= GENERICDEF_USE_RGBAGEN;
			break;
		default:
			break;
	}

	if (pStage->bundle[0].tcGen != TCGEN_TEXTURE)
	{
		shaderAttribs |= GENERICDEF_USE_TCGEN;
	}

	if (tess.shader->numDeforms && !ShaderRequiresCPUDeforms(tess.shader))
	{
		shaderAttribs |= GENERICDEF_USE_DEFORM_VERTEXES;
	}

	if (glState.vertexAttribsInterpolation > 0.0f && backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
	{
		shaderAttribs |= GENERICDEF_USE_VERTEX_ANIMATION;
	}

	return &tr.genericShader[shaderAttribs];
}
