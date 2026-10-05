#version 330 core

in vec3 vertexColor;     // цвет, смешанный из цветов трёх вершин
uniform bool uNegative;  // клавиша I

out vec4 fragColor;

void main() {
    vec3 color = uNegative ? vec3(1.0) - vertexColor : vertexColor;
    fragColor = vec4(color, 1.0);
}
