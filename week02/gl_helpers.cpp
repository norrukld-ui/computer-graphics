#include "gl_helpers.hpp"

#include <cstdio>

namespace {

void onGlfwError(int code, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

void onFramebufferResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint compileStage(GLenum stage, const char* source) {
    GLuint shader = glCreateShader(stage);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "%s shader error:\n%s\n",
                     stage == GL_VERTEX_SHADER ? "Vertex" : "Fragment", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

GLFWwindow* openWindow(int width, int height, const char* title) {
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

    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
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
    std::printf("OpenGL %s | %s\n",
                reinterpret_cast<const char*>(glGetString(GL_VERSION)),
                reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    return window;
}

GLuint buildShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertex = compileStage(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = compileStage(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertex == 0 || fragment == 0) {
        glDeleteShader(vertex);   // glDeleteShader(0) просто ничего не делает
        glDeleteShader(fragment);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    // После линковки исходные шейдеры уже не нужны — код скопирован в программу.
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Program link error:\n%s\n", log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}
