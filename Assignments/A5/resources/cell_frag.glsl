#version 120

uniform vec3 lightPos; // in camera coordinates

uniform vec3 light1position;
uniform vec3 light1color;


uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;


varying vec3 fragPos; // passed from the vertex shader
varying vec3 normalCam; // passed from the vertex shader



void main()
{
    vec3 n = normalize(normalCam);
    
    vec3 viewDirCam = normalize(-fragPos);
    
    vec3 ca = ka; // Compute diffuse color
    
    
    // work with lights
    vec3 result = vec3(0.0f, 0.0f, 0.0f);
    
    // first light
    vec3 lightDirCam = normalize(light1position - fragPos); // Calculate light direction in camera space
    float diffuse = max(0.0, dot(lightDirCam, n)); // Compute diffuse component
    vec3 cd = kd * diffuse; // Compute diffuse color
    
    vec3 h = normalize(lightDirCam + viewDirCam);
    float specular = pow(max(0.0, dot(h, n)), s); // Compute specular component
    vec3 cs = ks * specular; // Compute specular color
    
    // R
    result += light1color*(ca + cd + cs);
    
    
    vec3 color = ca + cd + cs;

    gl_FragColor = vec4(result, 1.0);
}
