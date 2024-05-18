#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat4 MVit;


attribute vec4 aPos;
attribute vec3 aNor;

// using silhouette shader

varying vec3 fragPos; // in camera space
varying vec3 normalCam; // in camera space

void main()
{
	gl_Position = P * MV * aPos;
    
    // using silhouette shader
    
    fragPos = (MV * aPos).xyz; // Convert position to camera space
    
    vec4 tmp = MVit * vec4(aNor, 0.0); // Convert normal to camera space
    normalCam = normalize(tmp.xyz);
    
}
