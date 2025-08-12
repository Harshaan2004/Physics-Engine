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
float G = 5.0f; // Gravitational constant

GLFWwindow *StartGLFW();

class Object
{
public:
    std::vector<float> position;
    std::vector<float> velocity;
    float radius;
    float mass;

    Object(std::vector<float> position, std::vector<float> velocity, float mass, float radius = 15.0f)
    {
        this->position = position;
        this->velocity = velocity;
        this->mass = mass;
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
        Object({100.0f, 300.0f}, {0.5f, -0.2f}, 100.0f, 15.0f),  // red
        Object({250.0f, 450.0f}, {-0.3f, 0.1f}, 100.0f, 15.0f),  // green
        Object({400.0f, 150.0f}, {0.2f, 0.4f}, 100.0f, 15.0f),   // blue
        Object({550.0f, 400.0f}, {-0.4f, -0.3f}, 100.0f, 15.0f), // yellow
        Object({700.0f, 250.0f}, {-0.2f, 0.3f}, 100.0f, 15.0f),  // magenta
    };

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();

        glColor3f(1.0f, 1.0f, 1.0f);

        for (auto &object : objects)
        {
            // Physics
            for (auto &other : objects)
            {
                if (&object != &other)
                {
                    float dx = other.position[0] - object.position[0];
                    float dy = other.position[1] - object.position[1];
                    float dist = sqrt(dx * dx + dy * dy);
                    std::vector<float> direction = {dx / dist, dy / dist};
                    dist = std::max(dist, 1.0f); // avoid division by zero

                    if (dist > object.radius + other.radius)
                    {
                        float force = G * (object.mass * other.mass) / (dist * dist);
                        float ax = force * dx / dist / object.mass;
                        float ay = force * dy / dist / object.mass;
                        object.accelerate(ax, ay);
                    }
                }
            };

            // Color based on memory address
            if (&object == &objects[0])
                glColor3f(1.0f, 0.0f, 0.0f); // red

            else if (&object == &objects[1])
                glColor3f(0.0f, 1.0f, 0.0f); // green

            else if (&object == &objects[2])
                glColor3f(0.0f, 0.0f, 1.0f); // blue

            else if (&object == &objects[3])
                glColor3f(1.0f, 1.0f, 0.0f); // yellow

            else if (&object == &objects[4])
                glColor3f(1.0f, 0.0f, 1.0f); // magenta

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

    GLFWwindow *window = glfwCreateWindow(800, 600, "Space Engine", NULL, NULL);
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