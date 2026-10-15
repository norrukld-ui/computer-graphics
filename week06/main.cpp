#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH = 1280;
const int HEIGHT = 720;

const float PI = 3.14159265359f;
const float RADIUS = 0.12f;
const float GROUND_Y = -0.65f;
const float JUMP_HEIGHT = 4.0f * RADIUS;
const float CYCLE_TIME = 1.2f;
const float START_X = 1.4f;

bool paused = false;
bool spaceWasPressed = false;

// Пернетақта арқылы ойынды басқару
void processInput(GLFWwindow* window) {
    bool spacePressed =
        glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    // Space пернесі жаңа басылғанда ойынды тоқтатамыз немесе жалғастырамыз
    if (spacePressed && !spaceWasPressed) {
        paused = !paused;
    }

    spaceWasPressed = spacePressed;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

// Вершиналық және фрагменттік шейдерлерді құру
GLuint createShaderProgram() {
    const char* vertexSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;

        uniform mat4 uTransform;

        void main() {
            gl_Position = uTransform * vec4(aPos, 0.0, 1.0);
        }
    )";

    const char* fragmentSource = R"(
        #version 330 core
        uniform vec3 uColor;
        out vec4 FragColor;

        void main() {
            FragColor = vec4(uColor, 1.0);
        }
    )";

    // Вершиналық шейдерді компиляциялау
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    // Фрагменттік шейдерді компиляциялау
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    // Екі шейдерді бір бағдарламаға біріктіру
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Геометрияны GPU жадына жіберу
struct Shape {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLsizei indexCount = 0;
};

// EBO арқылы шеңберді құру: центр және 48 шеткі нүкте
Shape createCircle() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Бірінші төбе — шеңбердің центрі
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    // Шеңбердің шетінде 48 нүкте жасау
    for (int i = 0; i < 48; ++i) {
        float angle = 2.0f * PI * i / 48.0f;

        vertices.push_back(RADIUS * std::cos(angle));
        vertices.push_back(RADIUS * std::sin(angle));
    }

    // Әр үшбұрыш центрден екі көршілес нүктеге жалғанады
    for (unsigned int i = 1; i <= 48; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i == 48 ? 1 : i + 1);
    }

    Shape circle;

    glGenVertexArrays(1, &circle.VAO);
    glGenBuffers(1, &circle.VBO);
    glGenBuffers(1, &circle.EBO);

    glBindVertexArray(circle.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, circle.VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, circle.EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        2 * sizeof(float), nullptr
    );
    glEnableVertexAttribArray(0);

    circle.indexCount = static_cast<GLsizei>(indices.size());

    glBindVertexArray(0);

    return circle;
}

// Теңқабырғалы үшбұрышты құру
Shape createTriangle() {
    float h = RADIUS * std::sqrt(3.0f) / 2.0f;

    // Үшбұрыш центрге қатысты симметриялы орналасады
    float vertices[] = {
        -RADIUS, -h / 3.0f,
         RADIUS, -h / 3.0f,
         0.0f,    2.0f * h / 3.0f
    };

    unsigned int indices[] = {0, 1, 2};

    Shape triangle;

    glGenVertexArrays(1, &triangle.VAO);
    glGenBuffers(1, &triangle.VBO);
    glGenBuffers(1, &triangle.EBO);

    glBindVertexArray(triangle.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, triangle.VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangle.EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        2 * sizeof(float), nullptr
    );
    glEnableVertexAttribArray(0);

    triangle.indexCount = 3;

    glBindVertexArray(0);

    return triangle;
}

