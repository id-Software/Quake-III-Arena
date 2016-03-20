uniform sampler2D u_DiffuseMap;

varying vec2      var_Tex1;
varying vec4      var_Color;


void main()
{
	vec4 color = texture2D(u_DiffuseMap, var_Tex1);

	gl_FragColor = color * var_Color;
}
