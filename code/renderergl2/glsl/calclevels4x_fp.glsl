uniform sampler2D u_TextureMap;

uniform vec4      u_Color;

uniform vec2      u_InvTexRes;
varying vec2      var_TexCoords;

const vec3  LUMINANCE_VECTOR =   vec3(0.2125, 0.7154, 0.0721); //vec3(0.299, 0.587, 0.114);

vec3 GetValues(vec2 offset, vec3 current)
{
	vec2 tc = var_TexCoords + u_InvTexRes * offset;
	vec3 minAvgMax = texture2D(u_TextureMap, tc).rgb;

#ifdef FIRST_PASS

  #if defined(USE_PBR)
	minAvgMax *= minAvgMax;
  #endif

	float lumi = max(dot(LUMINANCE_VECTOR, minAvgMax), 0.000001);
	float loglumi = clamp(log2(lumi), -10.0, 10.0);
	minAvgMax = vec3(loglumi * 0.05 + 0.5);
#endif

	return vec3(min(current.x, minAvgMax.x), current.y + minAvgMax.y, max(current.z, minAvgMax.z));
}

void main()
{
	vec3 current = vec3(1.0, 0.0, 0.0);

#ifdef FIRST_PASS
	current = GetValues(vec2( 0.0,  0.0), current);
#else
	current = GetValues(vec2(-1.5, -1.5), current);
	current = GetValues(vec2(-0.5, -1.5), current);
	current = GetValues(vec2( 0.5, -1.5), current);
	current = GetValues(vec2( 1.5, -1.5), current);
	
	current = GetValues(vec2(-1.5, -0.5), current);
	current = GetValues(vec2(-0.5, -0.5), current);
	current = GetValues(vec2( 0.5, -0.5), current);
	current = GetValues(vec2( 1.5, -0.5), current);
	
	current = GetValues(vec2(-1.5,  0.5), current);
	current = GetValues(vec2(-0.5,  0.5), current);
	current = GetValues(vec2( 0.5,  0.5), current);
	current = GetValues(vec2( 1.5,  0.5), current);

	current = GetValues(vec2(-1.5,  1.5), current);
	current = GetValues(vec2(-0.5,  1.5), current);
	current = GetValues(vec2( 0.5,  1.5), current);
	current = GetValues(vec2( 1.5,  1.5), current);

	current.y *= 0.0625;
#endif

	gl_FragColor = vec4(current, 1.0f);
}
