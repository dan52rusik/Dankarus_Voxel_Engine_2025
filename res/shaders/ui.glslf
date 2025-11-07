#version 330 core

in vec2 a_textureCoord;
in vec4 a_color;
out vec4 f_color;

uniform sampler2D u_texture;
uniform int u_useTex;

void main() {
	if (u_useTex == 1) {
		// ШАГ A: Диагностика - игнорим текстуру, рисуем чисто пурпурный
		f_color = vec4(1.0, 0.0, 1.0, 1.0);
	} else {
		f_color = a_color;
	}
}
