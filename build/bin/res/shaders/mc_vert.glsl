#version 330 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;

out vec3 a_normal;
out vec3 a_worldPos; // Мировая позиция для текстурирования

uniform mat4 model;
uniform mat4 projview;

void main() {
	mat3 nmat = mat3(transpose(inverse(model)));
	a_normal = normalize(nmat * v_normal);
	
	// Вычисляем мировую позицию
	vec4 worldPos = model * vec4(v_position, 1.0);
	a_worldPos = worldPos.xyz;
	
	gl_Position = projview * worldPos;
}

