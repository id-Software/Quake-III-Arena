uniform sampler2D u_DiffuseMap;

varying vec2      var_DiffuseTex;

varying vec4      var_Color;


void main()
{
	vec4 color  = texture2D(u_DiffuseMap, var_DiffuseTex);
	gl_FragColor = color * var_Color;
}
