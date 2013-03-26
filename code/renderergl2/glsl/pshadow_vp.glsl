attribute vec4 attr_Position;
attribute vec3 attr_Normal;

uniform mat4   u_ModelViewProjectionMatrix;
varying vec3   var_Position;
varying vec3   var_Normal;


void main()
{
	vec4 position  = attr_Position;

	gl_Position = u_ModelViewProjectionMatrix * position;

	var_Position  = position.xyz;
	var_Normal    = attr_Normal;
}
