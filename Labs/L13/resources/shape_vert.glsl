#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat4 MVit;

attribute vec4 aPos;
attribute vec3 aNor;

varying vec3 color;

void main()
{
	gl_Position = P * MV * aPos;
	vec3 n = mat3(MVit) * aNor;
	n = normalize(n);
	color = abs(n.z) * vec3(0.5, 0.5, 0.5);
}
