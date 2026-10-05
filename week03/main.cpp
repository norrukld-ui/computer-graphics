// Неделя 3. Цвет как атрибут вершины и его интерполяция по треугольнику.
// Шейдеры лежат в отдельных файлах (week03/shaders) и перезагружаются на лету.
//
// Управление:
//   I   — инвертировать цвета в фрагментном шейдере (1.0 - цвет)
//   U   — всем трём вершинам один и тот же цвет (интерполяции не видно)
//   R   — перечитать шейдеры с диска (можно править .frag и сразу смотреть)
//   B   — загрузить заведомо сломанный broken.frag и увидеть лог ошибки
//   Esc — выход

#include "shader_program.hpp"

#include <GLFW/glfw3.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <string>

namespace {

// Путь к папке шейдеров подставляет CMake (см. week03/CMakeLists.txt),
// поэтому программа находит их откуда бы её ни запустили.
const std::string kShaderDir = WEEK03_SHADER_DIR;
const std::string kVertexPath = kShaderDir + "triangle.vert";
const std::string kFragmentPath = kShaderDir + "triangle.frag";
const std::string kBrokenFragmentPath = kShaderDir + "broken.frag";

// Позиция и цвет лежат в буфере вперемешку: x y r g b | x y r g b | ...
struct Vertex {
    float position[2];
    float color[3];
};

constexpr std::array<Vertex, 3> kRgbTriangle = {{
    {{-0.6f, -0.55f}, {1.0f, 0.0f, 0.0f}},  // красный
    {{ 0.6f, -0.55f}, {0.0f, 1.0f, 0.0f}},  // зелёный
    {{ 0.0f,  0.65f}, {0.0f, 0.0f, 1.0f}},  // синий
}};
constexpr float kSingleColor[3] = {0.95f, 0.55f, 0.20f};

struct AppState {
    bool invert = false;
    bool monochrome = false;
    bool colorsChanged = false;  // нужно перезалить вершины в VBO
    bool reloadRequested = false;
    bool brokenRequested = false;
};

void onGlfwError(int code, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

void onFramebufferResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void onKey(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) {
        return;
    }
    auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_I:
        state->invert = !state->invert;
        std::printf("Invert: %s\n", state->invert ? "on" : "off");
        break;
    case GLFW_KEY_U:
        state->monochrome = !state->monochrome;
        state->colorsChanged = true;
        std::printf("Vertex colors: %s\n", state->monochrome ? "all the same" : "red / green / blue");
        break;
    case GLFW_KEY_R:
        state->reloadRequested = true;
        break;
    case GLFW_KEY_B:
        state->brokenRequested = true;
        break;
    default:
        break;
    }
}

GLFWwindow* openWindow() {
    glfwSetErrorCallback(onGlfwError);
    if (!glfwInit()) {
        return nullptr;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "CG Week 03 - Colored triangle", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::fprintf(stderr, "Failed to load OpenGL functions (GLAD)\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }
    glfwSetFramebufferSizeCallback(window, onFramebufferResize);
    glfwSwapInterval(1);
    return window;
}

std::array<Vertex, 3> currentVertices(bool monochrome) {
    std::array<Vertex, 3> vertices = kRgbTriangle;
    if (monochrome) {
        for (Vertex& v : vertices) {
            for (int i = 0; i < 3; ++i) {
                v.color[i] = kSingleColor[i];
            }
        }
    }
    return vertices;
}

// Весь цикл работы с OpenGL-объектами вынесен в отдельную функцию:
// ShaderProgram удаляет программу в деструкторе, а это должно
// произойти до того, как мы уничтожим окно и контекст.
int run(GLFWwindow* window) {
    AppState state;
    glfwSetWindowUserPointer(window, &state);
    glfwSetKeyCallback(window, onKey);

    ShaderProgram shader;
    if (!shader.load(kVertexPath, kFragmentPath)) {
        return 1;
    }
    GLint invertLocation = shader.uniformLocation("uNegative");

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // GL_DYNAMIC_DRAW — подсказка драйверу, что содержимое будем менять (клавиша U).
    const std::array<Vertex, 3> initial = currentVertices(state.monochrome);
    glBufferData(GL_ARRAY_BUFFER, sizeof(initial), initial.data(), GL_DYNAMIC_DRAW);

    // Шаг (stride) у обоих атрибутов — размер целой вершины,
    // а смещение показывает, где внутри вершины начинается нужное поле.
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    std::printf("Controls: I - invert, U - same color, R - reload shaders, "
                "B - load broken shader, Esc - quit\n");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (state.colorsChanged) {
            // Меняем только содержимое уже созданного буфера, не пересоздавая его.
            const std::array<Vertex, 3> vertices = currentVertices(state.monochrome);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices.data());
            state.colorsChanged = false;
        }
        if (state.reloadRequested || state.brokenRequested) {
            const std::string& fragment = state.brokenRequested ? kBrokenFragmentPath : kFragmentPath;
            std::printf("Loading %s ...\n", fragment.c_str());
            if (shader.load(kVertexPath, fragment)) {
                // У новой программы uniform может оказаться в другом месте.
                invertLocation = shader.uniformLocation("uNegative");
                std::printf("Shaders reloaded\n");
            } else {
                std::printf("Keeping the previous working shader\n");
            }
            state.reloadRequested = false;
            state.brokenRequested = false;
        }

        glClearColor(0.10f, 0.10f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Сначала glUseProgram, потом glUniform*: uniform задаётся
        // для программы, которая сейчас активна.
        shader.use();
        glUniform1i(invertLocation, state.invert ? 1 : 0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    return 0;
}

} // namespace

int main() {
    GLFWwindow* window = openWindow();
    if (window == nullptr) {
        return 1;
    }
    const int code = run(window);
    glfwDestroyWindow(window);
    glfwTerminate();
    return code;
}
