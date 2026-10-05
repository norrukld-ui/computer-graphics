
#include <glad/gl.h>     
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

const int WIDTH  = 1280;
const int HEIGHT = 720;

// Пробел басылғанда true болады (3-тапсырма)
bool whiteBackground = false;

// ---------------------------------------------------------------------
//  Терезе өлшемі өзгергенде шақырылады
// ---------------------------------------------------------------------
void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------
//  Пернетақтаны тексеру. Әр кадрда шақырылады.
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        whiteBackground = true;
    }
}

// =====================================================================
//  MAIN
// =====================================================================
int main() {

    // -----------------------------------------------------------------
    //  1. GLFW-ны іске қосу
    // -----------------------------------------------------------------
    if (!glfwInit()) {
        std::cerr << "GLFW іске қосылмады\n";
        return -1;
    }

    // Қандай OpenGL нұсқасы керек екенін айтамыз.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    // -----------------------------------------------------------------
    //  2. Терезе жасау
    // -----------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
                                          "Компьютерлік графика",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті "
                     "қолдамауы мүмкін.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);              // осы терезенің контексі белсенді
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(0);                         // VSync өшірулі (4-тапсырма)

    // -----------------------------------------------------------------
    //  3. GLAD: OpenGL функцияларын жүктеу
    //     Контекст белсенді болғаннан КЕЙІН ғана. Ретін бұзсаң — бәрі құлайды.
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";


    // -----------------------------------------------------------------
    //  4. Негізгі цикл
    // -----------------------------------------------------------------
    int frameCount = 0;
    double lastFpsTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {

        processInput(window);

        // --- Экранды тазалау ---
        if (whiteBackground) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            float t = (float)glfwGetTime();
            float r = (std::sin(t * 3.0f) + 1.0f) * 0.5f * 0.3f;
            float g = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
            glClearColor(r, g, 0.35f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        // --- FPS есептеу (4-тапсырма) ---
        frameCount++;
        double now = glfwGetTime();
        if (now - lastFpsTime >= 1.0) {
            std::cout << "FPS: " << frameCount << "\n";
            frameCount = 0;
            lastFpsTime = now;
        }

        glfwSwapBuffers(window);   // дайын кадрды экранға шығару
        glfwPollEvents();          // пернетақта/тінтуір оқиғаларын өңдеу
    }

    // -----------------------------------------------------------------
    //  5. Тазалау
    // -----------------------------------------------------------------

    glfwTerminate();
    return 0;
}


// =====================================================================
//  1-АПТА ТАПСЫРМАЛАРЫ
// =====================================================================
//  1. Терезенің өлшемін 1280x720 ет.
//
//  2. Фон түсінің өзгеру жылдамдығын арттыр.
//     Кеңес: t-ге көбейтілетін сан — жиілік. Соңындағы 0.3f — амплитуда.
//     Екеуін шатастырма.
//
//  3. Пробел басылғанда фон ақ түске айналсын.
//     Кеңес: processInput ішінде тексеріп, жаһандық айнымалыға жаз.
//     Назар: тексеруді glClear-дан БҰРЫН істе.
//
//  4. glfwSwapInterval(0) қой да, консольге FPS шығар.
//     Кеңес: әр кадрда шығарма — секундына бір рет жеткілікті,
//     әйтпесе консоль қатып қалады.
// =====================================================================