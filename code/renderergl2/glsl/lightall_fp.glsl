uniform sampler2D u_DiffuseMap;

#if defined(USE_LIGHTMAP)
uniform sampler2D u_LightMap;
#endif

#if defined(USE_NORMALMAP)
uniform sampler2D u_NormalMap;
#endif

#if defined(USE_DELUXEMAP)
uniform sampler2D u_DeluxeMap;
#endif

#if defined(USE_SPECULARMAP)
uniform sampler2D u_SpecularMap;
#endif

#if defined(USE_SHADOWMAP)
uniform sampler2D u_ShadowMap;
#endif

#if defined(USE_CUBEMAP)
uniform samplerCube u_CubeMap;
#endif

#if defined(USE_LIGHT_VECTOR)
uniform vec3      u_DirectedLight;
uniform vec3      u_AmbientLight;
uniform float     u_LightRadius;
#endif

#if defined(USE_PRIMARY_LIGHT) || defined(USE_SHADOWMAP)
uniform vec3  u_PrimaryLightColor;
uniform vec3  u_PrimaryLightAmbient;
uniform float u_PrimaryLightRadius;
#endif

#if defined(USE_LIGHT)
uniform vec2      u_MaterialInfo;
#endif

varying vec2      var_DiffuseTex;
#if defined(USE_LIGHTMAP)
varying vec2      var_LightTex;
#endif
varying vec4      var_Color;

#if (defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)) || defined(USE_PARALLAXMAP)
varying vec3      var_ViewDir;
varying vec3      var_Normal;
varying vec3      var_Tangent;
varying vec3      var_Bitangent;
#endif

#if defined(USE_LIGHT_VERTEX) && !defined(USE_FAST_LIGHT)
varying vec3      var_lightColor;
#endif

#if defined(USE_LIGHT) && !defined(USE_DELUXEMAP)
varying vec4      var_LightDir;
#endif

#if defined(USE_PRIMARY_LIGHT) || defined(USE_SHADOWMAP)
varying vec3      var_PrimaryLightDir;
#endif


#define EPSILON 0.00000001

#if defined(USE_PARALLAXMAP)
float SampleDepth(sampler2D normalMap, vec2 t)
{
  #if defined(SWIZZLE_NORMALMAP)
	return 1.0 - texture2D(normalMap, t).r;
  #else
	return 1.0 - texture2D(normalMap, t).a;
  #endif
}

float RayIntersectDisplaceMap(vec2 dp, vec2 ds, sampler2D normalMap)
{
	const int linearSearchSteps = 16;
	const int binarySearchSteps = 6;

	// current size of search window
	float size = 1.0 / float(linearSearchSteps);

	// current depth position
	float depth = 0.0;

	// best match found (starts with last position 1.0)
	float bestDepth = 1.0;

	// search front to back for first point inside object
	for(int i = 0; i < linearSearchSteps - 1; ++i)
	{
		depth += size;
		
		float t = SampleDepth(normalMap, dp + ds * depth);
		
		if(bestDepth > 0.996)		// if no depth found yet
			if(depth >= t)
				bestDepth = depth;	// store best depth
	}

	depth = bestDepth;
	
	// recurse around first point (depth) for closest match
	for(int i = 0; i < binarySearchSteps; ++i)
	{
		size *= 0.5;

		float t = SampleDepth(normalMap, dp + ds * depth);
		
		if(depth >= t)
		{
			bestDepth = depth;
			depth -= 2.0 * size;
		}

		depth += size;
	}

	return bestDepth;
}
#endif

vec3 CalcDiffuse(vec3 diffuseAlbedo, vec3 N, vec3 L, vec3 E, float NE, float NL, float shininess)
{
  #if defined(USE_OREN_NAYAR) || defined(USE_TRIACE_OREN_NAYAR)
	float gamma = dot(E, L) - NE * NL;
	float B = 2.22222 + 0.1 * shininess;
		
    #if defined(USE_OREN_NAYAR)
	float A = 1.0 - 1.0 / (2.0 + 0.33 * shininess);
	gamma = clamp(gamma, 0.0, 1.0);
    #endif
	
    #if defined(USE_TRIACE_OREN_NAYAR)
	float A = 1.0 - 1.0 / (2.0 + 0.65 * shininess);

	if (gamma >= 0.0)
    #endif
	{
		B = max(B * max(NL, NE), EPSILON);
	}

	return diffuseAlbedo * (A + gamma / B);
  #else
	return diffuseAlbedo;
  #endif
}

