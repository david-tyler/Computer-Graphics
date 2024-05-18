#version 120

uniform sampler2D textureA;
uniform sampler2D textureB;

uniform float t;
varying vec2 vTex;

void main()
{
	//gl_FragColor.rgb = vec3(vTex, 0.0);
    if(t > 5)
    {
        gl_FragColor.rgb = texture2D(textureB, vTex).rgb;

    }
    else
    {
        gl_FragColor.rgb = texture2D(textureA, vTex).rgb;

    }
    
}