// Жер сызығын құру
Shape createGround() {
    float vertices[] = {
        -1.6f, GROUND_Y,
         1.6f, GROUND_Y
    };

    Shape ground;

    glGenVertexArrays(1, &ground.VAO);
    glGenBuffers(1, &ground.VBO);

    glBindVertexArray(ground.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, ground.VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        2 * sizeof(float), nullptr
    );
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    ground.indexCount = 2;

    return ground;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW іске қосылмады!\n";
        return -1;
    }

    // OpenGL 3.3 Core режимін таңдау
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        WIDTH, HEIGHT, "Week 06 - Jump and Roll", nullptr, nullptr
    );

    if (!window) {
        std::cerr << "Терезе ашылмады!\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді!\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    GLuint shaderProgram = createShaderProgram();

    Shape circle = createCircle();
    Shape triangle = createTriangle();
    Shape ground = createGround();

    // Координаталарды экранға түрлендіретін ортографиялық матрица
    glm::mat4 projection = glm::ortho(
        -1.6f, 1.6f,
        -0.9f, 0.9f,
        -1.0f, 1.0f
    );

    GLint transformLocation =
        glGetUniformLocation(shaderProgram, "uTransform");
    GLint colorLocation =
        glGetUniformLocation(shaderProgram, "uColor");

    float elapsed = 0.0f;
    float previousTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        float currentTime = static_cast<float>(glfwGetTime());
        float dt = currentTime - previousTime;
        previousTime = currentTime;

        // Терезе фокусынан шыққанда үлкен секірісті болдырмау
        if (dt > 0.05f) {
            dt = 0.05f;
        }

        processInput(window);

        // Ойын тоқтатылмаса ғана уақыт алға жылжиды
        if (!paused) {
            elapsed += dt;
        }

        // Қайталанатын қозғалыс циклінің 0..1 аралығындағы фазасы
        float phase = std::fmod(elapsed, CYCLE_TIME) / CYCLE_TIME;

        // Шар әр циклде жерден көтеріледі де қайта төмен түседі
        float jump = JUMP_HEIGHT * std::sin(PI * phase);
        float ballY = GROUND_Y + RADIUS + jump;

        // Үшбұрыш солдан оңға қарай қозғалып, цикл соңында қайта басталады
        float triangleX = -START_X + 2.0f * START_X * phase;

        // Жүрілген жолға байланысты үшбұрыштың айналу бұрышы
        float distanceTravelled = 2.0f * START_X * phase;
        float angle = -distanceTravelled / RADIUS;

        // Үшбұрыштың ең төменгі нүктесін айналудан кейін есептейміз
        float h = RADIUS * std::sqrt(3.0f) / 2.0f;

        glm::vec2 triangleVertices[] = {
            {-RADIUS, -h / 3.0f},
            { RADIUS, -h / 3.0f},
            { 0.0f,    2.0f * h / 3.0f}
        };

        float minY = 1000.0f;

        for (const auto& vertex : triangleVertices) {
            glm::vec4 rotated = glm::rotate(
                glm::mat4(1.0f), angle, glm::vec3(0, 0, 1)
            ) * glm::vec4(vertex, 0.0f, 1.0f);

            if (rotated.y < minY) {
                minY = rotated.y;
            }
        }

        // Үшбұрыш айналғанда да жердің үстінде тұрады
        float triangleY = GROUND_Y - minY;

        glClearColor(0.06f, 0.07f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Жер сызығын салу
        glm::mat4 groundModel(1.0f);
        glm::mat4 groundMVP = projection * groundModel;

        glUniformMatrix4fv(
            transformLocation, 1, GL_FALSE, &groundMVP[0][0]
        );
        glUniform3f(colorLocation, 0.75f, 0.78f, 0.85f);

        glBindVertexArray(ground.VAO);
        glDrawArrays(GL_LINES, 0, 2);

        // Шардың орнына glm::translate және өлшеміне glm::scale қолдану
        glm::mat4 ballModel(1.0f);
        ballModel = glm::translate(
            ballModel, glm::vec3(0.0f, ballY, 0.0f)
        );
        ballModel = glm::scale(
            ballModel, glm::vec3(1.0f, 1.0f, 1.0f)
        );

        glm::mat4 ballMVP = projection * ballModel;

        glUniformMatrix4fv(
            transformLocation, 1, GL_FALSE, &ballMVP[0][0]
        );
        glUniform3f(colorLocation, 0.2f, 0.65f, 1.0f);

        glBindVertexArray(circle.VAO);
        glDrawElements(
            GL_TRIANGLES, circle.indexCount,
            GL_UNSIGNED_INT, nullptr
        );

        // Үшбұрыштың орнын, айналуын және өлшемін есептеу
        glm::mat4 triangleModel(1.0f);

        triangleModel = glm::translate(
            triangleModel,
            glm::vec3(triangleX, triangleY, 0.0f)
        );

        triangleModel = glm::rotate(
            triangleModel, angle, glm::vec3(0, 0, 1)
        );

        triangleModel = glm::scale(
            triangleModel, glm::vec3(1.0f, 1.0f, 1.0f)
        );

        glm::mat4 triangleMVP = projection * triangleModel;

        glUniformMatrix4fv(
            transformLocation, 1, GL_FALSE, &triangleMVP[0][0]
        );
        glUniform3f(colorLocation, 1.0f, 0.45f, 0.18f);

        glBindVertexArray(triangle.VAO);
        glDrawElements(
            GL_TRIANGLES, triangle.indexCount,
            GL_UNSIGNED_INT, nullptr
        );

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // GPU жадысын босату
    glDeleteVertexArrays(1, &circle.VAO);
    glDeleteBuffers(1, &circle.VBO);
    glDeleteBuffers(1, &circle.EBO);

    glDeleteVertexArrays(1, &triangle.VAO);
    glDeleteBuffers(1, &triangle.VBO);
    glDeleteBuffers(1, &triangle.EBO);

    glDeleteVertexArrays(1, &ground.VAO);
    glDeleteBuffers(1, &ground.VBO);

    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
