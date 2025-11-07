#version 330 core

layout (location = 0) in vec2 v_position;
layout (location = 1) in vec2 v_textureCoord;
layout (location = 2) in vec4 v_color_in;

out vec2 v_uv;
out vec4 v_color;

uniform mat4 u_projview;

void main(){
	v_uv = v_textureCoord;
	v_color = v_color_in;
	gl_Position = u_projview * vec4(v_position, 0.5, 1.0);
}
