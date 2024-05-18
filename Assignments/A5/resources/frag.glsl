#version 120

uniform vec3 newLightsColors[70];
uniform vec3 newLightsPos[70];
uniform float t;
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;


varying vec3 fragPos; // passed from the vertex shader
varying vec3 normalCam; // passed from the vertex shader



void main()
{    
    gl_FragData[0].xyz = fragPos;
    gl_FragData[1].xyz = normalCam;
    gl_FragData[2].xyz = ka;
    gl_FragData[3].xyz = kd;
    
    
}


