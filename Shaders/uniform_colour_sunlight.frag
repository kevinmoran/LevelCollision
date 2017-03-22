#version 140

in vec3 normal;
uniform vec4 colour;

out vec4 frag_colour;

vec3 sun_dir = normalize(vec3(-1.0, -1.0, 0.3));

void main() {
	float x = dot(-sun_dir, normal);
	x = x+0.8;
	frag_colour = colour*x;
}