#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat3 T;

attribute vec4 aPos;
attribute vec3 aNor;
attribute vec2 aTex;

varying vec2 vTex0;

void main()
{
	gl_Position = P * MV * aPos;
	vTex0 = aTex;
}
