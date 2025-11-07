#version 330 core

in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_texture;   // без layout(binding)
out vec4 f_color;

void main() {
    vec4 t = texture(u_texture, v_uv);
    // Для белой текстуры шрифта (RGB=1.0, альфа=маска):
    // Умножаем цвет на текстуру - это даст правильный результат
    // v_color * t = (v_color.rgb * t.rgb, v_color.a * t.a)
    // Для белой текстуры (t.rgb = 1.0) это даст (v_color.rgb, v_color.a * t.a)
    f_color = v_color * t;
}
