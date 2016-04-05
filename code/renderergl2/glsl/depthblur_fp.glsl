uniform sampler2D u_ScreenImageMap;
uniform sampler2D u_ScreenDepthMap;

uniform vec4   u_ViewInfo; // zfar / znear, zfar, 1/width, 1/height
varying vec2   var_ScreenTex;

//float gauss[8] = float[8](0.17, 0.17, 0.16, 0.14, 0.12, 0.1, 0.08, 0.06);
//float gauss[5] = float[5](0.30, 0.23, 0.097, 0.024, 0.0033);
float gauss[4] = float[4](0.40, 0.24, 0.054, 0.0044);
//float gauss[3] = float[3](0.60, 0.19, 0.0066);
#define BLUR_SIZE 4

#if !defined(USE_DEPTH)
//#define USE_GAUSS
#endif

float getLinearDepth(sampler2D depthMap, const vec2 tex, const float zFarDivZNear)
{
	float sampleZDivW = texture2D(depthMap, tex).r;
	return 1.0 / mix(zFarDivZNear, 1.0, sampleZDivW);
}

vec4 depthGaussian1D(sampler2D imageMap, sampler2D depthMap, vec2 tex, float zFarDivZNear, float zFar, vec2 scale)
{

#if defined(USE_DEPTH)
	float depthCenter = getLinearDepth(depthMap, tex, zFarDivZNear);
	vec2 slope = vec2(dFdx(depthCenter), dFdy(depthCenter)) / vec2(dFdx(tex.x), dFdy(tex.y));
	scale /= clamp(zFarDivZNear * depthCenter / 32.0, 1.0, 2.0);
#endif

#if defined(USE_HORIZONTAL_BLUR)
	vec2 direction = vec2(scale.x * 2.0, 0.0);
	vec2 nudge = vec2(0.0, scale.y * 0.5);
#else // if defined(USE_VERTICAL_BLUR)
	vec2 direction = vec2(0.0, scale.y * 2.0);
	vec2 nudge = vec2(-scale.x * 0.5, 0.0);
#endif

#if defined(USE_GAUSS)
	vec4 result = texture2D(imageMap, tex) * gauss[0];
	float total = gauss[0];
#else
	vec4 result = texture2D(imageMap, tex);
	float total = 1.0;
#endif

	float zLimit = 5.0 / zFar;
	int i, j;
	for (i = 0; i < 2; i++)
	{
		for (j = 1; j < BLUR_SIZE; j++)
		{
			vec2 offset = direction * (float(j) - 0.25) + nudge;
#if defined(USE_DEPTH)
			float depthSample = getLinearDepth(depthMap, tex + offset, zFarDivZNear);
			float depthExpected = depthCenter + dot(slope, offset);
			float useSample = float(abs(depthSample - depthExpected) < zLimit);
#else
			float useSample = 1.0;
#endif
#if defined(USE_GAUSS)
			result += texture2D(imageMap, tex + offset) * (gauss[j] * useSample);
			total += gauss[j] * useSample;
#else
			result += texture2D(imageMap, tex + offset) * useSample;
			total += useSample;
#endif
			nudge = -nudge;
		}

		direction = -direction;
		nudge = -nudge;
	}

	return result / total;
}

void main()
{
	gl_FragColor = depthGaussian1D(u_ScreenImageMap, u_ScreenDepthMap, var_ScreenTex, u_ViewInfo.x, u_ViewInfo.y, u_ViewInfo.zw);
}
