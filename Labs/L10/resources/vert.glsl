#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat4 invTransposeMV; // Uniform for inverse transpose of MV
attribute vec4 aPos; // In object space
attribute vec3 aNor; // In object space
attribute vec2 aTex;

varying vec3 vNor; // In camera space
varying vec2 vTex;

void main()
{
	gl_Position = P * (MV * aPos);
    
    vNor = normalize(mat3(invTransposeMV) * aNor); // Assuming MV contains only translations and rotations
	vTex = aTex;
}
