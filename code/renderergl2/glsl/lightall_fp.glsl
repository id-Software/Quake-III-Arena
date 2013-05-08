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
uniform vec4      u_LightOrigin;
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

#if defined(USE_NORMALMAP) && !defined(USE_VERT_TANGENT_SPACE)
varying vec3      var_Position;
#endif

#if defined(USE_TCGEN) || defined(USE_NORMALMAP) || (defined(USE_LIGHT) && !defined(USE_FAST_LIGHT))
varying vec3      var_SampleToView;
#endif

#if !defined(USE_FAST_LIGHT)
varying vec3      var_Normal;
#endif

#if defined(USE_VERT_TANGENT_SPACE)
varying vec3      var_Tangent;
varying vec3      var_Bitangent;
#endif

varying vec3      var_VertLight;

#if defined(USE_LIGHT) && !defined(USE_DELUXEMAP)
varying vec3      var_LightDirection;
#endif

#if defined(USE_PRIMARY_LIGHT) || defined(USE_SHADOWMAP)
varying vec3      var_PrimaryLightDirection;
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
		B *= max(max(NL, NE), EPSILON);
	}

	return diffuseAlbedo * (A + gamma / B);
  #else
	return diffuseAlbedo;
  #endif
}

#if defined(USE_SPECULARMAP)
vec3 CalcSpecular(vec3 specularReflectance, float NH, float NL, float NE, float EH, float shininess)
{
  #if defined(USE_BLINN) || defined(USE_TRIACE) || defined(USE_TORRANCE_SPARROW)
	float blinn = pow(NH, shininess);
  #endif

  #if defined(USE_BLINN)
	return specularReflectance * blinn;
  #endif

  #if defined(USE_COOK_TORRANCE) || defined (USE_TRIACE) || defined (USE_TORRANCE_SPARROW)
	vec3 fresnel = specularReflectance + (vec3(1.0) - specularReflectance) * pow(1.0 - EH, 5);
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
	vec3 surfN = normalize(var_Normal);
#endif

#if defined(USE_DELUXEMAP)
	vec3 L = 2.0 * texture2D(u_DeluxeMap, var_LightTex).xyz - vec3(1.0);
	//L += var_LightDirection * 0.0001;
#elif defined(USE_LIGHT)
	vec3 L = var_LightDirection;
#endif

#if defined(USE_LIGHTMAP)
	vec4 lightSample = texture2D(u_LightMap, var_LightTex).rgba;
  #if defined(RGBE_LIGHTMAP)
	lightSample.rgb *= exp2(lightSample.a * 255.0 - 128.0);
  #endif
	vec3 lightColor = lightSample.rgb;
#elif defined(USE_LIGHT_VECTOR) && !defined(USE_FAST_LIGHT)
  #if defined(USE_INVSQRLIGHT)
	float intensity = 1.0 / dot(L, L);
  #else
	float intensity = clamp((1.0 - dot(L, L) / (u_LightRadius * u_LightRadius)) * 1.07, 0.0, 1.0);
  #endif

	vec3 lightColor = u_DirectedLight * intensity;
	vec3 ambientColor  = u_AmbientLight;
#elif defined(USE_LIGHT_VERTEX) && !defined(USE_FAST_LIGHT)
	vec3 lightColor = var_VertLight;
#endif
	
#if defined(USE_TCGEN) || defined(USE_NORMALMAP) || (defined(USE_LIGHT) && !defined(USE_FAST_LIGHT))
	vec3 E = normalize(var_SampleToView);
#endif
	vec2 texCoords = var_DiffuseTex;

	float ambientDiff = 1.0;

#if defined(USE_NORMALMAP)
  #if defined(USE_VERT_TANGENT_SPACE)
	mat3 tangentToWorld = mat3(var_Tangent, var_Bitangent, var_Normal);
  #else
	vec3 q0  = dFdx(var_Position);
	vec3 q1  = dFdy(var_Position);
	vec2 st0 = dFdx(texCoords);
	vec2 st1 = dFdy(texCoords);
	float dir = sign(st1.t * st0.s - st0.t * st1.s);

	vec3   tangent =  normalize(q0 * st1.t - q1 * st0.t) * dir;
	vec3 bitangent = -normalize(q0 * st1.s - q1 * st0.s) * dir;

	mat3 tangentToWorld = mat3(tangent, bitangent, var_Normal);
  #endif

  #if defined(USE_PARALLAXMAP)
	vec3 offsetDir = normalize(E * tangentToWorld);
	offsetDir.xy *= -0.05 / offsetDir.z;

	texCoords += offsetDir.xy * RayIntersectDisplaceMap(texCoords, offsetDir.xy, u_NormalMap);
  #endif
	vec3 texN;
  #if defined(SWIZZLE_NORMALMAP)
	texN.xy = 2.0 * texture2D(u_NormalMap, texCoords).ag - 1.0;
  #else
	texN.xy = 2.0 * texture2D(u_NormalMap, texCoords).rg - 1.0;
  #endif
	texN.z = sqrt(clamp(1.0 - dot(texN.xy, texN.xy), 0.0, 1.0));
	vec3 N = tangentToWorld * texN;
  #if defined(r_normalAmbient)
	ambientDiff = 0.781341 * texN.z + 0.218659;
  #endif
#elif defined(USE_LIGHT) && !defined(USE_FAST_LIGHT) 
	vec3 N = surfN;
#endif

#if (defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)) || (defined(USE_TCGEN) && defined(USE_NORMALMAP))
	N = normalize(N);
#endif

#if defined(USE_TCGEN) && defined(USE_NORMALMAP)
	if (u_TCGen0 == TCGEN_ENVIRONMENT_MAPPED)
	{
		texCoords = -reflect(E, N).yz * vec2(0.5, -0.5) + 0.5;
	}
#endif

	vec4 diffuseAlbedo = texture2D(u_DiffuseMap, texCoords);
#if defined(USE_LIGHT) && defined(USE_GAMMA2_TEXTURES)
	diffuseAlbedo.rgb *= diffuseAlbedo.rgb;
#endif

#if defined(USE_LIGHT) && defined(USE_FAST_LIGHT)
	gl_FragColor = diffuse.rgb;
  #if defined(USE_LIGHTMAP) 
	gl_FragColor *= lightColor;
  #endif
#elif defined(USE_LIGHT)
	L = normalize(L);

	float surfNL = clamp(dot(surfN,  L),   0.0, 1.0);
	
  #if defined(USE_SHADOWMAP) 
	vec2 shadowTex = gl_FragCoord.xy * r_FBufScale;
	float shadowValue = texture2D(u_ShadowMap, shadowTex).r;

	// surfaces not facing the light are always shadowed
	shadowValue *= step(0.0, dot(surfN, var_PrimaryLightDirection));
  
    #if defined(SHADOWMAP_MODULATE)
	//vec3 shadowColor = min(u_PrimaryLightAmbient, lightColor);
	vec3 shadowColor = u_PrimaryLightAmbient * lightColor;

      #if 0
	// Only shadow when the world light is parallel to the primary light
	shadowValue = 1.0 + (shadowValue - 1.0) * clamp(dot(L, var_PrimaryLightDirection), 0.0, 1.0);
      #endif
	lightColor = mix(shadowColor, lightColor, shadowValue);
    #endif
  #endif

  #if defined(USE_LIGHTMAP) || defined(USE_LIGHT_VERTEX)
    #if defined(USE_STANDARD_DELUXEMAP)
	// Standard deluxe mapping treats the light sample as fully directed
	// and doesn't compensate for light angle attenuation.
	vec3 ambientColor = vec3(0.0);
    #else
	// Separate the light sample into directed and ambient parts.
	//
	// ambientMax  - if the cosine of the angle between the surface
	//               normal and the light is below this value, the light
	//               is fully ambient.
	// directedMax - if the cosine of the angle between the surface
	//               normal and the light is above this value, the light
	//               is fully directed.
	const float ambientMax  = 0.25;
	const float directedMax = 0.5;

	float directedScale = clamp((surfNL - ambientMax) / (directedMax - ambientMax), 0.0, 1.0);
	
	// Scale the directed portion to compensate for the baked-in
	// light angle attenuation.
	directedScale /= max(surfNL, ambientMax);
	
      #if defined(r_normalAmbient)
	directedScale *= 1.0 - r_normalAmbient;
      #endif

	// Recover any unused light as ambient
	vec3 ambientColor = lightColor;
	lightColor *= directedScale;
	ambientColor -= lightColor * surfNL;
    #endif
  #endif

	float NL = clamp(dot(N, L), 0.0, 1.0);
	float NE = clamp(dot(N, E), 0.0, 1.0);

	float maxReflectance = u_MaterialInfo.x;
	float shininess = u_MaterialInfo.y;

  #if defined(USE_SPECULARMAP)
	vec4 specularReflectance = texture2D(u_SpecularMap, texCoords);
	specularReflectance.rgb *= maxReflectance;
	shininess *= specularReflectance.a;
	// adjust diffuse by specular reflectance, to maintain energy conservation
	diffuseAlbedo.rgb *= vec3(1.0) - specularReflectance.rgb;
  #endif

	gl_FragColor.rgb = lightColor * NL * CalcDiffuse(diffuseAlbedo.rgb, N, L, E, NE, NL, shininess);
	gl_FragColor.rgb += ambientDiff * ambientColor * diffuseAlbedo.rgb;
  #if defined(USE_PRIMARY_LIGHT)
	vec3 L2 = var_PrimaryLightDirection;
	float NL2 = clamp(dot(N, L2), 0.0, 1.0);

    #if defined(USE_SHADOWMAP)
	gl_FragColor.rgb += u_PrimaryLightColor * shadowValue * NL2 * CalcDiffuse(diffuseAlbedo.rgb, N, L2, E, NE, NL2, shininess);
    #else
	gl_FragColor.rgb += u_PrimaryLightColor * NL2 * CalcDiffuse(diffuseAlbedo.rgb, N, L2, E, NE, NL2, shininess);
    #endif
  #endif
  
  #if defined(USE_SPECULARMAP)
	vec3 H = normalize(L + E);

	float EH = clamp(dot(E, H), 0.0, 1.0);
	float NH = clamp(dot(N, H), 0.0, 1.0);

	gl_FragColor.rgb += lightColor * NL * CalcSpecular(specularReflectance.rgb, NH, NL, NE, EH, shininess);
  
    #if defined(r_normalAmbient)
	vec3 ambientHalf = normalize(surfN + E);
	float ambientSpec = max(dot(ambientHalf, N) + 0.5, 0.0);
	ambientSpec *= ambientSpec * 0.44;
	gl_FragColor.rgb += specularReflectance.rgb * ambientSpec * ambientColor;
    #endif

    #if defined(USE_PRIMARY_LIGHT)
	vec3 H2 = normalize(L2 + E);
	float EH2 = clamp(dot(E, H2), 0.0, 1.0);
	float NH2 = clamp(dot(N, H2), 0.0, 1.0);


      #if defined(USE_SHADOWMAP)
	gl_FragColor.rgb += u_PrimaryLightColor * shadowValue * NL2 * CalcSpecular(specularReflectance.rgb, NH2, NL2, NE, EH2, shininess);
      #else
	gl_FragColor.rgb += u_PrimaryLightColor * NL2 * CalcSpecular(specularReflectance.rgb, NH2, NL2, NE, EH2, shininess);
      #endif
    #endif
  #endif  
#else
	gl_FragColor.rgb = diffuseAlbedo.rgb;
#endif

	gl_FragColor.a = diffuseAlbedo.a;

	gl_FragColor *= var_Color;
}
