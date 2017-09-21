#version 140

in vec3 vp;
in vec2 vt;
in vec3 vn;
uniform mat4 M,V,P;

out vec2 tex_coords;
out vec3 normal;

void main () {
	tex_coords = vt;
	normal = mat3(M)*vn;
	gl_Position = P*V*M*vec4(vp, 1.0) - vec4(0.0,0.0,0.005,0.0);
}