#ifndef PATHTRACER_SHADER_H
#define PATHTRACER_SHADER_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "include.h"

class Shader {
    public:
        unsigned int ID;

        Shader();
        Shader(const char* vertexPath, const char* fragmentPath);
        void use();

        void setUniform1f(const std::string &name, float value0);
        void setUniform2f(const std::string &name, float value0, float value1);
        void setUniform3f(const std::string &name, float value0, float value1, float value2);
        void setUniform4f(const std::string &name, float value0, float value1, float value2, float value3);

        void setUniformMatrix2fv(const std::string &name, int count, float *value);
        void setUniformMatrix3fv(const std::string &name, int count, float *value);
        void setUniformMatrix4fv(const std::string &name, int count, float *value);

        void setUniform1ui(const std::string &name, uint value0);
        void setUniform2ui(const std::string &name, uint value0, uint value1);
        void setUniform3ui(const std::string &name, uint value0, uint value1, uint value2);
        void setUniform4ui(const std::string &name, uint value0, uint value1, uint value2, uint value3);

        void setUniform1i(const std::string &name, int value0);
        void setUniform2i(const std::string &name, int value0, int value1);
        void setUniform3i(const std::string &name, int value0, int value1, int value2);
        void setUniform4i(const std::string &name, int value0, int value1, int value2, int value3);
};

#endif //PATHTRACER_SHADER_H