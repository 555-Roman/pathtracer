#ifndef PATHTRACER_SHADER_H
#define PATHTRACER_SHADER_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
    public:
        unsigned int ID;

        Shader(const char* vertexPath, const char* fragmentPath);
        void use();
        void setUniform1f(const std::string &name, float value0);
        void setUniform2f(const std::string &name, float value0, float value1);
        void setUniform3f(const std::string &name, float value0, float value1, float value2);
        void setUniform4f(const std::string &name, float value0, float value1, float value2, float value3);
};

#endif //PATHTRACER_SHADER_H