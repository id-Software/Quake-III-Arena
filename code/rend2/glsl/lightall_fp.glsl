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

uniform vec3      u_ViewOrigin;

#if defined(USE_TCGEN)
uniform int    u_TCGen0;
#endif

#if defined(USE_LIGHT_VECTOR)
uniform vec3      u_DirectedLight;
uniform vec3      u_AmbientLight;
uniform float     u_LightRadius;
#endif

#if defined(USE_LIGHT)
uniform vec2      u_MaterialInfo;
#endif

varying vec2      var_DiffuseTex;
#if defined(USE_LIGHTMAP)
varying vec2      var_LightTex;
#endif
varying vec4      var_Color;

#if defined(USE_NORMALMAP) && !defined(USE_VERT_TANGENT_SPACE)
varying vec3      var_Position;
#endif

varying vec3      var_SampleToView;

#if !defined(USE_FAST_LIGHT)
varying vec3      var_Normal;
#endif

#if defined(USE_VERT_TANGENT_SPACE)
varying vec3      var_Tangent;
varying vec3      var_Bitangent;
#endif

varying vec3      var_VertLight;

#if defined(USE_LIGHT) && !defined(USE_DELUXEMAP)
varying vec3      var_WorldLight;
#endif

#define EPSILON 0.00000001

#if defined(USE_PARALLAXMAP)
float SampleHeight(sampler2D normalMap, vec2 t)
{
  #if defined(SWIZZLE_NORMALMAP)
	return texture2D(normalMap, t).r;
  #else
	return texture2D(normalMap, t).a;
  #endif
}

float RayIntersectDisplaceMap(vec2 dp, vec2 ds, sampler2D normalMap)
{
	const int linearSearchSteps = 16;
	const int binarySearchSteps = 6;

	float depthStep = 1.0 / float(linearSearchSteps);

	// current size of search window
	float size = depthStep;

	// current depth position
	float depth = 0.0;

	// best match found (starts with last position 1.0)
	float bestDepth = 1.0;

	// search front to back for first point inside object
	for(int i = 0; i < linearSearchSteps - 1; ++i)
	{
		depth += size;
		
		float t = 1.0 - SampleHeight(normalMap, dp + ds * depth);
		
		if(bestDepth > 0.996)		// if no depth found yet
			if(depth >= t)
				bestDepth = depth;	// store best depth
	}

	depth = bestDepth;
	
	// recurse around first point (depth) for closest match
	for(int i = 0; i < binarySearchSteps; ++i)
	{
		size *= 0.5;

		float t = 1.0 - SampleHeight(normalMap, dp + ds * depth);
		
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

float CalcDiffuse(vec3 N, vec3 L, vec3 E, float NE, float NL, float fzero, float shininess)
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
		B *= max(max(NL, NE), EPSILON);
	}

	return (A + gamma / B) * (1.0 - fzero);
  #else
	return 1.0 - fzero;
  #endif
}

#if defined(USE_SPECULARMAP)
float CalcSpecular(float NH, float NL, float NE, float EH, float fzero, float shininess)
{
  #if defined(USE_BLINN) || defined(USE_TRIACE) || defined(USE_TORRANCE_SPARROW)
	float blinn = pow(NH, shininess);
  #endif

  #if defined(USE_BLINN)
	return blinn;
  #endif

  #if defined(USE_COOK_TORRANCE) || defined (USE_TRIACE) || defined (USE_TORRANCE_SPARROW)
	float fresnel = fzero + (1.0 - fzero) * pow(1.0 - EH, 5);
  #endif

  #if defined(USE_COOK_TORRANCE) || defined(USE_TORRANCE_SPARROW)
	float geo = 2.0 * NH * min(NE, NL);
	geo /= max(EH, geo);
  #endif  

  #if defined(USE_COOK_TORRANCE)
	float m_sq = 2.0 / max(shininess, EPSILON);
	float NH_sq = NH * NH;
	float m_NH_sq = m_sq * NH_sq;
	float beckmann = exp((NH_sq - 1.0) / max(m_NH_sq, EPSILON)) / max(4.0 * m_NH_sq * NH_sq, EPSILON);

	return fresnel * geo * beckmann / max(NE, EPSILON);
  #endif

  #if defined(USE_TRIACE)
	float scale = 0.1248582 * shininess + 0.2691817;

	return fresnel * scale * blinn / max(max(NL, NE), EPSILON);
  #endif
  
  #if defined(USE_TORRANCE_SPARROW)
	float scale = 0.125 * shininess + 1.0;

	return fresnel * geo * scale * blinn / max(NE, EPSILON);
  #endif
}
#endif

