#version 120

uniform mat4 P;
uniform mat4 MV;
uniform float t;
uniform mat4 invTransposeMV; // Uniform for inverse transpose of MV
attribute vec4 aPos; // In object space
// attribute vec3 aNor; // In object space
attribute vec2 aTex;

varying vec3 vNor; // In camera space
varying vec2 vTex;

void main()
{
    float x = aPos.x;
    float theta = aPos.y;
    
    // Compute position
    float fx = cos(x + t) + 2;
    float y = fx * cos(theta);
    float z = fx * sin(theta);
    vec3 p = vec3(x, y, z);

    // Compute normal
    float dfdx = -sin(x);
    
    float dp_dx_x = 1.0f;
    float dp_dx_y = dfdx * cos(theta);
    float dp_dx_z = dfdx * sin(theta);
    
    float dp_dtheta_x = 0.0f;
    float dp_dtheta_y = -(fx * sin(theta));
    float dp_dtheta_z = fx * cos(theta);
    
    vec3 dp_dx = vec3(dp_dx_x, dp_dx_y,  dp_dx_z);
    vec3 dp_dtheta = vec3(dp_dtheta_x, dp_dtheta_y, dp_dtheta_z);
    vec3 n = normalize(cross(dp_dx, dp_dtheta));
    
	gl_Position = P * (MV * vec4(p, 1.0));
    
    vNor = normalize(mat3(invTransposeMV) * n); // Assuming MV contains only translations and rotations
	vTex = aTex;
}
