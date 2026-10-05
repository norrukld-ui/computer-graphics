// Шейдерная программа, которая читает GLSL из файлов и умеет
// перезагружаться на лету.
#pragma once

#include <glad/gl.h>

#include <string>

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    // Владеет объектом OpenGL — копировать нельзя, иначе удалим его дважды.
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    // Читает два файла, компилирует и линкует их.
    // При успехе заменяет текущую программу новой и возвращает true.
    // При ошибке печатает лог (файл, строка, причина), оставляет
    // прежнюю рабочую программу и возвращает false.
    bool load(const std::string& vertexPath, const std::string& fragmentPath);

    bool isValid() const { return id_ != 0; }
    void use() const { glUseProgram(id_); }
    GLint uniformLocation(const char* name) const { return glGetUniformLocation(id_, name); }

private:
    GLuint id_ = 0;
};