void main()
{
#if !defined(USE_FAST_LIGHT) && (defined(USE_LIGHT) || defined(USE_NORMALMAP))
	vec3 surfNormal = normalize(var_Normal);
#endif

#if defined(USE_DELUXEMAP)
	vec3 worldLight = 2.0 * texture2D(u_DeluxeMap, var_LightTex).xyz - vec3(1.0);
	//worldLight += var_WorldLight * 0.0001;
#elif defined(USE_LIGHT)
	vec3 worldLight = var_WorldLight;
#endif

#if defined(USE_LIGHTMAP)
	vec4 lightSample = texture2D(u_LightMap, var_LightTex).rgba;
  #if defined(RGBE_LIGHTMAP)
	lightSample.rgb *= exp2(lightSample.a * 255.0 - 128.0);
  #endif
	vec3 directedLight = lightSample.rgb;
#elif defined(USE_LIGHT_VECTOR) && !defined(USE_FAST_LIGHT)
  #if defined(USE_INVSQRLIGHT)
	float intensity = 1.0 / dot(worldLight, worldLight);
  #else
	float intensity = clamp((1.0 - dot(worldLight, worldLight) / (u_LightRadius * u_LightRadius)) * 1.07, 0.0, 1.0);
  #endif

	vec3 directedLight = u_DirectedLight * intensity;
	vec3 ambientLight  = u_AmbientLight;

  #if defined(USE_SHADOWMAP)
	vec2 shadowTex = gl_FragCoord.xy * r_FBufScale;
	directedLight *= texture2D(u_ShadowMap, shadowTex).r;
  #endif
#elif defined(USE_LIGHT_VERTEX) && !defined(USE_FAST_LIGHT)
	vec3 directedLight = var_VertLight;
#endif
	
#if defined(USE_TCGEN) || defined(USE_NORMALMAP) || (defined(USE_LIGHT) && !defined(USE_FAST_LIGHT))
	vec3 SampleToView = normalize(var_SampleToView);
#endif
	vec2 tex = var_DiffuseTex;

	float ambientDiff = 1.0;

#if defined(USE_NORMALMAP)
  #if defined(USE_VERT_TANGENT_SPACE)
    vec3   tangent = var_Tangent;
	vec3 bitangent = var_Bitangent;
  #else
	vec3 q0  = dFdx(var_Position);
	vec3 q1  = dFdy(var_Position);
	vec2 st0 = dFdx(tex);
	vec2 st1 = dFdy(tex);
	float dir = sign(st1.t * st0.s - st0.t * st1.s);

	vec3   tangent = normalize( q0 * st1.t - q1 * st0.t) * dir;
	vec3 bitangent = -normalize( q0 * st1.s - q1 * st0.s) * dir;
  #endif

	mat3 tangentToWorld = mat3(tangent, bitangent, var_Normal);

  #if defined(USE_PARALLAXMAP)
	vec3 offsetDir = normalize(SampleToView * tangentToWorld);
    #if 0
    float height = SampleHeight(u_NormalMap, tex);
	float pdist = 0.05 * height - (0.05 / 2.0);
    #else
	offsetDir.xy *= -0.05 / offsetDir.z;
	float pdist = RayIntersectDisplaceMap(tex, offsetDir.xy, u_NormalMap);
    #endif	
	tex += offsetDir.xy * pdist;
  #endif
  #if defined(SWIZZLE_NORMALMAP)
	vec3 normal = 2.0 * texture2D(u_NormalMap, tex).agb - 1.0;
  #else
	vec3 normal = 2.0 * texture2D(u_NormalMap, tex).rgb - 1.0;
  #endif
	normal.z = sqrt(clamp(1.0 - dot(normal.xy, normal.xy), 0.0, 1.0));
	vec3 worldNormal = tangentToWorld * normal;
  #if defined(r_normalAmbient)
	ambientDiff = 0.781341 * normal.z + 0.218659;
  #endif
#elif defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)
	vec3 worldNormal = surfNormal;
#endif

#if (defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)) || (defined(USE_TCGEN) && defined(USE_NORMALMAP))
	worldNormal = normalize(worldNormal);
#endif

#if defined(USE_TCGEN) && defined(USE_NORMALMAP)
	if (u_TCGen0 == TCGEN_ENVIRONMENT_MAPPED)
	{
		tex = -reflect(normalize(SampleToView), worldNormal).yz * vec2(0.5, -0.5) + 0.5;
	}
#endif

	vec4 diffuse = texture2D(u_DiffuseMap, tex);

#if defined(USE_LIGHT) && defined(USE_FAST_LIGHT)
  #if defined(USE_LIGHTMAP)
	diffuse.rgb *= directedLight;
  #endif
#elif defined(USE_LIGHT)
	worldLight = normalize(worldLight);

  #if defined(USE_LIGHTMAP) || defined(USE_LIGHT_VERTEX)
	#if defined(r_normalAmbient)
	vec3 ambientLight = directedLight * r_normalAmbient;
	directedLight -= ambientLight;
    #else
	vec3 ambientLight = vec3(0.0);
    #endif
	directedLight /= max(dot(surfNormal, worldLight), 0.004);
  #endif

	float NL = clamp(dot(worldNormal,  worldLight),   0.0, 1.0);
	float surfNL = clamp(dot(surfNormal,  worldLight),   0.0, 1.0);
	NL = min(NL, surfNL * 2.0);
	float NE = clamp(dot(worldNormal,  SampleToView), 0.0, 1.0);
	
	float fzero = u_MaterialInfo.x;
	float shininess = u_MaterialInfo.y;
  #if defined(USE_SPECULARMAP)
	vec4 specular = texture2D(u_SpecularMap, tex);
	//specular.rgb = clamp(specular.rgb - diffuse.rgb, 0.0, 1.0);
	shininess *= specular.a;
  #endif
	float directedDiff = NL * CalcDiffuse(worldNormal, worldLight, SampleToView, NE, NL, fzero, shininess);
	diffuse.rgb *= directedLight * directedDiff + ambientDiff * ambientLight;
  
  #if defined(USE_SPECULARMAP)
	vec3 halfAngle = normalize(worldLight + SampleToView);

	float EH = clamp(dot(SampleToView, halfAngle), 0.0, 1.0);
	float NH = clamp(dot(worldNormal,  halfAngle), 0.0, 1.0);

	float directedSpec = NL * CalcSpecular(NH, NL, NE, EH, fzero, shininess);
  
    #if defined(r_normalAmbient)
	vec3 ambientHalf = normalize(surfNormal + SampleToView);
	float ambientSpec = max(dot(ambientHalf, worldNormal) + 0.5, 0.0);
	ambientSpec *= ambientSpec * 0.44;
	ambientSpec = pow(ambientSpec, shininess) * fzero;
	specular.rgb *= directedSpec * directedLight + ambientSpec * ambientLight;
    #else
	specular.rgb *= directedSpec * directedLight;
    #endif
  #endif
#endif

	gl_FragColor = diffuse;

#if defined(USE_SPECULARMAP) && defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)
	gl_FragColor.rgb += specular.rgb;
#endif

	gl_FragColor *= var_Color;
}
