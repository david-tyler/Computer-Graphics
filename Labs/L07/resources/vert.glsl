#version 120

uniform mat4 P;
uniform mat4 MV;

attribute vec4 aPos; // in object space
attribute vec3 aNor; // in object space

varying vec3 fragPos; // in camera space
varying vec3 normalCam; // in camera space

void main()
{
	gl_Position = P * MV * aPos;
    fragPos = (MV * aPos).xyz; // Convert position to camera space
    normalCam = (MV * vec4(aNor, 0.0)).xyz; // Convert normal to camera space

}
