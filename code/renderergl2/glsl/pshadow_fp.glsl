uniform sampler2D u_ShadowMap;

uniform vec3      u_LightForward;
uniform vec3      u_LightUp;
uniform vec3      u_LightRight;
uniform vec4      u_LightOrigin;
uniform float     u_LightRadius;
varying vec3      var_Position;
varying vec3      var_Normal;

void main()
{
	vec3 lightToPos = var_Position - u_LightOrigin.xyz;
	vec2 st = vec2(-dot(u_LightRight, lightToPos), dot(u_LightUp, lightToPos));
	
	float fade = length(st);
	
#if defined(USE_DISCARD)
	if (fade >= 1.0)
	{
		discard;
	}
#endif

	fade = clamp(8.0 - fade * 8.0, 0.0, 1.0);
	
	st = st * 0.5 + vec2(0.5);

#if defined(USE_SOLID_PSHADOWS)
	float intensity = max(sign(u_LightRadius - length(lightToPos)), 0.0);
#else
	float intensity = clamp((1.0 - dot(lightToPos, lightToPos) / (u_LightRadius * u_LightRadius)) * 2.0, 0.0, 1.0);
#endif
	
	float lightDist = length(lightToPos);
	float dist;

#if defined(USE_DISCARD)
	if (dot(u_LightForward, lightToPos) <= 0.0)
	{
		discard;
	}

	if (dot(var_Normal, lightToPos) > 0.0)
	{
		discard;
	}
#else
	intensity *= max(sign(dot(u_LightForward, lightToPos)), 0.0);
	intensity *= max(sign(-dot(var_Normal, lightToPos)), 0.0);
#endif

	intensity *= fade;

	float part;
#if defined(USE_PCF)
	part  = float(texture2D(u_ShadowMap, st + vec2(-1.0/512.0, -1.0/512.0)).r != 1.0);
	part += float(texture2D(u_ShadowMap, st + vec2( 1.0/512.0, -1.0/512.0)).r != 1.0);
	part += float(texture2D(u_ShadowMap, st + vec2(-1.0/512.0,  1.0/512.0)).r != 1.0);
	part += float(texture2D(u_ShadowMap, st + vec2( 1.0/512.0,  1.0/512.0)).r != 1.0);
#else
	part  = float(texture2D(u_ShadowMap, st).r != 1.0);
#endif

	if (part <= 0.0)
	{
		discard;
	}

#if defined(USE_PCF)
	intensity *= part * 0.25;
#else
	intensity *= part;
#endif

	gl_FragColor.rgb = vec3(0);
	gl_FragColor.a = clamp(intensity, 0.0, 0.75);
}
