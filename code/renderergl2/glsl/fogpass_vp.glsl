attribute vec4  attr_Position;
attribute vec3  attr_Normal;
attribute vec4  attr_TexCoord0;

//#if defined(USE_VERTEX_ANIMATION)
attribute vec4  attr_Position2;
attribute vec3  attr_Normal2;
//#endif

uniform vec4    u_FogDistance;
uniform vec4    u_FogDepth;
uniform float   u_FogEyeT;

//#if defined(USE_DEFORM_VERTEXES)
uniform int     u_DeformGen;
uniform float   u_DeformParams[5];
//#endif

uniform float   u_Time;
uniform mat4    u_ModelViewProjectionMatrix;

//#if defined(USE_VERTEX_ANIMATION)
uniform float   u_VertexLerp;
//#endif

varying float   var_Scale;

#if defined(USE_DEFORM_VERTEXES)
vec3 DeformPosition(const vec3 pos, const vec3 normal, const vec2 st)
{
	if (u_DeformGen == 0)
	{
		return pos;
	}

	float base =      u_DeformParams[0];
	float amplitude = u_DeformParams[1];
	float phase =     u_DeformParams[2];
	float frequency = u_DeformParams[3];
	float spread =    u_DeformParams[4];

	if (u_DeformGen == DGEN_BULGE)
	{
		phase *= M_PI * 0.25 * st.x;
	}
	else // if (u_DeformGen <= DGEN_WAVE_INVERSE_SAWTOOTH)
	{
		phase += dot(pos.xyz, vec3(spread));
	}

	float value = phase + (u_Time * frequency);
	float func;

	if (u_DeformGen == DGEN_WAVE_SIN)
	{
		func = sin(value * 2.0 * M_PI);
	}
	else if (u_DeformGen == DGEN_WAVE_SQUARE)
	{
		func = sign(sin(value * 2.0 * M_PI));
	}
	else if (u_DeformGen == DGEN_WAVE_TRIANGLE)
	{
		func = abs(fract(value + 0.75) - 0.5) * 4.0 - 1.0;
	}
	else if (u_DeformGen == DGEN_WAVE_SAWTOOTH)
	{
		func = fract(value);
	}
	else if (u_DeformGen == DGEN_WAVE_INVERSE_SAWTOOTH)
	{
		func = (1.0 - fract(value));
	}
	else if (u_DeformGen == DGEN_BULGE)
	{
		func = sin(value);
	}

	return pos + normal * (base + func * amplitude);
}
#endif

float CalcFog(vec4 position)
{
	float s = dot(position, u_FogDistance) * 8.0;
	float t = dot(position, u_FogDepth);

	if (t < 1.0)
	{
		t = step(step(0.0, -u_FogEyeT), t);
	}
	else
	{
		t /= t - min(u_FogEyeT, 0.0);
	}
	
	return s * t;
}

void main()
{
#if defined(USE_VERTEX_ANIMATION)
	vec4 position = mix(attr_Position, attr_Position2, u_VertexLerp);
	vec3 normal = normalize(mix(attr_Normal, attr_Normal2, u_VertexLerp));
#else
	vec4 position = attr_Position;
	vec3 normal = attr_Normal;
#endif

#if defined(USE_DEFORM_VERTEXES)
	position.xyz = DeformPosition(position.xyz, normal, attr_TexCoord0.st);
#endif

	gl_Position = u_ModelViewProjectionMatrix * position;

	var_Scale = CalcFog(position);
}
