// Неделя 2, часть 2. Сетка kCols x kRows: каждая клетка разрезана диагональю
// (левый-нижний -> правый-верхний угол) на два треугольника:
// нижний-правый — белый, верхний-левый — чёрный.
//
// Управление:
//   W   — каркасный режим вкл/выкл (видно, из каких треугольников состоит сетка)
//   Esc — выход

#include "gl_helpers.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

constexpr int kCols = 3;
constexpr int kRows = 4;
constexpr float kFill = 0.85f;  // какую долю окна занимает сетка по узкой стороне

constexpr float kLowerHalfColor[] = {1.0f, 1.0f, 1.0f};  // белый
constexpr float kUpperHalfColor[] = {0.0f, 0.0f, 0.0f};  // чёрный

struct Vertex {
    float x, y;
};

// Вершины задаются в «клетках»: клетка — квадрат 1x1, сетка центрирована
// в (0, 0). В NDC их переводит uniform uCellToNdc в вершинном шейдере.
const char* const kVertexShader = R"(#version 330 core
layout (location = 0) in vec2 inPosition;
uniform vec2 uCellToNdc;
void main() {
    gl_Position = vec4(inPosition * uCellToNdc, 0.0, 1.0);
}
)";

const char* const kFragmentShader = R"(#version 330 core
uniform vec3 uFillColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(uFillColor, 1.0);
}
)";

// Буфер строится так: сначала нижние-правые треугольники всех клеток,
// потом верхние-левые. Тогда вся сетка рисуется двумя вызовами glDrawArrays —
// по одному на цвет, — а не двумя на каждую клетку.
std::vector<Vertex> buildGrid() {
    std::vector<Vertex> lower;
    std::vector<Vertex> upper;
    const float left = -kCols / 2.0f;
    const float bottom = -kRows / 2.0f;

    for (int row = 0; row < kRows; ++row) {
        for (int col = 0; col < kCols; ++col) {
            const float x0 = left + col;
            const float y0 = bottom + row;
            const float x1 = x0 + 1.0f;
            const float y1 = y0 + 1.0f;
            // Обе половины опираются на диагональ (x0,y0) -> (x1,y1).
            lower.insert(lower.end(), {{x0, y0}, {x1, y0}, {x1, y1}});
            upper.insert(upper.end(), {{x0, y0}, {x1, y1}, {x0, y1}});
        }
    }

    lower.insert(lower.end(), upper.begin(), upper.end());
    return lower;
}

struct Scale {
    float x, y;
};

// Подбирает масштаб «клетки -> NDC» так, чтобы сетка помещалась в окно,
// а клетки оставались квадратными при любом соотношении сторон.
// Почему делим на aspect: 1 единица NDC по x = width/2 пикселей,
// по y = height/2 пикселей. Чтобы клетка была квадратной в пикселях,
// её размер по x в NDC должен быть в aspect раз меньше, чем по y.
Scale fitGridToWindow(int width, int height) {
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    const float sy = 2.0f * kFill * std::min(1.0f / kRows, aspect / kCols);
    return {sy / aspect, sy};
}

void onKey(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) {
        return;
    }
    auto* showEdges = static_cast<bool*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_W) {
        *showEdges = !*showEdges;
    }
}

} // namespace

int main() {
    GLFWwindow* window = openWindow(1280, 720, "CG Week 02 - Grid");
    if (window == nullptr) {
        return 1;
    }

    bool showEdges = false;
    glfwSetWindowUserPointer(window, &showEdges);
    glfwSetKeyCallback(window, onKey);

    GLuint program = buildShaderProgram(kVertexShader, kFragmentShader);
    if (program == 0) {
        glfwTerminate();
        return 1;
    }
    const GLint scaleLocation = glGetUniformLocation(program, "uCellToNdc");
    const GLint colorLocation = glGetUniformLocation(program, "uFillColor");

    const std::vector<Vertex> vertices = buildGrid();
    const GLsizei halfCount = static_cast<GLsizei>(vertices.size() / 2);

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    std::printf("Grid %dx%d: %d cells, %zu vertices\n", kCols, kRows, kCols * kRows, vertices.size());
    std::printf("Controls: W - wireframe, Esc - quit\n");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            continue;  // окно свёрнуто — рисовать некуда
        }

        glClearColor(0.30f, 0.42f, 0.58f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, showEdges ? GL_LINE : GL_FILL);

        glUseProgram(program);
        const Scale scale = fitGridToWindow(width, height);
        glUniform2f(scaleLocation, scale.x, scale.y);
        glBindVertexArray(vao);

        glUniform3fv(colorLocation, 1, kLowerHalfColor);
        glDrawArrays(GL_TRIANGLES, 0, halfCount);
        glUniform3fv(colorLocation, 1, kUpperHalfColor);
        glDrawArrays(GL_TRIANGLES, halfCount, halfCount);

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
