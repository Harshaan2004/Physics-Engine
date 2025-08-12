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

struct Ball
{
    float x, y;   // Position
    float vx, vy; // Velocity
    float radius; // Radius of the ball
    float mass;   // Mass of the ball (for gravitational calculations)
};

// Function to calculate distance between two points
float distance(float x1, float y1, float x2, float y2)
{
    return sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

// Function to start GLFW and create a window
GLFWwindow *StartGLFW()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialise GLFW" << std::endl; // Initialisation failed
        return nullptr;                                        // Return null if initialisation fails
    }
    return glfwCreateWindow((int)screenWidth, (int)screenHeight, "Engine Simulation", NULL, NULL); // Create a windowed mode window and its OpenGL context
}

int main()
{
    GLFWwindow *window = StartGLFW();
    if (!window)
        return -1;

    glfwMakeContextCurrent(window);
    glewInit();

    glViewport(0, 0, screenWidth, screenHeight);     // Define the drawable region of the window
    glMatrixMode(GL_PROJECTION);                     // Set the projection matrix
    glLoadIdentity();                                // Reset projection matrix
    glOrtho(0, screenWidth, 0, screenHeight, -1, 1); // 2D orthographic projection: left, right, bottom, top, near, far
    glMatrixMode(GL_MODELVIEW);                      // Switch back to modelview matrix
    glLoadIdentity();                                // Reset modelview matrix
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);         // Set background color to dark blue

    // Create two balls
    Ball ball1 = {300.0f, 300.0f, 0.0f, 40.0f, 15.0f, 10.0f};  // Ball 1 at (300, 300) with initial velocity and radius
    Ball ball2 = {500.0f, 300.0f, 0.0f, -40.0f, 15.0f, 10.0f}; // Ball 2 at (500, 300) with initial velocity and radius

    float G = 4000.0f; // Artificial gravitational constant

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        // Calculate distance and direction
        float dx = ball2.x - ball1.x;                              // Difference in x-coordinates
        float dy = ball2.y - ball1.y;                              // Difference in y-coordinates
        float dist = distance(ball1.x, ball1.y, ball2.x, ball2.y); // Calculate distance between the two balls

        if (dist > ball1.radius + ball2.radius)
        {
            // Avoid division by zero and simulate attraction
            float force = G * (ball1.mass * ball2.mass) / (dist * dist); // Gravitational force based on mass and distance
            float ax = force * dx / dist / ball1.mass;                   // Acceleration for ball1
            float ay = force * dy / dist / ball1.mass;                   // Acceleration for ball1
            float bx = -force * dx / dist / ball2.mass;                  // Acceleration for ball2 (opposite direction)
            float by = -force * dy / dist / ball2.mass;                  // Acceleration for ball2 (opposite direction)

            // Apply acceleration to velocities
            ball1.vx += ax; // Update ball1's velocity in x direction
            ball1.vy += ay; // Update ball1's velocity in y direction
            ball2.vx += bx; // Update ball2's velocity in x direction
            ball2.vy += by; // Update ball2's velocity in x and y directions
        }

        // Update positions
        ball1.x += ball1.vx * 0.016f; // Update ball1's position based on its velocity
        ball1.y += ball1.vy * 0.016f; // Update ball1's position based on its velocity
        ball2.x += ball2.vx * 0.016f; // Update ball2's position based on its velocity
        ball2.y += ball2.vy * 0.016f; // Update ball2's position based on its velocity

        // Draw ball1
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(0.8f, 0.2f, 0.2f);  // Set color for ball1
        glVertex2f(ball1.x, ball1.y); // Center of the circle
        for (int i = 0; i <= 100; i++)
        {
            float angle = 2.0f * 3.14159f * i / 100;       // Calculate angle for each vertex
            float x = ball1.x + cos(angle) * ball1.radius; // Calculate the vertex position on the circle edge
            float y = ball1.y + sin(angle) * ball1.radius; // Calculate the vertex position on the circle edge
            glVertex2f(x, y);
        }
        glEnd();

        // Draw ball2
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(0.2f, 0.6f, 1.0f);
        glVertex2f(ball2.x, ball2.y);
        for (int i = 0; i <= 100; i++)
        {
            float angle = 2.0f * 3.14159f * i / 100;
            float x = ball2.x + cos(angle) * ball2.radius; // Calculate the vertex position on the circle edge
            float y = ball2.y + sin(angle) * ball2.radius; // Calculate the vertex position on the circle edge
            glVertex2f(x, y);
        }
        glEnd();

        glfwSwapBuffers(window);           // Swap front and back buffers to display the rendered frame
        glfwPollEvents();                  // Poll for and process events
        glfwWaitEventsTimeout(1.0 / 60.0); // Limit frame rate to ~60fps
    }

    glfwTerminate(); // Clean up and close the window
    return 0;        // Exit the program
}