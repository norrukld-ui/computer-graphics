#version 330 core

// Два атрибута на вершину: номера location совпадают
// с первым аргументом glVertexAttribPointer в main.cpp.
layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec3 inColor;

// Выход вершинного шейдера. До фрагментного шейдера он дойдёт
// уже интерполированным между тремя вершинами треугольника.
out vec3 vertexColor;

void main() {
    vertexColor = inColor;
    gl_Position = vec4(inPosition, 0.0, 1.0);
}
