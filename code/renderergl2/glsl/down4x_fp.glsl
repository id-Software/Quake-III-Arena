uniform sampler2D u_TextureMap;

uniform vec2      u_InvTexRes;
varying vec2      var_TexCoords;

void main()
{
	vec4 color;
	vec2 tc;
	
	tc = var_TexCoords + u_InvTexRes * vec2(-1.5, -1.5);  color  = texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(-0.5, -1.5);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 0.5, -1.5);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 1.5, -1.5);  color += texture2D(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2(-1.5, -0.5); color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(-0.5, -0.5); color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 0.5, -0.5); color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 1.5, -0.5); color += texture2D(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2(-1.5,  0.5); color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(-0.5,  0.5); color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 0.5,  0.5); color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 1.5,  0.5); color += texture2D(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2(-1.5,  1.5);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(-0.5,  1.5);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 0.5,  1.5);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 1.5,  1.5);  color += texture2D(u_TextureMap, tc);
	
	color *= 0.0625;
	
	gl_FragColor = color;
}
