#include <GL/glew.h>                    // GLEW helps load OpenGL extensions, must be included before GLFW
#include <GLFW/glfw3.h>                 // GLFW handles window creation and input
#include <glm/glm.hpp>                  // GLM for math (not directly used here yet)
#include <glm/gtc/matrix_transform.hpp> // GLM matrix transforms (not directly used)
#include <glm/gtc/type_ptr.hpp>         // Convert GLM types to raw pointers (not directly used)

#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <cmath> // Needed for sine and cosine functions

float screenWidth = 800.0f;
float screenHeight = 600.0f;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.05f;

GLFWwindow *StartGLFW();

std::string LoadShaderSource(const char *path)
{
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint CompileShader(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }
    return shader;
}

GLuint CreateShaderProgram(const char *vertPath, const char *fragPath)
{
    std::string vertSrc = LoadShaderSource(vertPath);
    std::string fragSrc = LoadShaderSource(fragPath);
    GLuint vert = CompileShader(GL_VERTEX_SHADER, vertSrc.c_str());
    GLuint frag = CompileShader(GL_FRAGMENT_SHADER, fragSrc.c_str());
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

int main()
{
    GLFWwindow *window = StartGLFW();
    if (!window)
        return -1;

    glEnable(GL_DEPTH_TEST);

    // --- Generate circle vertices in XY plane ---
    const int res = 100;
    std::vector<float> vertices = {0.0f, 0.0f, 0.0f}; // center
    float radius = 1.0f;
    for (int i = 0; i <= res; ++i)
    {
        float angle = 2.0f * 3.1415926535f * (float(i) / res);
        vertices.push_back(cos(angle) * radius);
        vertices.push_back(sin(angle) * radius);
        vertices.push_back(0.0f);
    }

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // --- Load shaders ---
    GLuint shader = CreateShaderProgram("Shaders/vertex.glsl", "Shaders/fragment.glsl");

    // --- Camera setup ---
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), screenWidth / screenHeight, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0, 0, 5), // Camera position
        glm::vec3(0, 0, 0), // Look at origin
        glm::vec3(0, 1, 0)  // Up
    );
    glm::mat4 model = glm::mat4(1.0f);

    while (!glfwWindowShouldClose(window))
    {
        // --- Camera movement ---
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        // Set uniforms
        GLint modelLoc = glGetUniformLocation(shader, "model");
        GLint viewLoc = glGetUniformLocation(shader, "view");
        GLint projLoc = glGetUniformLocation(shader, "projection");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, res + 2);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);

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

    glewExperimental = GL_TRUE;

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return nullptr;
    }

    glViewport(0, 0, 800, 600);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    return window;
}