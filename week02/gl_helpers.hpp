// Общие для двух программ недели 2 вспомогательные функции:
// создание окна и сборка шейдерной программы.
#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

// Инициализирует GLFW, открывает окно с контекстом OpenGL 3.3 Core и
// загружает функции OpenGL через GLAD. При ошибке возвращает nullptr.
GLFWwindow* openWindow(int width, int height, const char* title);

// Компилирует вершинный и фрагментный шейдеры и связывает их в программу.
// При ошибке печатает лог компилятора и возвращает 0.
GLuint buildShaderProgram(const char* vertexSource, const char* fragmentSource);
