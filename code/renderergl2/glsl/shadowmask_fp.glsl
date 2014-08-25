uniform sampler2D u_ScreenDepthMap;

uniform sampler2DShadow u_ShadowMap;
#if defined(USE_SHADOW_CASCADE)
uniform sampler2DShadow u_ShadowMap2;
uniform sampler2DShadow u_ShadowMap3;
uniform sampler2DShadow u_ShadowMap4;
#endif

uniform mat4      u_ShadowMvp;
#if defined(USE_SHADOW_CASCADE)
uniform mat4      u_ShadowMvp2;
uniform mat4      u_ShadowMvp3;
uniform mat4      u_ShadowMvp4;
#endif

uniform vec3   u_ViewOrigin;
uniform vec4   u_ViewInfo; // zfar / znear, zfar

varying vec2   var_DepthTex;
varying vec3   var_ViewDir;

// depth is GL_DEPTH_COMPONENT24
// so the maximum error is 1.0 / 2^24
#define DEPTH_MAX_ERROR 0.000000059604644775390625

// Input: It uses texture coords as the random number seed.
// Output: Random number: [0,1), that is between 0.0 and 0.999999... inclusive.
// Author: Michael Pohoreski
// Copyright: Copyleft 2012 :-)
// Source: http://stackoverflow.com/questions/5149544/can-i-generate-a-random-number-inside-a-pixel-shader

float random( const vec2 p )
{
  // We need irrationals for pseudo randomness.
  // Most (all?) known transcendental numbers will (generally) work.
  const vec2 r = vec2(
    23.1406926327792690,  // e^pi (Gelfond's constant)
     2.6651441426902251); // 2^sqrt(2) (Gelfond-Schneider constant)
  //return fract( cos( mod( 123456789., 1e-7 + 256. * dot(p,r) ) ) );
  return mod( 123456789., 1e-7 + 256. * dot(p,r) );  
}

float PCF(const sampler2DShadow shadowmap, const vec2 st, const float dist)
{
	float mult;
	float scale = 2.0 / r_shadowMapSize;

#if 0
	// from http://http.developer.nvidia.com/GPUGems/gpugems_ch11.html
	vec2 offset = vec2(greaterThan(fract(var_DepthTex.xy * r_FBufScale * 0.5), vec2(0.25)));
	offset.y += offset.x;
	if (offset.y > 1.1) offset.y = 0.0;
	
	mult = shadow2D(shadowmap, vec3(st + (offset + vec2(-1.5,  0.5)) * scale, dist)).r
	     + shadow2D(shadowmap, vec3(st + (offset + vec2( 0.5,  0.5)) * scale, dist)).r
	     + shadow2D(shadowmap, vec3(st + (offset + vec2(-1.5, -1.5)) * scale, dist)).r
	     + shadow2D(shadowmap, vec3(st + (offset + vec2( 0.5, -1.5)) * scale, dist)).r;
	 
	mult *= 0.25;
#endif

#if defined(USE_SHADOW_FILTER)
	float r = random(var_DepthTex.xy);
	float sinr = sin(r) * scale;
	float cosr = cos(r) * scale;
	mat2 rmat = mat2(cosr, sinr, -sinr, cosr);

	mult =  shadow2D(shadowmap, vec3(st + rmat * vec2(-0.7055767, 0.196515), dist)).r;
	mult += shadow2D(shadowmap, vec3(st + rmat * vec2(0.3524343, -0.7791386), dist)).r;
	mult += shadow2D(shadowmap, vec3(st + rmat * vec2(0.2391056, 0.9189604), dist)).r;
  #if defined(USE_SHADOW_FILTER2)
	mult += shadow2D(shadowmap, vec3(st + rmat * vec2(-0.07580382, -0.09224417), dist)).r;
	mult += shadow2D(shadowmap, vec3(st + rmat * vec2(0.5784913, -0.002528916), dist)).r;
	mult += shadow2D(shadowmap, vec3(st + rmat * vec2(0.192888, 0.4064181), dist)).r;
	mult += shadow2D(shadowmap, vec3(st + rmat * vec2(-0.6335801, -0.5247476), dist)).r;
	mult += shadow2D(shadowmap, vec3(st + rmat * vec2(-0.5579782, 0.7491854), dist)).r;
	mult += shadow2D(shadowmap, vec3(st + rmat * vec2(0.7320465, 0.6317794), dist)).r;

	mult *= 0.11111;
  #else
    mult *= 0.33333;
  #endif
#else
	mult = shadow2D(shadowmap, vec3(st, dist)).r;
#endif

	return mult;
}

float getLinearDepth(sampler2D depthMap, vec2 tex, float zFarDivZNear)
{
	float sampleZDivW = texture2D(depthMap, tex).r - DEPTH_MAX_ERROR;
	return 1.0 / mix(zFarDivZNear, 1.0, sampleZDivW);
}

void main()
{
	float result;

	float depth = getLinearDepth(u_ScreenDepthMap, var_DepthTex, u_ViewInfo.x);
	vec4 biasPos = vec4(u_ViewOrigin + var_ViewDir * (depth - 0.5 / u_ViewInfo.x), 1.0);

	vec4 shadowpos = u_ShadowMvp * biasPos;

#if defined(USE_SHADOW_CASCADE)
	if (all(lessThan(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
	{
#endif
		shadowpos.xyz = shadowpos.xyz * (0.5 / shadowpos.w) + vec3(0.5);
		result = PCF(u_ShadowMap, shadowpos.xy, shadowpos.z);
#if defined(USE_SHADOW_CASCADE)
	}
	else
	{
		shadowpos = u_ShadowMvp2 * biasPos;

		if (all(lessThan(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
		{
			shadowpos.xyz = shadowpos.xyz * (0.5 / shadowpos.w) + vec3(0.5);
			result = PCF(u_ShadowMap2, shadowpos.xy, shadowpos.z);
		}
		else
		{
			shadowpos = u_ShadowMvp3 * biasPos;

			if (all(lessThan(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
			{
				shadowpos.xyz = shadowpos.xyz * (0.5 / shadowpos.w) + vec3(0.5);
				result = PCF(u_ShadowMap3, shadowpos.xy, shadowpos.z);
			}
			else
			{
				shadowpos = u_ShadowMvp4 * biasPos;
				shadowpos.xyz = shadowpos.xyz * (0.5 / shadowpos.w) + vec3(0.5);
				result = PCF(u_ShadowMap4, shadowpos.xy, shadowpos.z);
			}
		}
	}
#endif

	gl_FragColor = vec4(vec3(result), 1.0);
}
