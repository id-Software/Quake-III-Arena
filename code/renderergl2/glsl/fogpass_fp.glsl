uniform vec4  u_Color;

varying float var_Scale;

void main()
{
	gl_FragColor = u_Color;
	gl_FragColor.a = sqrt(clamp(var_Scale, 0.0, 1.0));
}
