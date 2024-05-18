#version 120

varying vec3 color;

void main()
{
	gl_FragColor.rgb = color;
}
