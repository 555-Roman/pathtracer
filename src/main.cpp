#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

#include "include.h"
#include "model.h"
#include "shader.h"
#include "import.h"
#include "skybox.h"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image/stb_image.h"

int WINDOW_WIDTH = 640;
int WINDOW_HEIGHT = 480;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, float dt);

Shader pathtraceProgram;
Shader displayProgram;
GLuint textures[2];

#define RIGHT vec3(1.0, 0.0, 0.0)
#define UP vec3(0.0, 1.0, 0.0)
#define FORWARD vec3(0.0, 0.0, -1.0)
float MOVEMENT_SPEED = 10.0;
float ROTATION_SPEED = 1.0;
vec3 cameraPos = vec3(14.2282, 18.136, 2.55665);
float cameraPitch = -0.484159;
float cameraYaw = -1.42489;
vec3 cameraRight = vec3(0.145391, 0, -0.989374);
vec3 cameraUp = vec3(-0.460518, 0.885067, -0.067674);
vec3 cameraForward = vec3(-0.875662, -0.465464, -0.12868);
mat3 cameraRotation = mat3(cameraRight, cameraUp, -cameraForward);
// float fov = 39.5978;
float fov = 90.0;

uint maxBounces = 12;
uint samples = 1;
uint currentFrame = 0;

bool benchmark = false;
float benchmarkStart = 0.0f;
#define BENCHMARK_STEPS 360.0f
#define BENCHMARK_DISTANCE 16.0f
#define BENCHMARK_CENTER vec3(0.0, 6.0, 0.0)
uint displayDebug = 0;

int main() {
    stbi_set_flip_vertically_on_load(true);

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
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    if (!GLAD_GL_ARB_bindless_texture) {
        std::cout << "for now only bindless-texture capable gpus are supported" << std::endl;
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


    import(RESOURCES_PATH "models/tests/sponza/sponza.obj");

    std::cout << models.size() << std::endl;
    std::cout << bvhNodes.size() << std::endl;
    std::cout << std::endl;

    // setSkyboxEquirectangular(RESOURCES_PATH "textures/rogland_clear_night_4k.png");


    GLuint triangle_ssbo;
    glGenBuffers(1, &triangle_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangle_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(Triangle), triangles.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, triangle_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint bvhNode_ssbo;
    glGenBuffers(1, &bvhNode_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhNode_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bvhNodes.size() * sizeof(BVHNode), bvhNodes.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bvhNode_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint model_ssbo;
    glGenBuffers(1, &model_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, model_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, models.size() * sizeof(Model), models.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, model_ssbo);
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
    double deltaTime;

    GLuint query;
    glGenQueries(1, &query);

    while (!glfwWindowShouldClose(window)) {
        currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;


        glfwPollEvents();


        if (benchmark) {
            if (float(currentFrame) / BENCHMARK_STEPS >= 1.0) {
                benchmark = false;
                currentFrame = 0;
                continue;
            }

            float x, z;
            x = BENCHMARK_DISTANCE * sin(float(currentFrame) / BENCHMARK_STEPS * 2.0f * 3.1415926f);
            z = BENCHMARK_DISTANCE * cos(float(currentFrame) / BENCHMARK_STEPS * 2.0f * 3.1415926f);

            cameraPos = vec3(x, 0.0, z) + BENCHMARK_CENTER;
            cameraForward = normalize(BENCHMARK_CENTER - cameraPos);
            cameraRight = normalize(cross(cameraForward, vec3(0.0, 1.0, 0.0)));
            cameraUp = normalize(cross(cameraRight, cameraForward));
            cameraRotation = mat3(cameraRight, cameraUp, -cameraForward);

            glBeginQuery(GL_TIME_ELAPSED, query);
        } else {
            processInput(window, deltaTime);
        }

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
        pathtraceProgram.setUniform1f("fov", fov);

        pathtraceProgram.setUniform1ui("modelCount", models.size());

        pathtraceProgram.setUniform1ui("maxBounces", maxBounces);
        pathtraceProgram.setUniform1ui("samples", samples);

        pathtraceProgram.setUniformHandleui64ARB("skyboxCubemapTexture", skyboxCubemapTexture);
        pathtraceProgram.setUniformHandleui64ARB("skyboxEquirectangularTexture", skyboxEquirectangularTexture);
        pathtraceProgram.setUniform1ui("skyboxFormat", skyboxFormat);

        pathtraceProgram.setUniform1ui("displayDebug", displayDebug);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, lastFrameTexture);
        pathtraceProgram.setUniform1i("lastFrame", 0);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        if (benchmark) {
            glEndQuery(GL_TIME_ELAPSED);

            GLuint64 ns;
            glGetQueryObjectui64v(query, GL_QUERY_RESULT, &ns);

            std::cout << float(currentFrame) / BENCHMARK_STEPS * 360.0f << ": " << ns / 1e6 << "  " << 1000.0 / (ns / 1e6) << std::endl;
        }


        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        displayProgram.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentFrameTexture);
        displayProgram.setUniform1i("accumulatedTexture", 0);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        glfwSwapBuffers(window);


        currentFrame++;
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

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


    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        std::cout << "vec3 cameraPos = vec3(" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ");" << std::endl;
        std::cout << "float cameraPitch = " << cameraPitch << ";" << std::endl;
        std::cout << "float cameraYaw = " << cameraYaw << ";" << std::endl;
        std::cout << "vec3 cameraRight = vec3(" << cameraRight.x << ", " << cameraRight.y << ", " << cameraRight.z << ");" << std::endl;
        std::cout << "vec3 cameraUp = vec3(" << cameraUp.x << ", " << cameraUp.y << ", " << cameraUp.z << ");" << std::endl;
        std::cout << "vec3 cameraForward = vec3(" << cameraForward.x << ", " << cameraForward.y << ", " << cameraForward.z << ");" << std::endl;
        std::cout << std::endl;
    }

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        y = WINDOW_HEIGHT - y;
        float pixel[4];
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, pixel);
        std::cout << "lower-left  " << x << " " << y << ": " << pixel[0] << " " << pixel[1] << " " << pixel[2] << std::endl;
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        benchmark = true;
        benchmarkStart = glfwGetTime();
        currentFrame = 0;
    }

    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        displayDebug = !displayDebug;
        currentFrame = 0;
    }
}