vec3 EnvironmentBRDF(float gloss, float NE, vec3 specular)
{
  #if 1
	// from http://blog.selfshadow.com/publications/s2013-shading-course/lazarov/s2013_pbs_black_ops_2_notes.pdf
	vec4 t = vec4( 1/0.96, 0.475, (0.0275 - 0.25 * 0.04)/0.96,0.25 ) * gloss;
	t += vec4( 0.0, 0.0, (0.015 - 0.75 * 0.04)/0.96,0.75 );
	float a0 = t.x * min( t.y, exp2( -9.28 * NE ) ) + t.z;
	float a1 = t.w;
	return clamp( a0 + specular * ( a1 - a0 ), 0.0, 1.0 );
  #elif 0
    // from http://seblagarde.wordpress.com/2011/08/17/hello-world/
	return mix(specular.rgb, max(specular.rgb, vec3(gloss)), CalcFresnel(NE));
  #else
    // from http://advances.realtimerendering.com/s2011/Lazarov-Physically-Based-Lighting-in-Black-Ops%20%28Siggraph%202011%20Advances%20in%20Real-Time%20Rendering%20Course%29.pptx
	return mix(specular.rgb, vec3(1.0), CalcFresnel(NE) / (4.0 - 3.0 * gloss));
  #endif
}

float CalcBlinn(float NH, float shininess)
{
#if 0
    // from http://seblagarde.wordpress.com/2012/06/03/spherical-gaussien-approximation-for-blinn-phong-phong-and-fresnel/
	float a = shininess + 0.775;
    return exp(a * NH - a);
#else
	return pow(NH, shininess);
#endif
}

float CalcGGX(float NH, float shininess)
{
	// from http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes.pdf
	float m_sq = 2.0 / shininess;
	float d = ((NH * NH) * (m_sq - 1.0) + 1.0);
	return m_sq / (d * d);
}

float CalcFresnel(float EH)
{
#if 1
	// From http://seblagarde.wordpress.com/2012/06/03/spherical-gaussien-approximation-for-blinn-phong-phong-and-fresnel/
	return exp2((-5.55473 * EH - 6.98316) * EH);
#elif 0
	float blend = 1.0 - EH;
	float blend2 = blend * blend;
	blend *= blend2 * blend2;
	
	return blend;
#else
	return pow(1.0 - NH, 5.0);
#endif
}

float CalcVisibility(float NH, float NL, float NE, float EH, float shininess)
{
#if 0
	float geo = 2.0 * NH * min(NE, NL);
	geo /= max(EH, geo);
	
	return geo;
#else
	// Modified from http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes.pdf
	// NL, NE in numerator factored out from cook-torrance
  #if defined(USE_GGX)
	float roughness = sqrt(2.0 / (shininess + 2.0));
	float k = (roughness + 1.0);
	k *= k * 0.125;
  #else
    float k = 2.0 / sqrt(3.1415926535 * (shininess + 2.0));
  #endif
	float k2 = 1.0 - k;
	
	float invGeo1 = NL * k2 + k;
	float invGeo2 = NE * k2 + k;
	
	return 1.0 / (invGeo1 * invGeo2);
  #endif
}


vec3 CalcSpecular(vec3 specular, float NH, float NL, float NE, float EH, float shininess)
{
	float blinn = CalcBlinn(NH, shininess);
	vec3 fSpecular = mix(specular, vec3(1.0), CalcFresnel(EH));
	float vis = CalcVisibility(NH, NL, NE, EH, shininess);

  #if defined(USE_BLINN)
    // Normalized Blinn-Phong
	return specular  * blinn * (shininess * 0.125    + 1.0);
  #elif defined(USE_BLINN_FRESNEL)
    // Normalized Blinn-Phong with Fresnel
	return fSpecular * blinn * (shininess * 0.125    + 1.0);
  #elif defined(USE_MCAULEY)
    // Cook-Torrance as done by Stephen McAuley
	// http://blog.selfshadow.com/publications/s2012-shading-course/mcauley/s2012_pbs_farcry3_notes_v2.pdf
	return fSpecular * blinn * (shininess * 0.25     + 0.125);
  #elif defined(USE_GOTANDA)
    // Neumann-Neumann as done by Yoshiharu Gotanda
	// http://research.tri-ace.com/Data/s2012_beyond_CourseNotes.pdf
	return fSpecular * blinn * (shininess * 0.124858 + 0.269182) / max(max(NL, NE), EPSILON);
  #elif defined(USE_LAZAROV)
    // Cook-Torrance as done by Dimitar Lazarov
	// http://blog.selfshadow.com/publications/s2013-shading-course/lazarov/s2013_pbs_black_ops_2_notes.pdf
	return fSpecular * blinn * (shininess * 0.125    + 0.25)     * vis;
  #endif
  
	return vec3(0.0);
}


