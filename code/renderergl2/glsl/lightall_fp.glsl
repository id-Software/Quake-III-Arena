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

#if defined(USE_NORMALMAP) || defined(USE_DELUXEMAP) || defined(USE_SPECULARMAP) || defined(USE_CUBEMAP)
uniform vec4      u_EnableTextures; // x = normal, y = deluxe, z = specular, w = cube
#endif

#if defined(USE_LIGHT_VECTOR) && !defined(USE_FAST_LIGHT)
uniform vec3      u_DirectedLight;
uniform vec3      u_AmbientLight;
#endif

#if defined(USE_PRIMARY_LIGHT) || defined(USE_SHADOWMAP)
uniform vec3  u_PrimaryLightColor;
uniform vec3  u_PrimaryLightAmbient;
#endif

#if defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)
uniform vec2      u_MaterialInfo;
#endif

varying vec4      var_TexCoords;

varying vec4      var_Color;

#if (defined(USE_LIGHT) && !defined(USE_FAST_LIGHT))
  #if defined(USE_VERT_TANGENT_SPACE)
varying vec4   var_Normal;
varying vec4   var_Tangent;
varying vec4   var_Bitangent;
  #else
varying vec3   var_Normal;
varying vec3   var_ViewDir;
  #endif
#endif

#if defined(USE_LIGHT_VERTEX) && !defined(USE_FAST_LIGHT)
varying vec3      var_LightColor;
#endif

#if defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)
varying vec4      var_LightDir;
#endif

#if defined(USE_PRIMARY_LIGHT) || defined(USE_SHADOWMAP)
varying vec4      var_PrimaryLightDir;
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
#if defined(USE_BLINN) || defined(USE_BLINN_FRESNEL)
	// Normalized Blinn-Phong
	float norm = shininess * 0.125    + 1.0;
#elif defined(USE_MCAULEY)
	// Cook-Torrance as done by Stephen McAuley
	// http://blog.selfshadow.com/publications/s2012-shading-course/mcauley/s2012_pbs_farcry3_notes_v2.pdf
	float norm = shininess * 0.25     + 0.125;
#elif defined(USE_GOTANDA)
	// Neumann-Neumann as done by Yoshiharu Gotanda
	// http://research.tri-ace.com/Data/s2012_beyond_CourseNotes.pdf
	float norm = shininess * 0.124858 + 0.269182;
#elif defined(USE_LAZAROV)
	// Cook-Torrance as done by Dimitar Lazarov
	// http://blog.selfshadow.com/publications/s2013-shading-course/lazarov/s2013_pbs_black_ops_2_notes.pdf
	float norm = shininess * 0.125    + 0.25;
#else
	float norm = 1.0;
#endif

#if 0
	// from http://seblagarde.wordpress.com/2012/06/03/spherical-gaussien-approximation-for-blinn-phong-phong-and-fresnel/
	float a = shininess + 0.775;
	return norm * exp(a * NH - a);
#else
	return norm * pow(NH, shininess);
#endif
}

float CalcGGX(float NH, float gloss)
{
	// from http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
	float a_sq = exp2(gloss * -13.0 + 1.0);
	float d = ((NH * NH) * (a_sq - 1.0) + 1.0);
	return a_sq / (d * d);
}

float CalcFresnel(float EH)
{
#if 1
	// From http://blog.selfshadow.com/publications/s2013-shading-course/lazarov/s2013_pbs_black_ops_2_notes.pdf
	// not accurate, but fast
	return exp2(-10.0 * EH);
#elif 0
	// From http://seblagarde.wordpress.com/2012/06/03/spherical-gaussien-approximation-for-blinn-phong-phong-and-fresnel/
	return exp2((-5.55473 * EH - 6.98316) * EH);
#elif 0
	float blend = 1.0 - EH;
	float blend2 = blend * blend;
	blend *= blend2 * blend2;
	
	return blend;
#else
	return pow(1.0 - EH, 5.0);
#endif
}

float CalcVisibility(float NH, float NL, float NE, float EH, float gloss)
{
#if defined(USE_GOTANDA)
	// Neumann-Neumann as done by Yoshiharu Gotanda
	// http://research.tri-ace.com/Data/s2012_beyond_CourseNotes.pdf
	return 1.0 / max(max(NL, NE), EPSILON);
#elif defined(USE_LAZAROV)
	// Cook-Torrance as done by Dimitar Lazarov
	// http://blog.selfshadow.com/publications/s2013-shading-course/lazarov/s2013_pbs_black_ops_2_notes.pdf
	float k = min(1.0, gloss + 0.545);
	return 1.0 / (k * (EH * EH - 1.0) + 1.0);
#elif defined(USE_GGX)
	float roughness = exp2(gloss * -6.5);

	// Modified from http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
	// NL, NE in numerator factored out from cook-torrance
	float k = roughness + 1.0;
	k *= k * 0.125;

	float k2 = 1.0 - k;
	
	float invGeo1 = NL * k2 + k;
	float invGeo2 = NE * k2 + k;

	return 1.0 / (invGeo1 * invGeo2);
#else
	return 1.0;
#endif
}


