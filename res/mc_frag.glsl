#version 330 core

in vec3 a_normal;
out vec4 f_color;

uniform vec3 u_lightDir = normalize(vec3(0.4, 0.8, 0.4));
uniform vec3 u_color = vec3(0.7, 0.85, 1.0);

void main() {
	float NdotL = max(dot(normalize(a_normal), normalize(u_lightDir)), 0.0);
	float ambient = 0.2;
	vec3 col = (ambient + NdotL) * u_color;
	f_color = vec4(col, 1.0);
}

