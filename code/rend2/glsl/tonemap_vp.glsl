attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;

uniform mat4   u_ModelViewProjectionMatrix;

varying vec2   var_TexCoords;


void main()
{
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;
	var_TexCoords = attr_TexCoord0.st;
}