float CalcLightAttenuation(vec3 dir, float sqrRadius)
{
	// point light at >0 radius, directional otherwise
	float point = float(sqrRadius > 0.0);

	// inverse square light
	float attenuation = sqrRadius / dot(dir, dir);

	// zero light at radius, approximating q3 style
	// also don't attenuate directional light
	attenuation = (0.5 * attenuation - 1.5) * point + 1.0;
	
	// clamp attenuation
	#if defined(NO_LIGHT_CLAMP)
	attenuation *= float(attenuation > 0.0);
	#else
	attenuation = clamp(attenuation, 0.0, 1.0);
	#endif
	
	return attenuation;
}


void main()
{
	vec3 L, N, E, H;
	float NL, NH, NE, EH;
	
#if (defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)) || defined(USE_PARALLAXMAP)
	mat3 tangentToWorld = mat3(var_Tangent, var_Bitangent, var_Normal);
#endif

#if defined(USE_DELUXEMAP)
	L = (2.0 * texture2D(u_DeluxeMap, var_LightTex).xyz - vec3(1.0));
  #if defined(USE_TANGENT_SPACE_LIGHT)
    L = L * tangentToWorld;
  #endif
#elif defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)
	L = var_LightDir.xyz;
#endif

#if (defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)) || defined(USE_PARALLAXMAP)
	E = normalize(var_ViewDir);
#endif

#if defined(USE_LIGHTMAP)
	vec4 lightSample = texture2D(u_LightMap, var_LightTex).rgba;
  #if defined(RGBM_LIGHTMAP)
	lightSample.rgb *= 32.0 * lightSample.a;
  #endif
	vec3 lightColor = lightSample.rgb;
#elif defined(USE_LIGHT_VECTOR) && !defined(USE_FAST_LIGHT)
	vec3 lightColor   = u_DirectedLight * CalcLightAttenuation(L, u_LightRadius * u_LightRadius);
	vec3 ambientColor = u_AmbientLight;
#elif defined(USE_LIGHT_VERTEX) && !defined(USE_FAST_LIGHT)
	vec3 lightColor = var_lightColor;
#endif

	vec2 texCoords = var_DiffuseTex;

#if defined(USE_PARALLAXMAP)
  #if defined(USE_TANGENT_SPACE_LIGHT)
	vec3 offsetDir = E;
  #else
	vec3 offsetDir = E * tangentToWorld;
  #endif

	offsetDir.xy *= -0.05 / offsetDir.z;

	texCoords += offsetDir.xy * RayIntersectDisplaceMap(texCoords, offsetDir.xy, u_NormalMap);
#endif

	vec4 diffuse = texture2D(u_DiffuseMap, texCoords);

#if defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)

  #if defined(USE_LINEAR_LIGHT)
	diffuse.rgb *= diffuse.rgb;
  #endif

  #if defined(USE_NORMALMAP)
    #if defined(SWIZZLE_NORMALMAP)
	N.xy = 2.0 * texture2D(u_NormalMap, texCoords).ag - vec2(1.0);
    #else
	N.xy = 2.0 * texture2D(u_NormalMap, texCoords).rg - vec2(1.0);
    #endif
	N.z = sqrt(1.0 - clamp(dot(N.xy, N.xy), 0.0, 1.0));
    #if !defined(USE_TANGENT_SPACE_LIGHT)
    N = normalize(tangentToWorld * N);
    #endif
  #elif defined(USE_TANGENT_SPACE_LIGHT)
	N = vec3(0.0, 0.0, 1.0);
  #else
    N = normalize(var_Normal);
  #endif
  
	L = normalize(L);

  #if defined(USE_SHADOWMAP) 
	vec2 shadowTex = gl_FragCoord.xy * r_FBufScale;
	float shadowValue = texture2D(u_ShadowMap, shadowTex).r;

	// surfaces not facing the light are always shadowed
	#if defined(USE_TANGENT_SPACE_LIGHT)
	shadowValue *= float(var_PrimaryLightDir.z > 0.0);
	#else
	shadowValue *= float(dot(var_Normal, var_PrimaryLightDir) > 0.0);
	#endif
  
    #if defined(SHADOWMAP_MODULATE)
	//vec3 shadowColor = min(u_PrimaryLightAmbient, lightColor);
	vec3 shadowColor = u_PrimaryLightAmbient * lightColor;

      #if 0
	// Only shadow when the world light is parallel to the primary light
	shadowValue = 1.0 + (shadowValue - 1.0) * clamp(dot(L, var_PrimaryLightDir), 0.0, 1.0);
      #endif
	lightColor = mix(shadowColor, lightColor, shadowValue);
    #endif
  #endif

  #if defined(USE_LIGHTMAP) || defined(USE_LIGHT_VERTEX)
	vec3 ambientColor = lightColor;
	
	#if defined(USE_TANGENT_SPACE_LIGHT)
	float surfNL = L.z;
	#else
	float surfNL = clamp(dot(var_Normal, L), 0.0, 1.0);
	#endif

	// Scale the incoming light to compensate for the baked-in light angle
	// attenuation.
	lightColor /= max(surfNL, 0.25);

	// Recover any unused light as ambient, in case attenuation is over 4x or
	// light is below the surface
	ambientColor -= lightColor * surfNL;
  #endif
  
	vec3 reflectance;

	NL = clamp(dot(N, L), 0.0, 1.0);
	NE = clamp(dot(N, E), 0.0, 1.0);

  #if defined(USE_SPECULARMAP)
	vec4 specular = texture2D(u_SpecularMap, texCoords);
    #if defined(USE_LINEAR_LIGHT)
	specular.rgb *= specular.rgb;
	#endif
  #else
	vec4 specular = vec4(1.0);
  #endif

	specular *= u_MaterialInfo.xxxy;
	
	float gloss = specular.a;
	float shininess = exp2(gloss * 13.0);
	float localOcclusion = clamp((diffuse.r + diffuse.g + diffuse.b) * 16.0f, 0.0, 1.0);

  #if defined(SPECULAR_IS_METALLIC)
    // diffuse is actually base color, and red of specular is metallicness
	float metallic = specular.r;
	
	specular.rgb = vec3(0.04) + 0.96 * diffuse.rgb * metallic;
	diffuse.rgb *= 1.0 - metallic;
  #else
	// adjust diffuse by specular reflectance, to maintain energy conservation
	diffuse.rgb *= vec3(1.0) - specular.rgb;
  #endif
  
	
	reflectance = CalcDiffuse(diffuse.rgb, N, L, E, NE, NL, shininess);

  #if defined(r_deluxeSpecular) || defined(USE_LIGHT_VECTOR)
	float adjShininess = shininess;
	
	#if !defined(USE_LIGHT_VECTOR)
	adjShininess = exp2(gloss * r_deluxeSpecular * 13.0);
	#endif
	
	H = normalize(L + E);

	EH = clamp(dot(E, H), 0.0, 1.0);
	NH = clamp(dot(N, H), 0.0, 1.0);

    #if !defined(USE_LIGHT_VECTOR)
	reflectance += CalcSpecular(specular.rgb, NH, NL, NE, EH, adjShininess) * r_deluxeSpecular * localOcclusion;
    #else
	reflectance += CalcSpecular(specular.rgb, NH, NL, NE, EH, adjShininess) * localOcclusion;
    #endif
  #endif
	
	gl_FragColor.rgb  = lightColor   * reflectance * NL;  
	gl_FragColor.rgb += ambientColor * (diffuse.rgb + specular.rgb);
	
  #if defined(USE_CUBEMAP)
	reflectance = EnvironmentBRDF(gloss, NE, specular.rgb);

	vec3 R = reflect(E, N);
	#if defined(USE_TANGENT_SPACE_LIGHT)
	R = tangentToWorld * R;
	#endif

    vec3 cubeLightColor = textureCubeLod(u_CubeMap, R, 7.0 - gloss * 7.0).rgb;

    #if defined(USE_LINEAR_LIGHT)
	cubeLightColor *= cubeLightColor;
    #endif

	#if defined(USE_LIGHTMAP)
	cubeLightColor *= lightSample.rgb;
	#elif defined (USE_LIGHT_VERTEX)
	cubeLightColor *= var_lightColor;
	#else
	cubeLightColor *= lightColor * NL + ambientColor;
	#endif
	
	//gl_FragColor.rgb += diffuse.rgb * textureCubeLod(u_CubeMap, N, 7.0).rgb;
	gl_FragColor.rgb += cubeLightColor * reflectance * localOcclusion;
  #endif

  #if defined(USE_PRIMARY_LIGHT)
	L = normalize(var_PrimaryLightDir);
	NL = clamp(dot(N, L), 0.0, 1.0);

	H = normalize(L + E);
	EH = clamp(dot(E, H), 0.0, 1.0);
	NH = clamp(dot(N, H), 0.0, 1.0);

	reflectance  = CalcDiffuse(diffuse.rgb, N, L, E, NE, NL, shininess);
	reflectance += CalcSpecular(specular.rgb, NH, NL, NE, EH, shininess);

	lightColor = u_PrimaryLightColor; // * CalcLightAttenuation(L, u_PrimaryLightRadius * u_PrimaryLightRadius);
	
    #if defined(USE_SHADOWMAP)
	lightColor *= shadowValue;
    #endif

	gl_FragColor.rgb += lightColor * reflectance * NL;
  #endif
  
  #if defined(USE_LINEAR_LIGHT)
	gl_FragColor.rgb = sqrt(gl_FragColor.rgb);
  #endif

	gl_FragColor.a = diffuse.a;
#else
	gl_FragColor = diffuse;
  #if defined(USE_LIGHTMAP) 
	gl_FragColor.rgb *= lightColor;
  #endif
#endif

	gl_FragColor *= var_Color;
}
