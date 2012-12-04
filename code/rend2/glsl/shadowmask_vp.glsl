attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;

uniform vec3   u_ViewForward;
uniform vec3   u_ViewLeft;
uniform vec3   u_ViewUp;
uniform vec4   u_ViewInfo; // zfar / znear

varying vec2   var_ScreenTex;
varying vec3   var_ViewDir;

void main()
{
	gl_Position = attr_Position;
	//vec2 screenCoords = gl_Position.xy / gl_Position.w;
	//var_ScreenTex = screenCoords * 0.5 + 0.5;
	var_ScreenTex = attr_TexCoord0.xy;
	vec2 screenCoords = attr_TexCoord0.xy * 2.0 - 1.0;
	var_ViewDir = u_ViewForward + u_ViewLeft * -screenCoords.x + u_ViewUp * screenCoords.y;
}
