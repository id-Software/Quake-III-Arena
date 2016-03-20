uniform sampler2D u_TextureMap;

uniform vec4      u_Color;

uniform vec2      u_InvTexRes;
varying vec2      var_TexCoords;

void main()
{
	vec4 color;
	vec2 tc;

#if 0
	float c[7] = float[7](1.0, 0.9659258263, 0.8660254038, 0.7071067812, 0.5, 0.2588190451, 0.0);

	tc = var_TexCoords + u_InvTexRes * vec2(  c[0],  c[6]);  color =  texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[1],  c[5]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[2],  c[4]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[3],  c[3]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[4],  c[2]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[5],  c[1]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[6],  c[0]);  color += texture2D(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2(  c[1], -c[5]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[2], -c[4]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[3], -c[3]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[4], -c[2]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[5], -c[1]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[6], -c[0]);  color += texture2D(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2( -c[0],  c[6]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[1],  c[5]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[2],  c[4]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[3],  c[3]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[4],  c[2]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[5],  c[1]);  color += texture2D(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2( -c[1], -c[5]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[2], -c[4]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[3], -c[3]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[4], -c[2]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[5], -c[1]);  color += texture2D(u_TextureMap, tc);
	
	gl_FragColor = color * 0.04166667 * u_Color;
#endif

	float c[5] = float[5](1.0, 0.9238795325, 0.7071067812, 0.3826834324, 0.0);

	tc = var_TexCoords + u_InvTexRes * vec2(  c[0],  c[4]);  color =  texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[1],  c[3]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[2],  c[2]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[3],  c[1]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[4],  c[0]);  color += texture2D(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2(  c[1], -c[3]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[2], -c[2]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[3], -c[1]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(  c[4], -c[0]);  color += texture2D(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2( -c[0],  c[4]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[1],  c[3]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[2],  c[2]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[3],  c[1]);  color += texture2D(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2( -c[1], -c[3]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[2], -c[2]);  color += texture2D(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( -c[3], -c[1]);  color += texture2D(u_TextureMap, tc);
	
	gl_FragColor = color * 0.0625 * u_Color;
}
