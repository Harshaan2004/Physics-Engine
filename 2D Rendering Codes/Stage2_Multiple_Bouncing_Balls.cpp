#include <GL/glew.h>                    // GLEW helps load OpenGL extensions, must be included before GLFW
#include <GLFW/glfw3.h>                 // GLFW handles window creation and input
#include <glm/glm.hpp>                  // GLM for math (not directly used here yet)
#include <glm/gtc/matrix_transform.hpp> // GLM matrix transforms (not directly used)
#include <glm/gtc/type_ptr.hpp>         // Convert GLM types to raw pointers (not directly used)

#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath> // Needed for sine and cosine functions

float screenWidth = 800.0f;
float screenHeight = 600.0f;

GLFWwindow *StartGLFW();

class Object
{
public:
    std::vector<float> position;
    std::vector<float> velocity;
    float radius;

    Object(std::vector<float> position, std::vector<float> velocity, float radius = 15.0f)
    {
        this->position = position;
        this->velocity = velocity;
        this->radius = radius;
    }

    void accelerate(float x, float y)
    {
        this->velocity[0] += x;
        this->velocity[1] += y;
    }

    void updatePosition()
    {
        this->position[0] += this->velocity[0];
        this->position[1] += this->velocity[1];
    }

    void DrawCircle(int res = 50)
    {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(position[0], position[1]);
        for (int i = 0; i <= res; ++i)
        {
            float angle = 2.0f * 3.1415926535f * (static_cast<float>(i) / res);
            float x = position[0] + cos(angle) * radius;
            float y = position[1] + sin(angle) * radius;
            glVertex2f(x, y);
        }
        glEnd();
    }
};

int main()
{
    GLFWwindow *window = StartGLFW();

    if (!window)
        return -1;

    std::vector<Object> objects = {
        Object({200.0f, 500.0f}, {2.0f, 0.0f}),
        Object({400.0f, 550.0f}, {-3.0f, 0.0f}),
        Object({600.0f, 520.0f}, {1.5f, 0.0f}),
    };

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();

        glColor3f(1.0f, 1.0f, 1.0f);

        for (auto &object : objects)
        {
            // Physics
            object.accelerate(0.0f, -0.2f); // Gravity
            object.updatePosition();

            // Bounce on walls
            if (object.position[1] - object.radius < 0 || object.position[1] + object.radius > screenHeight)
            {
                object.velocity[1] *= -0.95f;
            }
            if (object.position[0] - object.radius < 0 || object.position[0] + object.radius > screenWidth)
            {
                object.velocity[0] *= -0.95f;
            }

            // Draw
            object.DrawCircle();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

GLFWwindow *StartGLFW()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialise GLFW" << std::endl;
        return nullptr;
    }

    GLFWwindow *window = glfwCreateWindow(800, 600, "Physics", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return nullptr;
    }

    glViewport(0, 0, 800, 600);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    return window;
}