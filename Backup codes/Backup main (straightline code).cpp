//
//  main.cpp
//  PlanetEngine
//
//  Created by Harshaan Singh Bahia on 18/07/2025.
//
//  This file is part of the PlanetEngine project.
//  PlanetEngine is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.

#include <GL/glew.h> // GLEW must come before GLFW
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>

// Struct to define a celestial body (e.g., sun, planet)
struct Body
{
    glm::vec2 position;           // Position of the body in 2D
    glm::vec2 velocity;           // Velocity vector
    float mass;                   // Mass affects gravitational pull
    float radius;                 // Visual size of the body
    std::vector<glm::vec2> trail; // History of positions (for orbit trail)
};

// Struct to define a static star in the background
struct Star
{
    float x, y;
};

std::vector<Star> stars;

// Initializes star positions randomly across the screen (called once)
void initStars(int count)
{
    stars.clear();
    for (int i = 0; i < count; ++i)
    {
        stars.push_back({
            static_cast<float>(rand() % 800 - 400), // x between -400 and +400
            static_cast<float>(rand() % 600 - 300)  // y between -300 and +300
        });
    }
}

// Draws a filled circle at 'center' with given 'radius' and 'color'
void drawCircle(glm::vec2 center, float radius, glm::vec3 color, int segments = 40)
{
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_TRIANGLE_FAN);       // Start drawing the filled circle
    glVertex2f(center.x, center.y); // Circle center
    for (int i = 0; i <= segments; ++i)
    {
        float angle = 2.0f * M_PI * float(i) / float(segments);
        float x = center.x + cosf(angle) * radius;
        float y = center.y + sinf(angle) * radius;
        glVertex2f(x, y); // Perimeter points
    }
    glEnd();
}

// Renders all stars (white dots in the background)
void renderStars()
{
    glPointSize(1.0f);
    glBegin(GL_POINTS);
    glColor3f(1.0f, 1.0f, 1.0f); // White color
    for (const auto &star : stars)
        glVertex2f(star.x, star.y);
    glEnd();
}

// Applies gravity and updates velocity and position of all bodies
void updatePhysics(std::vector<Body> &bodies, float dt)
{
    const float G = 0.1f; // Gravitational constant (scaled down for visuals)

    for (size_t i = 0; i < bodies.size(); ++i)
    {
        glm::vec2 force(0.0f);

        for (size_t j = 0; j < bodies.size(); ++j)
        {
            if (i == j)
                continue; // Skip self

            glm::vec2 dir = bodies[j].position - bodies[i].position;
            float distSq = glm::dot(dir, dir) + 1.0f; // Add 1 to prevent divide by zero

            // Newton's law of universal gravitation
            force += G * bodies[i].mass * bodies[j].mass / distSq * glm::normalize(dir);
        }

        // Apply acceleration (F = ma => a = F/m)
        bodies[i].velocity += (force / bodies[i].mass) * dt;
    }

    // Update positions based on new velocity
    for (auto &b : bodies)
    {
        b.position += b.velocity * dt;

        // Save position for trail rendering
        b.trail.push_back(b.position);
        if (b.trail.size() > 500)
            b.trail.erase(b.trail.begin());
    }
}

// Renders all celestial bodies and their orbit trails
void renderBodies(const std::vector<Body> &bodies)
{
    for (size_t i = 0; i < bodies.size(); ++i)
    {
        // Yellow sun, blue planet
        glm::vec3 color = (i == 0) ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.7f, 1.0f);
        drawCircle(bodies[i].position, bodies[i].radius, color);

        // Draw trail
        glColor3f(0.7f, 0.7f, 0.7f);
        glBegin(GL_LINE_STRIP);
        for (auto &p : bodies[i].trail)
            glVertex2f(p.x, p.y);
        glEnd();
    }
}

int main()
{
    srand(static_cast<unsigned>(time(nullptr))); // Random seed for stars

    // Initialize GLFW
    if (!glfwInit())
        return -1;

    // Create window
    GLFWwindow *window = glfwCreateWindow(800, 600, "Gravity Sim", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    glewInit();

    // Set up 2D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-400, 400, -300, 300, -1, 1); // X: -400 to 400, Y: -300 to 300
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initStars(200); // Create 200 background stars

    // Create a sun and one orbiting planet
    std::vector<Body> bodies = {
        {{0.0f, 0.0f}, {0.0f, 0.0f}, 1000.0f, 10.0f}, // Sun
        {{150.0f, 0.0f}, {0.0f, 55.0f}, 1.0f, 5.0f}   // Planet
    };

    float lastTime = glfwGetTime();

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
        glClear(GL_COLOR_BUFFER_BIT);

        updatePhysics(bodies, dt); // Simulate motion

        renderStars();        // Draw stars first
        renderBodies(bodies); // Then draw planets + trails

        glfwSwapBuffers(window); // Display the frame
        glfwPollEvents();        // Check for input/events
    }

    glfwTerminate();
    return 0;
}