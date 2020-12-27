attribute vec3 attr_Position;
attribute vec3 attr_Normal;

uniform mat4   u_ModelViewProjectionMatrix;
varying vec3   var_Position;
varying vec3   var_Normal;


void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

	var_Position  = attr_Position;
	var_Normal    = attr_Normal;
}
