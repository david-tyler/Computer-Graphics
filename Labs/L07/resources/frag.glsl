#version 120

uniform vec3 lightPos; // in camera coordinates
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;


varying vec3 fragPos; // passed from the vertex shader
varying vec3 normalCam; // passed from the vertex shader


void main()
{
    vec3 n = normalize(normalCam);
    vec3 viewDirCam = normalize(-fragPos); // Calculate view direction in camera space
    vec3 ca = vec3(0.2, 0.2, 0.2); // Compute diffuse color

    vec3 lightDirCam = normalize(lightPos - fragPos); // Calculate light direction in camera space
    float diffuse = max(0.0, dot(lightDirCam, n)); // Compute diffuse component
    vec3 cd = kd * diffuse; // Compute diffuse color
        
    vec3 h = normalize(lightDirCam + viewDirCam);
    
    float specular = pow(max(0.0, dot(h, n)), s); // Compute specular component
    
    vec3 cs = ks * specular; // Compute specular color
    vec3 color = ca + cd + cs;

    gl_FragColor = vec4(color.r, color.g, color.b, 1.0);
}
