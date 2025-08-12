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

// Function to start GLFW and create a window
GLFWwindow *StartGLFW();
void DrawCircle(float centerX, float centerY, float radius, int res);

int main()
{
    GLFWwindow *window = StartGLFW();

    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    float radius = 50.0f;
    int res = 100;
    bool isResting = false;

    std::vector<float> position = {400.0f, 300.0f};
    std::vector<float> velocity = {0.0f, 0.0f};

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();
        glColor3f(1.0f, 1.0f, 1.0f);

        DrawCircle(position[0], position[1], radius, 50);

        if (!isResting)
        {
            // Update position
            position[0] += velocity[0];
            position[1] += velocity[1];

            // Apply gravity
            velocity[1] += -9.81f / 60.0f;

            // Bounce off bottom
            if (position[1] - radius < 0)
            {
                position[1] = radius;
                velocity[1] *= -0.8f;

                // Check if we should come to rest
                if (std::abs(velocity[1]) < 1.0f)
                {
                    velocity[1] = 0.0f;
                    isResting = true;
                }
            }

            // Bounce off top (optional, not related to jitter)
            if (position[1] + radius > screenHeight)
            {
                position[1] = screenHeight - radius;
                velocity[1] *= -0.8f;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

GLFWwindow *StartGLFW()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialise GLFW" << std::endl;
        return nullptr;
    }

    GLFWwindow *window = glfwCreateWindow(800, 600, "Space Engine", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
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

void DrawCircle(float centerX, float centerY, float radius, int res)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(centerX, centerY);
    for (int i = 0; i <= res; ++i)
    {
        float angle = 2.0f * 3.1415926535f * (static_cast<float>(i) / res);
        float x = centerX + cos(angle) * radius;
        float y = centerY + sin(angle) * radius;
        glVertex2f(x, y);
    }
    glEnd();
}