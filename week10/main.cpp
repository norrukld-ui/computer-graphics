#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
constexpr bool advanced = true;
struct Camera {
    glm::vec3 position{0.0f, 2.0f, 11.0f};
    float yaw = -90.0f, pitch = -9.0f, fov = 50.0f;
    bool limitPitch = true, invertY = false, captured = true, firstMouse = true;
    double lastX = 0.0, lastY = 0.0;

    glm::vec3 front() const {
        const float y = glm::radians(yaw), p = glm::radians(pitch);
        return glm::normalize(glm::vec3(std::cos(y) * std::cos(p),
                                       std::sin(p), std::sin(y) * std::cos(p)));
    }
    glm::vec3 right() const {
        return glm::vec3(-std::sin(glm::radians(yaw)), 0.0f,
                         std::cos(glm::radians(yaw)));
    }
    glm::vec3 up() const { return glm::normalize(glm::cross(right(), front())); }
};
Camera camera;
void errorCallback(int code, const char* message) {
    std::cerr << "GLFW error " << code << ": " << message << '\n';
}
void mouseCallback(GLFWwindow* window, double x, double y) {
    if (!camera.captured || !glfwGetWindowAttrib(window, GLFW_FOCUSED)) return;
    if (camera.firstMouse) {
        camera.lastX = x; camera.lastY = y; camera.firstMouse = false;
        return;
    }
    camera.yaw += static_cast<float>(x - camera.lastX) * 0.1f;
    camera.pitch += static_cast<float>(camera.lastY - y) * 0.1f
                    * (camera.invertY ? -1.0f : 1.0f);
    camera.lastX = x; camera.lastY = y;
    camera.yaw = std::remainder(camera.yaw, 360.0f);
    if (camera.limitPitch) camera.pitch = std::clamp(camera.pitch, -89.0f, 89.0f);
    else camera.pitch = std::remainder(camera.pitch, 360.0f);
}
void scrollCallback(GLFWwindow*, double, double offset) {
    camera.fov = std::clamp(camera.fov - static_cast<float>(offset), 1.0f, 90.0f);
}
void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_R) {
        camera = Camera{};
        if (advanced) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    if (!advanced) return;
    if (key == GLFW_KEY_TAB) {
        camera.captured = !camera.captured;
        camera.firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR,
                        camera.captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
    if (key == GLFW_KEY_L) {
        camera.limitPitch = !camera.limitPitch;
        if (camera.limitPitch) camera.pitch = std::clamp(camera.pitch, -89.0f, 89.0f);
        std::cout << "Pitch limit: " << (camera.limitPitch ? "on" : "off") << '\n';
    }
    if (key == GLFW_KEY_I) {
        camera.invertY = !camera.invertY;
        std::cout << "Invert Y: " << (camera.invertY ? "on" : "off") << '\n';
    }
    if (key == GLFW_KEY_P) {
        const auto f = camera.front();
        std::cout << std::fixed << std::setprecision(3)
                  << "Position: (" << camera.position.x << ", " << camera.position.y
                  << ", " << camera.position.z << ") Direction: (" << f.x << ", "
                  << f.y << ", " << f.z << ") Yaw: " << camera.yaw
                  << " Pitch: " << camera.pitch << " FOV: " << camera.fov << std::endl;
    }
}
void moveCamera(GLFWwindow* window, float dt) {
    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) return;
    auto pressed = [window](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };
    glm::vec3 movement(0.0f);
    if (pressed(GLFW_KEY_W)) movement += camera.front();
    if (pressed(GLFW_KEY_S)) movement -= camera.front();
    if (pressed(GLFW_KEY_D)) movement += camera.right();
    if (pressed(GLFW_KEY_A)) movement -= camera.right();
    if (pressed(GLFW_KEY_SPACE)) movement.y += 1.0f;
    if (pressed(GLFW_KEY_LEFT_SHIFT) || pressed(GLFW_KEY_RIGHT_SHIFT)) movement.y -= 1.0f;
    const bool fast = pressed(GLFW_KEY_LEFT_CONTROL) || pressed(GLFW_KEY_RIGHT_CONTROL);
    if (glm::length(movement) > 0.0001f)
        camera.position += glm::normalize(movement) * (fast ? 9.0f : 3.0f) * dt;
}
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]; glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(log);
    }
    return shader;
}
GLuint createProgram() {
    const char* vertex = R"(#version 330 core
layout(location = 0) in vec3 position;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 worldPosition;
void main() {
    vec4 world = model * vec4(position, 1.0);
    worldPosition = world.xyz;
    gl_Position = projection * view * world;
})";
    const char* fragment = R"(#version 330 core
