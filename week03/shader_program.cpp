#include "shader_program.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace {

bool readFile(const std::string& path, std::string& text) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::fprintf(stderr, "Cannot open file: %s\n", path.c_str());
        return false;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    text = buffer.str();
    return true;
}

// Длина лога заранее неизвестна: спрашиваем её у OpenGL и выделяем ровно столько.
std::string shaderLog(GLuint shader) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(length > 1 ? length : 1, '\0');
    glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
    return log.data();
}

std::string programLog(GLuint program) {
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(length > 1 ? length : 1, '\0');
    glGetProgramInfoLog(program, static_cast<GLsizei>(log.size()), nullptr, log.data());
    return log.data();
}

// Возвращает id скомпилированного шейдера или 0 при ошибке.
GLuint compileFile(GLenum stage, const std::string& path) {
    std::string source;
    if (!readFile(path, source)) {
        return 0;
    }

    GLuint shader = glCreateShader(stage);
    const char* text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        // Формат лога зависит от драйвера, но почти всегда в нём есть номер
        // строки, например "0(7) : error ..." или "ERROR: 0:7: ...".
        std::fprintf(stderr, "[compile error] %s\n%s\n", path.c_str(), shaderLog(shader).c_str());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(id_);
}

bool ShaderProgram::load(const std::string& vertexPath, const std::string& fragmentPath) {
    GLuint vertex = compileFile(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = compileFile(GL_FRAGMENT_SHADER, fragmentPath);
    if (vertex == 0 || fragment == 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        std::fprintf(stderr, "[link error] %s + %s\n%s\n",
                     vertexPath.c_str(), fragmentPath.c_str(), programLog(program).c_str());
        glDeleteProgram(program);
        return false;
    }

    // Новая программа собрана — только теперь заменяем старую.
    glDeleteProgram(id_);
    id_ = program;
    return true;
}
