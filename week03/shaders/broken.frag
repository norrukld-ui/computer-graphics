#version 330 core

// Этот шейдер НАМЕРЕННО содержит ошибку — для демонстрации вывода
// лога компиляции (клавиша B). Ниже опечатка: vertexColour
// (британское написание) вместо vertexColor.

in vec3 vertexColor;
uniform bool uNegative;

out vec4 fragColor;

void main() {
    vec3 color = uNegative ? vec3(1.0) - vertexColour : vertexColor;
    fragColor = vec4(color, 1.0);
}
