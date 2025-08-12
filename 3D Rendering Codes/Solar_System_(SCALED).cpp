#include <GL/glew.h>                    // GLEW helps load OpenGL extensions, must be included before GLFW
#include <GLFW/glfw3.h>                 // GLFW handles window creation and input
#include <glm/glm.hpp>                  // GLM for math
#include <glm/gtc/matrix_transform.hpp> // GLM matrix transforms
#include <glm/gtc/type_ptr.hpp>         // Convert GLM types to raw pointers

#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath> // Needed for sine and cosine functions
#include <string>
#include <random>

// Window dimensions
int screenWidth = 1024;
int screenHeight = 768;

// Camera variables
glm::vec3 cameraPos = glm::vec3(10.0f, 5.0f, 10.0f);
glm::vec3 cameraFront = glm::vec3(-0.7f, -0.3f, -0.7f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Mouse control variables
float lastX = screenWidth / 2.0f;
float lastY = screenHeight / 2.0f;
float yaw = -135.0f;
float pitch = -20.0f;
bool firstMouse = true;
bool mousePressed = false;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Simulation control
bool isPaused = false;
float simulationSpeed = 1.0f;

// ===== Constants =====
const double AU_METERS = 149597870700.0; // meters in 1 AU
const double G = 1.993560809749174e-44;  // Gravitational constant in AU^3 kg^-1 s^-2
const float RADIUS_SCALE = 0.005f;       // purely for visual scaling

// Helper to convert km/s to AU/s
inline double kmps_to_AUps(double kmps)
{
    return (kmps * 1000.0) / AU_METERS;
}

// Shader source code //

// Vertex shader source code
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

// Fragment shader source code
const char *fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

void main()
{
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting
    float specularStrength = 0.8;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

// Trail shader for orbit visualisation //

// Trail vertex shader
const char *trailVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

// Trail fragment shader
const char *trailFragmentShader = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 color;
uniform float alpha;

void main()
{
    FragColor = vec4(color, alpha);
}
)";

// Function declarations
GLFWwindow *StartGLFW();
unsigned int CreateShaderProgram(const char *vertexSource, const char *fragmentSource);
void GenerateSphere(std::vector<float> &vertices, std::vector<unsigned int> &indices, float radius, int sectors, int stacks);
void ProcessInput(GLFWwindow *window);
void MouseCallback(GLFWwindow *window, double xpos, double ypos);
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

// Object class to represent celestial bodies
class Object3D
{
public:
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 color;

    float radius;
    float mass;

    std::vector<glm::vec3> trail;

    const int maxTrailLength = 500;

    Object3D(glm::vec3 pos, glm::vec3 vel, float m, glm::vec3 col, float r = 0.5f)
    {
        position = pos;
        velocity = vel;
        acceleration = glm::vec3(0.0f);
        mass = m;
        color = col;
        radius = r;
    }

    void applyForce(glm::vec3 force)
    {
        acceleration += force / mass;
    }

    void updatePosition(float dt)
    {
        if (isPaused)
            return;

        // Update velocity and position using Verlet integration for better stability
        velocity += acceleration * dt;
        position += velocity * dt;

        // Add position to trail
        trail.push_back(position);
        if (trail.size() > maxTrailLength)
        {
            trail.erase(trail.begin());
        }

        // Reset acceleration for next frame
        acceleration = glm::vec3(0.0f);
    }

    void calculateGravitationalForce(Object3D &other)
    {
        if (&other == this)
            return;

        glm::vec3 direction = other.position - position;
        float distance = glm::length(direction);

        // Prevent division by zero and extreme forces at close distances
        distance = std::max(distance, radius + other.radius);

        // Newton's law of universal gravitation: F = G * m1 * m2 / r^2
        float forceMagnitude = G * mass * other.mass / (distance * distance);

        // Normalize direction and apply force
        glm::vec3 force = glm::normalize(direction) * forceMagnitude;
        applyForce(force);
    }
};

// Create initial objects with interesting orbital configurations
std::vector<Object3D> CreateObjects()
{
    std::vector<Object3D> objects;

    // ===== Sun =====
    objects.emplace_back(
        glm::dvec3(0.0, 0.0, 0.0),   // position in AU
        glm::dvec3(0.0, 0.0, 0.0),   // velocity in AU/s
        1.989e30,                    // mass (kg)
        glm::vec3(1.0f, 0.9f, 0.3f), // color
        65.0f * RADIUS_SCALE         // visual radius
    );

    // ===== Mercury =====
    objects.emplace_back(
        glm::dvec3(0.387, 0.0, 0.0),               // position in AU
        glm::dvec3(0.0, kmps_to_AUps(47.36), 0.0), // velocity in AU/s
        3.3011e23,                                 // mass (kg)
        glm::vec3(0.7f, 0.4f, 0.2f),               // color (brown)
        0.383f * RADIUS_SCALE                      // visual radius
    );

    // ===== Venus =====
    objects.emplace_back(
        glm::dvec3(0.723, 0.0, 0.0),               // position in AU
        glm::dvec3(0.0, kmps_to_AUps(35.02), 0.0), // velocity in AU/s
        4.8675e24,                                 // mass (kg)
        glm::vec3(0.9f, 0.7f, 0.4f),               // color (brown)
        0.949f * RADIUS_SCALE                      // visual radius
    );

    // ===== Earth =====
    objects.emplace_back(
        glm::dvec3(1.0, 0.0, 0.0),                 // position in AU
        glm::dvec3(0.0, kmps_to_AUps(29.78), 0.0), // velocity in AU/s
        5.97237e24,                                // mass (kg)
        glm::vec3(0.2f, 0.4f, 0.7f),               // color (blue)
        1.0f * RADIUS_SCALE                        // visual radius
    );

    // ===== Mars =====
    objects.emplace_back(
        glm::dvec3(1.524, 0.0, 0.0),               // position in AU
        glm::dvec3(0.0, kmps_to_AUps(24.07), 0.0), // velocity in AU/s
        6.4171e23,                                 // mass (kg)
        glm::vec3(0.9f, 0.5f, 0.3f),               // color (brown)
        0.532f * RADIUS_SCALE                      // visual radius
    );

    // ===== Jupiter =====
    objects.emplace_back(
        glm::dvec3(5.203, 0.0, 0.0),               // position in AU
        glm::dvec3(0.0, kmps_to_AUps(13.07), 0.0), // velocity in AU/s
        1.8982e27,                                 // mass (kg)
        glm::vec3(1.0f, 0.5f, 0.2f),               // color (orange)
        11.21f * RADIUS_SCALE                      // visual radius
    );

    // ===== Saturn =====
    objects.emplace_back(
        glm::dvec3(9.537, 0.0, 0.0),              // position in AU
        glm::dvec3(0.0, kmps_to_AUps(9.69), 0.0), // velocity in AU/s
        5.6834e26,                                // mass (kg)
        glm::vec3(0.8f, 0.7f, 0.6f),              // color (light brown)
        9.45f * RADIUS_SCALE                      // visual radius
    );

    // ===== Uranus =====
    objects.emplace_back(
        glm::dvec3(19.191, 0.0, 0.0),             // position in AU
        glm::dvec3(0.0, kmps_to_AUps(6.81), 0.0), // velocity in AU/s
        8.6810e25,                                // mass (kg)
        glm::vec3(0.6f, 0.8f, 0.9f),              // color (light blue)
        4.01f * RADIUS_SCALE                      // visual radius
    );

    // ===== Neptune =====
    objects.emplace_back(
        glm::dvec3(30.07, 0.0, 0.0),              // position in AU
        glm::dvec3(0.0, kmps_to_AUps(5.43), 0.0), // velocity in AU/s
        1.02413e26,                               // mass (kg)
        glm::vec3(0.3f, 0.5f, 0.9f),              // color (light blue)
        3.88f * RADIUS_SCALE                      // visual radius
    );

    return objects;
}

// Main function to initialize GLFW, create window, and start rendering
int main()
{
    GLFWwindow *window = StartGLFW();
    if (!window)
        return -1;

    // Set callbacks
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);

    // Create shader programs
    unsigned int shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
    unsigned int trailShader = CreateShaderProgram(trailVertexShader, trailFragmentShader);

    // Generate sphere mesh
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    GenerateSphere(sphereVertices, sphereIndices, 1.0f, 36, 18);

    // Create VAO, VBO, EBO for sphere
    unsigned int sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Create VAO and VBO for trails
    unsigned int trailVAO, trailVBO;
    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);

    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Enable depth testing and blending for trails
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(2.0f);

    // Create objects
    std::vector<Object3D> objects = CreateObjects();

    // Light position (moves around for dynamic lighting)
    float lightAngle = 0.0f;

    // Main render loop
    while (!glfwWindowShouldClose(window))
    {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        ProcessInput(window);

        // Update physics
        if (!isPaused)
        {
            // Calculate gravitational forces between all objects
            for (size_t i = 0; i < objects.size(); ++i)
            {
                for (size_t j = 0; j < objects.size(); ++j)
                {
                    objects[i].calculateGravitationalForce(objects[j]);
                }
            }

            // Update positions
            for (auto &obj : objects)
            {
                obj.updatePosition(deltaTime * simulationSpeed);
            }

            // Animate light
            lightAngle += deltaTime * 0.5f;
        }

        // Dynamic light position
        glm::vec3 lightPos(10.0f * cos(lightAngle), 5.0f, 10.0f * sin(lightAngle));

        // Clear the screen
        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set up view and projection matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

        // Render trails first (without depth writing)
        glDepthMask(GL_FALSE);
        glUseProgram(trailShader);
        glUniformMatrix4fv(glGetUniformLocation(trailShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(trailShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        for (const auto &obj : objects)
        {
            if (obj.trail.size() > 1)
            {
                glBindVertexArray(trailVAO);
                glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
                glBufferData(GL_ARRAY_BUFFER, obj.trail.size() * sizeof(glm::vec3),
                             obj.trail.data(), GL_DYNAMIC_DRAW);

                // Fade trail based on position in trail
                glUniform3fv(glGetUniformLocation(trailShader, "color"), 1, glm::value_ptr(obj.color));
                glUniform1f(glGetUniformLocation(trailShader, "alpha"), 0.3f);

                glDrawArrays(GL_LINE_STRIP, 0, obj.trail.size());
                glBindVertexArray(0);
            }
        }
        glDepthMask(GL_TRUE);

        // Render spheres
        glUseProgram(shaderProgram);

        // Send view and projection matrices (same for all objects)
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Send lighting uniforms
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));

        // Draw each object
        for (const auto &obj : objects)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::scale(model, glm::vec3(obj.radius));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(obj.color));

            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteVertexArrays(1, &trailVAO);
    glDeleteBuffers(1, &trailVBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(trailShader);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// Function to initialize GLFW, create window, and set up OpenGL context
GLFWwindow *StartGLFW()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialise GLFW" << std::endl;
        return nullptr;
    }

    // Set OpenGL version to 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "3D Space Engine - Gravitational Orbits", NULL, NULL);
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

    glViewport(0, 0, screenWidth, screenHeight);

    std::cout << "=== 3D Space Engine Controls ===" << std::endl;
    std::cout << "WASD: Move camera" << std::endl;
    std::cout << "Q/E: Move up/down" << std::endl;
    std::cout << "Mouse (hold left): Look around" << std::endl;
    std::cout << "Scroll: Zoom" << std::endl;
    std::cout << "Space: Pause/unpause" << std::endl;
    std::cout << "R: Reset simulation" << std::endl;
    std::cout << "ESC: Exit" << std::endl;
    std::cout << "================================" << std::endl;

    return window;
}

