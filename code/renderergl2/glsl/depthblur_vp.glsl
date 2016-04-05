attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;

uniform vec4   u_ViewInfo; // zfar / znear, zfar, 1/width, 1/height

varying vec2   var_ScreenTex;

void main()
{
	gl_Position = attr_Position;
	vec2 wh = vec2(1.0) / u_ViewInfo.zw - vec2(1.0);
	var_ScreenTex = (floor(attr_TexCoord0.xy * wh) + vec2(0.5)) * u_ViewInfo.zw;

	//vec2 screenCoords = gl_Position.xy / gl_Position.w;
	//var_ScreenTex = screenCoords * 0.5 + 0.5;
}
