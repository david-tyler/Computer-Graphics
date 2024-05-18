#version 120


varying vec3 fragPos; // position in camera space
varying vec3 normalCam; // passed from the vertex shader, normal in camera space

// in camera space the camera is at the origin

void main()
{
    vec3 n = normalize(normalCam);
    vec3 viewDirCam = normalize(-fragPos);

    float dotProduct = abs(dot(n, viewDirCam));
    
    if (dotProduct < 0.3) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0); // Black color for silhouette
    } else {
        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0); // White color for non-silhouette
    }
}
