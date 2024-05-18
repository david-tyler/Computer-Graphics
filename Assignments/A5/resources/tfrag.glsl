#version 120
    
uniform sampler2D textureA; // pos texture
uniform sampler2D textureB; // nor texture
uniform sampler2D textureC; // ke texture
uniform sampler2D textureD; // kd texture
uniform vec2 windowSize;
uniform bool useBlur;

// more uniforms for lighting

uniform vec3 newLightsColors[70];
uniform vec3 newLightsPos[70];

// uniform vec3 ks;
// uniform float s;


vec2 poissonDisk[] = vec2[](
    vec2(-0.220147, 0.976896),
    vec2(-0.735514, 0.693436),
    vec2(-0.200476, 0.310353),
    vec2( 0.180822, 0.454146),
    vec2( 0.292754, 0.937414),
    vec2( 0.564255, 0.207879),
    vec2( 0.178031, 0.024583),
    vec2( 0.613912,-0.205936),
    vec2(-0.385540,-0.070092),
    vec2( 0.962838, 0.378319),
    vec2(-0.886362, 0.032122),
    vec2(-0.466531,-0.741458),
    vec2( 0.006773,-0.574796),
    vec2(-0.739828,-0.410584),
    vec2( 0.590785,-0.697557),
    vec2(-0.081436,-0.963262),
    vec2( 1.000000,-0.100160),
    vec2( 0.622430, 0.680868)
);

vec3 sampleTextureArea(sampler2D texture, vec2 tex0)
{
    const int N = 18; // [1-18]
    const float blur = 0.005;
    vec3 val = vec3(0.0, 0.0, 0.0);
    for(int i = 0; i < N; i++) {
        val += texture2D(texture, tex0.xy + poissonDisk[i]*blur).rgb;
    }
    val /= N;
    return val;
}

    
void main()
{
    vec2 tex;
    tex.x = gl_FragCoord.x/windowSize.x;
    tex.y = gl_FragCoord.y/windowSize.y;
    
    // Fetch shading data
    vec3 pos;
    vec3 nor;
    vec3 ke;
    vec3 kd;
    if (useBlur == false)
    {
        pos = texture2D(textureA, tex).rgb;
        nor = texture2D(textureB, tex).rgb;
        ke = texture2D(textureC, tex).rgb;
        kd = texture2D(textureD, tex).rgb;
    }
    else{
        pos = sampleTextureArea(textureA, tex).rgb;
        nor = sampleTextureArea(textureB, tex).rgb;
        ke = sampleTextureArea(textureC, tex).rgb;
        kd = sampleTextureArea(textureD, tex).rgb;
    }
    // Calculate lighting here
    
    
    
    
    // work with lights
    vec3 result = vec3(0.0f, 0.0f, 0.0f);
    
    // first light
    for(int i = 0; i < 70; i++)
    {
        vec3 n = normalize(nor);
        
        vec3 viewDirCam = normalize(-pos);
        
        
        // DIFFUSE
        vec3 lightDirCam = normalize(newLightsPos[i] - pos); // Calculate light direction in camera space
        float diffuse = max(0.0, dot(lightDirCam, n)); // Compute diffuse component
        vec3 cd = kd * diffuse; // Compute diffuse color
        
        // SPECULAR
        vec3 h = normalize(lightDirCam );
        float specular = pow(max(0.0, dot(h, n)), 10.0f); // Compute specular component
        vec3 cs = ke * specular; // Compute specular color
        
        // ATTENUATION
        float distance = length(newLightsPos[i] - pos);
        vec3 color =  (cd + cs);
        float attenuation =  (1.0 +  0.0429f * distance + 0.9857f * (distance * distance));
        // R
        
        result += (newLightsColors[i] * (color / attenuation));
    }
    
    result += ke;
    
    gl_FragColor.rgb = result.rgb;
}
