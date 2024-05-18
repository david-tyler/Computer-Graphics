#version 120

varying vec3 fragPos; // position in camera space
varying vec3 normalCam; // passed from the vertex shader, normal in camera space

// in camera space the camera is at the origin
void main()
{
    // gl_FragData[0] = vec4(1.0, 0.0, 0.0, 1.0); // writing red to this this is textureA
    // gl_FragData[1] = vec4(0.0, 0.0, 1.0, 1.0); // writing blue to this this is textureB
	
    
    
    
    vec3 n = normalize(normalCam);
    vec3 viewDirCam = normalize(-fragPos);

    // Blinn Phong
    
    vec3 ca = vec3(0.0f, 0.0f, 1.0f); // Compute diffuse color
    // work with lights
    vec3 result = vec3(0.0f, 0.0f, 0.0f);
    
                    // light position default
    vec3 lightPos = vec3(1.0f, 1.0f, 1.0f);
    vec3 lightDirCam = normalize(lightPos - fragPos); // Calculate light direction in camera space
    
    float diffuse = max(0.0f, dot(lightDirCam, n)); // Compute diffuse component
    
    vec3 kd = vec3(0.0f, 0.0f, 1.0f);
    vec3 cd = kd * diffuse; // Compute diffuse color
    
    
    vec3 h = normalize(lightDirCam + viewDirCam);
    float specular = pow(max(0.0f, dot(h, n)), 10.0f); // Compute specular component
    vec3 ks = vec3(1.0f, 1.0f, 1.0f);
    vec3 cs = ks * specular; // Compute specular color
    
    // R        // light color of white
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
    result += lightColor*(ca + cd + cs);

    vec4 blue = vec4(result, 1.0);
    float dotProduct = abs(dot(n, viewDirCam));
    
    // using silhouette shader
    vec4 white = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (dotProduct < 0.3) {
        // gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0); // Black color for silhouette
        white = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        
        
        
    } else {
        //gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0); // White color for non-silhouette
        white = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        

    }
    
    
    
   
    
    gl_FragData[0] = white;
    gl_FragData[1] = blue;
   
    
}