vec3 CalcSpecular(vec3 specular, float NH, float NL, float NE, float EH, float gloss, float shininess)
{
#if defined(USE_GGX)
	float distrib = CalcGGX(NH, gloss);
#else
	float distrib = CalcBlinn(NH, shininess);
#endif

#if defined(USE_BLINN)
	vec3 fSpecular = specular;
#else
	vec3 fSpecular = mix(specular, vec3(1.0), CalcFresnel(EH));
#endif

	float vis = CalcVisibility(NH, NL, NE, EH, gloss);

	return fSpecular * (distrib * vis);
}


float CalcLightAttenuation(float point, float normDist)
{
	// zero light at 1.0, approximating q3 style
	// also don't attenuate directional light
	float attenuation = (0.5 * normDist - 1.5) * point + 1.0;

	// clamp attenuation
	#if defined(NO_LIGHT_CLAMP)
	attenuation = max(attenuation, 0.0);
	#else
	attenuation = clamp(attenuation, 0.0, 1.0);
	#endif

	return attenuation;
}

// from http://www.thetenthplanet.de/archives/1180
mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv )
{
	// get edge vectors of the pixel triangle
	vec3 dp1 = dFdx( p );
	vec3 dp2 = dFdy( p );
	vec2 duv1 = dFdx( uv );
	vec2 duv2 = dFdy( uv );

	// solve the linear system
	vec3 dp2perp = cross( dp2, N );
	vec3 dp1perp = cross( N, dp1 );
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame 
	float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
	return mat3( T * invmax, B * invmax, N );
}

