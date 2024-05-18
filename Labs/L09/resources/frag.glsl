#version 120

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform vec3 lightPosCam;

varying vec2 vTex0;

void main()
{
	vec3 kd = texture2D(texture0, vTex0).rgb;
	gl_FragColor = vec4(kd, 1.0);
}
