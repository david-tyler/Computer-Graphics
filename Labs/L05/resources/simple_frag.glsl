#version 120

varying vec3 fragColor; // Receive the color from the vertex shader
uniform int windowHeight; // Uniform variable for window size


void main()
{
    
    
    
    if (gl_FragCoord.y > windowHeight / 2.0)
    {
        gl_FragColor = vec4(0.678, 0.847, 0.902, 1.0); // light blue
    }
    else
    {
        gl_FragColor = vec4(fragColor.rgb, 1.0); // Use the vertex color as the fragment color
    }
	

}
