#version 330 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;

out vec3 a_normal;

uniform mat4 model;
uniform mat4 projview;

void main() {
	mat3 nmat = mat3(transpose(inverse(model)));
	a_normal = normalize(nmat * v_normal);
	gl_Position = projview * model * vec4(v_position, 1.0);
}

