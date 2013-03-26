#version 120

attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;

uniform mat4   u_ModelViewProjectionMatrix;

varying vec2   var_Tex1;


void main()
{
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;
	var_Tex1 = attr_TexCoord0.st;
}
