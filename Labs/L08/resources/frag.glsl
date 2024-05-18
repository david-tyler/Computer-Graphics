#version 120

uniform sampler2D texture0;
uniform sampler2D texture1;

varying vec2 vTex0;
varying vec2 vTex1;

void main()
{
	vec4 color0 = texture2D(texture0, vTex0);
	vec4 color1 = texture2D(texture1, vTex1);
    
    // Check if the Reveille texture color is white or close to white
    float epsilon = 0.05;  // Adjust epsilon based on the color tolerance
    bool isWhite = all(lessThanEqual(abs(color1.rgb - vec3(1.0)), vec3(epsilon)));
    
    // Use the color from the original TAMU texture if Reveille texture color is white
    vec4 finalColor = isWhite ? color0 : color1;
    
    gl_FragColor = finalColor;
}
