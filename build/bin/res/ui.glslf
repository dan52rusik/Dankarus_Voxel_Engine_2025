#version 330 core

in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_texture;
uniform int u_useTex; // 0 — панели, 1 — текст

out vec4 f_color;

void main() {
	if (u_useTex == 1) {
		// Как в voxelcore-13: просто умножаем цвет на текстуру
		vec4 t = texture(u_texture, v_uv);
		f_color = v_color * t;
	} else {
		f_color = v_color;
	}
}

