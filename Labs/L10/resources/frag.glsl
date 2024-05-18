#version 120

uniform sampler2D texture0;

varying vec3 vNor; // Unused
varying vec2 vTex;

void main()
{
	vec3 n = normalize(vNor);
	gl_FragColor.rgb = abs(n.z) * texture2D(texture0, vTex).rgb;
}
