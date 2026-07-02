#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

#include "include.h"
#include "model.h"
#include "shader.h"
#include "objImport.h"
#include "glm/gtc/type_ptr.hpp"

int WINDOW_WIDTH = 640;
int WINDOW_HEIGHT = 480;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, float dt);

Shader pathtraceProgram;
Shader displayProgram;
GLuint textures[2];

float MOVEMENT_SPEED = 1.0;
float ROTATION_SPEED = 1.0;
vec3 cameraPos = vec3(-0.8, -0.4, 2.5);
float cameraPitch = 0.0;
float cameraYaw = 0.0;
#define RIGHT vec3(1.0, 0.0, 0.0)
#define UP vec3(0.0, 1.0, 0.0)
#define FORWARD vec3(0.0, 0.0, -1.0)
vec3 cameraRight = RIGHT;
vec3 cameraUp = UP;
vec3 cameraForward = FORWARD;
mat3 cameraRotation = mat3(RIGHT, UP, -FORWARD);

uint maxBounces = 3;
uint samples = 1;
uint currentFrame = 0;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Pathtracer", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    pathtraceProgram = Shader(RESOURCES_PATH "shaders/pathtrace.vert", RESOURCES_PATH "shaders/pathtrace.frag");

    displayProgram = Shader(RESOURCES_PATH "shaders/display.vert", RESOURCES_PATH "shaders/display.frag");

    float vertices[] = {
        1.0f,  1.0f,
        1.0f, -1.0f,
       -1.0f, -1.0f,
       -1.0f,  1.0f,
   };
    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3,
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    importModel(RESOURCES_PATH "models/obj/cube.obj", vec3(-2.0, 1.0, -1.5), 1.0, Material{vec3(1.0), 0.0, vec3(1.0), 10.0});
    addModel(0, 12, vec3(0.0, -6.0, 0.0), 5.0, Material{vec3(0.9, 0.1, 0.1), 0.0, vec3(0.0), 0.0});
    addModel(0, 12, vec3(0.0, -0.5, 0.0), 0.5, Material{vec3(0.1, 0.9, 0.1), 0.0, vec3(0.0), 0.0});
    std::cout << triangles.size() << std::endl;
    std::cout << models.size() << std::endl;

    GLuint triangle_ssbo;
    glGenBuffers(1, &triangle_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangle_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(Triangle), triangles.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, triangle_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint model_ssbo;
    glGenBuffers(1, &model_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, model_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, models.size() * sizeof(Model), models.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, model_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);

    glGenTextures(2, textures);
    for (GLuint texture : textures) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    double currentTime = glfwGetTime();
    double lastTime = currentTime;
    float deltaTime;

    while (!glfwWindowShouldClose(window)) {
        currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        processInput(window, deltaTime);


        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        GLuint currentFrameTexture = textures[currentFrame % 2];
        GLuint lastFrameTexture = textures[1 - (currentFrame % 2)];

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, currentFrameTexture, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

        pathtraceProgram.use();

        pathtraceProgram.setUniform1ui("currentFrame", currentFrame);

        pathtraceProgram.setUniform3f("cameraPos", cameraPos.x, cameraPos.y, cameraPos.z);
        pathtraceProgram.setUniformMatrix3fv("cameraRotation", 1, value_ptr(cameraRotation));

        pathtraceProgram.setUniform2ui("halfScreenSize", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

        pathtraceProgram.setUniform1ui("triangleCount", triangles.size());
        pathtraceProgram.setUniform1ui("modelCount", models.size());

        pathtraceProgram.setUniform1ui("maxBounces", maxBounces);
        pathtraceProgram.setUniform1ui("samples", samples);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, lastFrameTexture);
        pathtraceProgram.setUniform1i("lastFrame", 0);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        displayProgram.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentFrameTexture);
        displayProgram.setUniform1i("accumulatedTexture", 0);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        glfwSwapBuffers(window);
        glfwPollEvents();

        // glBindTexture(GL_TEXTURE_2D, 0);
        // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);

        currentFrame++;
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
    for (GLuint texture : textures) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    currentFrame = 0;
}


void processInput(GLFWwindow *window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    bool moved = false;
    bool rotated = false;

    vec3 forwardMovement =
        float((glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS))
        * normalize(vec3(cameraForward.x, 0.0, cameraForward.z)) * dt * MOVEMENT_SPEED;
    vec3 lateralMovement =
        float((glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS))
        * normalize(vec3(cameraRight.x, 0.0, cameraRight.z)) * dt * MOVEMENT_SPEED;
    vec3 verticalMovement =
        float((glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS))
        * vec3(0.0, 1.0, 0.0) * dt * MOVEMENT_SPEED;
    if (forwardMovement != vec3(0.0) || lateralMovement != vec3(0.0) || verticalMovement != vec3(0.0)) {
        moved = true;
        cameraPos += forwardMovement + lateralMovement + verticalMovement;
    }

    float pitchDelta =
        ((glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS))
        * 3.1415926f * dt * ROTATION_SPEED;
    float yawDelta =
        ((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS))
        * 3.1415926f * dt * ROTATION_SPEED;
    if (pitchDelta != 0.0 || yawDelta != 0.0) {
        rotated = true;
        cameraPitch += pitchDelta;
        cameraYaw += yawDelta;
        mat4 tmp = mat4(1.0);
        tmp = rotate(tmp, -cameraYaw, UP);
        tmp = rotate(tmp, cameraPitch, RIGHT);
        cameraRotation = mat3(tmp);
        cameraForward = cameraRotation * FORWARD;
        cameraRight = cameraRotation * RIGHT;
        cameraUp = cameraRotation * UP;
    }

    if (moved || rotated) {
        currentFrame = 0;
    }
}
