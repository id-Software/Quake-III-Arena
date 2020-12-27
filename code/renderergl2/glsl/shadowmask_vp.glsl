attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;

uniform vec3   u_ViewForward;
uniform vec3   u_ViewLeft;
uniform vec3   u_ViewUp;
uniform vec4   u_ViewInfo; // zfar / znear

varying vec2   var_DepthTex;
varying vec3   var_ViewDir;

void main()
{
	gl_Position = attr_Position;
	vec2 screenCoords = gl_Position.xy / gl_Position.w;
	var_DepthTex = attr_TexCoord0.xy;
	var_ViewDir = u_ViewForward + u_ViewLeft * -screenCoords.x + u_ViewUp * screenCoords.y;
}