in vec3 worldPosition;
uniform vec3 color;
out vec4 fragColor;
void main() {
    vec3 normal = normalize(cross(dFdx(worldPosition), dFdy(worldPosition)));
    if (!gl_FrontFacing) normal = -normal;
    float light = 0.38 + 0.62 * max(dot(normal, normalize(vec3(0.4, 0.8, 0.6))), 0.0);
    fragColor = vec4(color * light, 1.0);
})";
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertex);
    GLuint fs;
    try { fs = compileShader(GL_FRAGMENT_SHADER, fragment); }
    catch (...) { glDeleteShader(vs); throw; }
    GLuint program = glCreateProgram();
    glAttachShader(program, vs); glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint ok; glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048]; glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        glDeleteProgram(program);
        throw std::runtime_error(log);
    }
    return program;
}
struct Cube { glm::vec3 position, color; float size, speed; };
std::vector<Cube> createScene() {
    if (!advanced) return {
        {{0, 0, 0}, {0.12f, 0.78f, 0.70f}, 1.8f, 0.26f},
        {{-3.1f, 0.5f, -1.5f}, {0.93f, 0.67f, 0.22f}, 1.25f, -0.32f},
        {{3.0f, -0.3f, -0.7f}, {0.24f, 0.66f, 0.36f}, 1.5f, 0.22f},
        {{-1.7f, -1.5f, 1.0f}, {0.08f, 0.57f, 0.55f}, 0.65f, 0.4f},
        {{1.5f, 2.0f, -2.0f}, {0.96f, 0.79f, 0.36f}, 0.85f, -0.28f},
        {{-0.5f, 0.7f, -4.5f}, {0.18f, 0.49f, 0.28f}, 1.1f, 0.2f}
    };
    std::vector<Cube> cubes{{{0, 0, 0}, {0.93f, 0.73f, 0.27f}, 1.9f, 0.17f}};
    for (int i = 0; i < 12; ++i) {
        const float angle = glm::radians(30.0f * i);
        cubes.push_back({{3.7f * std::cos(angle), (i % 3 - 1) * 0.7f,
                          3.7f * std::sin(angle)},
                         i % 3 == 0 ? glm::vec3(0.85f, 0.65f, 0.22f)
                                    : glm::vec3(0.22f, 0.64f + 0.02f * i, 0.34f),
                         0.45f + 0.17f * (i % 4), 0.12f + 0.015f * i});
    }
    return cubes;
}
}

int main() {
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(1280, 800,
        advanced ? "Week 10 - Advanced Free Camera" : "Week 09 - 3D Scene and Camera",
        nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to load OpenGL\n";
        glfwDestroyWindow(window); glfwTerminate(); return 1;
    }
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetWindowFocusCallback(window, [](GLFWwindow*, int) { camera.firstMouse = true; });
    if (advanced) {
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    std::cout << "WASD: move | Space/Shift: up/down | Ctrl: faster | R: reset | Esc: exit\n";
    if (advanced) std::cout << "Mouse: look | Wheel: FOV | L: pitch limit | I: invert Y"
                              " | P: camera info | Tab: cursor\n";
    GLuint program;
    try { program = createProgram(); }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        glfwDestroyWindow(window); glfwTerminate(); return 1;
    }
    const float vertices[] = {
        -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f,0.5f,-0.5f, -0.5f,0.5f,-0.5f,
        -0.5f,-0.5f,0.5f, 0.5f,-0.5f,0.5f, 0.5f,0.5f,0.5f, -0.5f,0.5f,0.5f
    };
    const unsigned int indices[] = {
        0,3,2, 2,1,0, 4,5,6, 6,7,4,
        0,4,7, 7,3,0, 1,2,6, 6,5,1,
        0,1,5, 5,4,0, 3,7,6, 6,2,3
    };
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(program);
    const GLint modelLoc = glGetUniformLocation(program, "model");
    const GLint viewLoc = glGetUniformLocation(program, "view");
    const GLint projectionLoc = glGetUniformLocation(program, "projection");
    const GLint colorLoc = glGetUniformLocation(program, "color");
    const auto cubes = createScene();
    double previous = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - previous);
        previous = now;
        glfwPollEvents();
        moveCamera(window, dt);
        int width, height; glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            glfwWaitEventsTimeout(0.05);
            previous = glfwGetTime();
            continue;
        }
        glViewport(0, 0, width, height);
        if (advanced) glClearColor(0.025f, 0.038f, 0.028f, 1.0f);
        else glClearColor(0.018f, 0.085f, 0.073f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const auto view = glm::lookAt(camera.position, camera.position + camera.front(), camera.up());
        const auto projection = glm::perspective(glm::radians(camera.fov),
                                static_cast<float>(width) / height, 0.1f, 150.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        const float t = static_cast<float>(now);
        for (std::size_t i = 0; i < cubes.size(); ++i) {
            const auto& cube = cubes[i];
            auto position = cube.position;
            if (advanced) position.y += 0.28f * std::sin(t * 0.65f + static_cast<float>(i));
            auto model = glm::translate(glm::mat4(1.0f), position);
            model = glm::rotate(model, t * cube.speed + static_cast<float>(i) * 0.3f,
                                glm::normalize(glm::vec3(0.4f, 1.0f, 0.25f)));
            model = glm::scale(model, glm::vec3(cube.size));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(colorLoc, 1, glm::value_ptr(cube.color));
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
        }
        glfwSwapBuffers(window);
    }
    glDeleteBuffers(1, &ebo); glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao); glDeleteProgram(program);
    glfwDestroyWindow(window); glfwTerminate();
    return 0;
}