// Function to create and compile shader program
unsigned int CreateShaderProgram(const char *vertexSource, const char *fragmentSource)
{
    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Check vertex shader compilation
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation failed:\n"
                  << infoLog << std::endl;
    }

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation failed:\n"
                  << infoLog << std::endl;
    }

    // Link shaders to program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check linking
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking failed:\n"
                  << infoLog << std::endl;
    }

    // Delete shaders as they're now linked
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Function to generate a sphere mesh for rendering
void GenerateSphere(std::vector<float> &vertices, std::vector<unsigned int> &indices, float radius, int sectors, int stacks)
{
    float x, y, z, xy;
    float nx, ny, nz, lengthInv = 1.0f / radius;
    float s, t;

    float sectorStep = 2 * M_PI / sectors;
    float stackStep = M_PI / stacks;
    float sectorAngle, stackAngle;

    // Generate vertices
    for (int i = 0; i <= stacks; ++i)
    {
        stackAngle = M_PI / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j)
        {
            sectorAngle = j * sectorStep;

            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }

    // Generate indices
    int k1, k2;
    for (int i = 0; i < stacks; ++i)
    {
        k1 = i * (sectors + 1);
        k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stacks - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

// Process input for camera movement and controls
void ProcessInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 5.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cameraPos -= cameraUp * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos += cameraUp * cameraSpeed;
}

// Mouse callback to handle camera rotation
void MouseCallback(GLFWwindow *window, double xpos, double ypos)
{
    if (!mousePressed)
        return;

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

// Mouse button callback to handle mouse press/release
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            mousePressed = true;
            firstMouse = true;
        }
        else if (action == GLFW_RELEASE)
        {
            mousePressed = false;
        }
    }
}

// Scroll callback to handle zooming
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    float zoomSpeed = 1.0f;
    cameraPos += cameraFront * (float)yoffset * zoomSpeed;
}

// Key callback to handle simulation controls
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_SPACE)
        {
            isPaused = !isPaused;
            std::cout << (isPaused ? "Simulation paused" : "Simulation resumed") << std::endl;
        }
        else if (key == GLFW_KEY_R)
        {
            // Reset simulation by recreating objects
            std::vector<Object3D> *objects = (std::vector<Object3D> *)glfwGetWindowUserPointer(window);
            if (objects)
            {
                *objects = CreateObjects();
                std::cout << "Simulation reset" << std::endl;
            }
        }
    }
}