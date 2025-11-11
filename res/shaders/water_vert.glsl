#version 330 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_texCoord;
layout (location = 3) in float v_alpha;

out vec2 a_texCoord;
out vec3 a_worldPos;
out vec3 a_normal;
out float a_alpha;

uniform mat4 model;
uniform mat4 projview;
uniform mat4 u_view;

void main(){
    vec4 modelpos = model * vec4(v_position, 1.0);
    
    // Вычисляем мировую позицию и нормаль
    a_worldPos = modelpos.xyz;
    mat3 nmat = mat3(transpose(inverse(model)));
    a_normal = normalize(nmat * v_normal);
    
    a_texCoord = v_texCoord;
    a_alpha = v_alpha;
    gl_Position = projview * modelpos;
}