void main()
{
	vec3 L, N, E, H;
	float NL, NH, NE, EH;

#if defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)
  #if defined(USE_VERT_TANGENT_SPACE)
	mat3 tangentToWorld = mat3(var_Tangent.xyz, var_Bitangent.xyz, var_Normal.xyz);
	E = vec3(var_Normal.w, var_Tangent.w, var_Bitangent.w);
  #else
	mat3 tangentToWorld = cotangent_frame(var_Normal, -var_ViewDir, var_TexCoords.xy);
	E = var_ViewDir;
  #endif

	E = normalize(E);

	L = var_LightDir.xyz;
  #if defined(USE_DELUXEMAP)
	L += (texture2D(u_DeluxeMap, var_TexCoords.zw).xyz - vec3(0.5)) * u_EnableTextures.y;
  #endif
	float sqrLightDist = dot(L, L);
#endif

#if defined(USE_LIGHTMAP)
	vec4 lightSample = texture2D(u_LightMap, var_TexCoords.zw);
	vec3 lightColor = lightSample.rgb;
  #if defined(RGBM_LIGHTMAP)
	lightColor *= lightSample.a;
  #endif
#elif defined(USE_LIGHT_VECTOR) && !defined(USE_FAST_LIGHT)
	vec3 lightColor   = u_DirectedLight * CalcLightAttenuation(float(var_LightDir.w > 0.0), var_LightDir.w / sqrLightDist);
	vec3 ambientColor = u_AmbientLight;
#elif defined(USE_LIGHT_VERTEX) && !defined(USE_FAST_LIGHT)
	vec3 lightColor = var_LightColor;
#endif

	vec2 texCoords = var_TexCoords.xy;

#if defined(USE_PARALLAXMAP)
	vec3 offsetDir = normalize(E * tangentToWorld);

	offsetDir.xy *= -0.05 / offsetDir.z;

	texCoords += offsetDir.xy * RayIntersectDisplaceMap(texCoords, offsetDir.xy, u_NormalMap);
#endif

	vec4 diffuse = texture2D(u_DiffuseMap, texCoords);
#if defined(USE_GAMMA2_TEXTURES)
	diffuse.rgb *= diffuse.rgb;
#endif


#if defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)
  #if defined(USE_NORMALMAP)
    #if defined(SWIZZLE_NORMALMAP)
	N.xy = texture2D(u_NormalMap, texCoords).ag - vec2(0.5);
    #else
	N.xy = texture2D(u_NormalMap, texCoords).rg - vec2(0.5);
    #endif
	N.xy *= u_EnableTextures.x;
	N.z = sqrt((0.25 - N.x * N.x) - N.y * N.y);
	N = tangentToWorld * N;
  #else
	N = var_Normal.xyz;
  #endif

	N = normalize(N);
	L /= sqrt(sqrLightDist);

  #if defined(USE_SHADOWMAP) 
	vec2 shadowTex = gl_FragCoord.xy * r_FBufScale;
	float shadowValue = texture2D(u_ShadowMap, shadowTex).r;

	// surfaces not facing the light are always shadowed
	shadowValue *= float(dot(var_Normal.xyz, var_PrimaryLightDir.xyz) > 0.0);

    #if defined(SHADOWMAP_MODULATE)
	//vec3 shadowColor = min(u_PrimaryLightAmbient, lightColor);
	vec3 shadowColor = u_PrimaryLightAmbient * lightColor;

      #if 0
	// Only shadow when the world light is parallel to the primary light
	shadowValue = 1.0 + (shadowValue - 1.0) * clamp(dot(L, var_PrimaryLightDir.xyz), 0.0, 1.0);
      #endif
	lightColor = mix(shadowColor, lightColor, shadowValue);
    #endif
  #endif

  #if defined(USE_LIGHTMAP) || defined(USE_LIGHT_VERTEX)
	vec3 ambientColor = lightColor;
	float surfNL = clamp(dot(var_Normal.xyz, L), 0.0, 1.0);

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

	vec4 specular = vec4(1.0);
  #if defined(USE_SPECULARMAP)
	specular += texture2D(u_SpecularMap, texCoords) * u_EnableTextures.z - u_EnableTextures.zzzz;
    #if defined(USE_GAMMA2_TEXTURES)
	specular.rgb *= specular.rgb;
    #endif
  #endif

	specular *= u_MaterialInfo.xxxy;

	float gloss = specular.a;
	float shininess = exp2(gloss * 13.0);

  #if defined(SPECULAR_IS_METALLIC)
	// diffuse is actually base color, and red of specular is metallicness
	float metallic = specular.r;

	specular.rgb = (0.96 * metallic) * diffuse.rgb + vec3(0.04);
	diffuse.rgb *= 1.0 - metallic;
  #else
	// adjust diffuse by specular reflectance, to maintain energy conservation
	diffuse.rgb *= vec3(1.0) - specular.rgb;
  #endif

	reflectance = CalcDiffuse(diffuse.rgb, N, L, E, NE, NL, shininess);

  #if defined(r_deluxeSpecular) || defined(USE_LIGHT_VECTOR)
	float adjGloss = gloss;
	float adjShininess = shininess;

    #if !defined(USE_LIGHT_VECTOR)
	adjGloss *= r_deluxeSpecular;
	adjShininess = exp2(adjGloss * 13.0);
    #endif

	H = normalize(L + E);

	EH = clamp(dot(E, H), 0.0, 1.0);
	NH = clamp(dot(N, H), 0.0, 1.0);

    #if !defined(USE_LIGHT_VECTOR)
	reflectance += CalcSpecular(specular.rgb, NH, NL, NE, EH, adjGloss, adjShininess) * r_deluxeSpecular;
    #else
	reflectance += CalcSpecular(specular.rgb, NH, NL, NE, EH, adjGloss, adjShininess);
    #endif
  #endif

	gl_FragColor.rgb  = lightColor   * reflectance * NL;
	gl_FragColor.rgb += ambientColor * (diffuse.rgb + specular.rgb);

  #if defined(USE_CUBEMAP)
	reflectance = EnvironmentBRDF(gloss, NE, specular.rgb);

	vec3 R = reflect(E, N);

	vec3 cubeLightColor = textureCubeLod(u_CubeMap, R, 7.0 - gloss * 7.0).rgb * u_EnableTextures.w;

	#if defined(USE_LIGHTMAP)
	cubeLightColor *= lightSample.rgb;
	#elif defined (USE_LIGHT_VERTEX)
	cubeLightColor *= var_LightColor;
	#else
	cubeLightColor *= lightColor * NL + ambientColor;
	#endif

	//gl_FragColor.rgb += diffuse.rgb * textureCubeLod(u_CubeMap, N, 7.0).rgb * u_EnableTextures.w;
	gl_FragColor.rgb += cubeLightColor * reflectance;
  #endif

  #if defined(USE_PRIMARY_LIGHT)
	vec3 L2, H2;
	float NL2, EH2, NH2;

	L2 = var_PrimaryLightDir.xyz;

	// enable when point lights are supported as primary lights
	//sqrLightDist = dot(L2, L2);
	//L2 /= sqrt(sqrLightDist);

	NL2 = clamp(dot(N, L2), 0.0, 1.0);

	H2 = normalize(L2 + E);
	EH2 = clamp(dot(E, H2), 0.0, 1.0);
	NH2 = clamp(dot(N, H2), 0.0, 1.0);

	reflectance  = CalcDiffuse(diffuse.rgb, N, L2, E, NE, NL2, shininess);
	reflectance += CalcSpecular(specular.rgb, NH2, NL2, NE, EH2, gloss, shininess);

	lightColor = u_PrimaryLightColor;

	// enable when point lights are supported as primary lights
	//lightColor *= CalcLightAttenuation(float(u_PrimaryLightDir.w > 0.0), u_PrimaryLightDir.w / sqrLightDist);

    #if defined(USE_SHADOWMAP)
	lightColor *= shadowValue;
    #endif

	gl_FragColor.rgb += lightColor * reflectance * NL2;
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
