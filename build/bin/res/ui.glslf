#version 330 core

in vec2 a_textureCoord;
in vec4 a_color;
out vec4 f_color;

uniform sampler2D u_texture;
uniform int u_useTex; // 0 = без текстуры (панели), 1 = с текстурой (шрифт)

void main(){
	vec4 base = (u_useTex == 1) ? texture(u_texture, a_textureCoord) : vec4(1.0);
	f_color = base * a_color;
}

