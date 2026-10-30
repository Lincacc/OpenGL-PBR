#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <stdexcept>

struct Shader {
    GLuint programHandle = 0;

    void bind() {
        glUseProgram(programHandle);
    }

    void setMatrix4(const std::string& name, const glm::mat4& matrix) {
        glUniformMatrix4fv(glGetUniformLocation(programHandle, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void setVec3f(const std::string& name, const glm::vec3& vec) {
        glUniform3fv(glGetUniformLocation(programHandle, name.c_str()), 1, glm::value_ptr(vec));
    }

    void setVec3Array(const std::string& name, const glm::vec3* vecs, int count) {
        if (!vecs || count <= 0)
            return;
        glUniform3fv(glGetUniformLocation(programHandle, name.c_str()), count, glm::value_ptr(vecs[0]));
    }

    void setVec2f(const std::string& name, const glm::vec2& vec) {
        glUniform2fv(glGetUniformLocation(programHandle, name.c_str()), 1, glm::value_ptr(vec));
    }

    void setFloat(const std::string& name, float value) {
        glUniform1f(glGetUniformLocation(programHandle, name.c_str()), value);
    }

    void setInt(const std::string& name, int value) {
        glUniform1i(glGetUniformLocation(programHandle, name.c_str()), value);
    }

	void compile(const std::string& vCode, const std::string& fCode) {
		GLuint vShaderHandle = glCreateShader(GL_VERTEX_SHADER);
        if (vShaderHandle == 0) {
            throw std::runtime_error("glCreateShader(GL_VERTEX_SHADER) failed");
        }
		const char* source = vCode.c_str();
		glShaderSource(vShaderHandle, 1, &source, nullptr);
		glCompileShader(vShaderHandle);
        check_shader(vShaderHandle, "vertex");

        GLuint fShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
        if (fShaderHandle == 0) {
            throw std::runtime_error("glCreateShader(GL_FRAGMENT_SHADER) failed");
        }
        source = fCode.c_str();
        glShaderSource(fShaderHandle, 1, &source, nullptr);
        glCompileShader(fShaderHandle);
        check_shader(fShaderHandle, "fragment");

        programHandle = glCreateProgram();
        if (programHandle == 0) {
            throw std::runtime_error("glCreateProgram failed");
        }

        glAttachShader(programHandle, vShaderHandle);
        glAttachShader(programHandle, fShaderHandle);
        glLinkProgram(programHandle);
        check_program(programHandle);

        glDetachShader(programHandle, vShaderHandle);
        glDetachShader(programHandle, fShaderHandle);
        glDeleteShader(vShaderHandle);
        glDeleteShader(fShaderHandle);
	}

	void check_shader(GLuint handle, const char* stage) {
        GLint ok = 0;
        glGetShaderiv(handle, GL_COMPILE_STATUS, &ok);
        if (ok)
            return;
        GLint log_len = 0;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_len);
        std::string log;
        if (log_len > 0) {
            std::vector<char> buf(static_cast<size_t>(log_len));
            glGetShaderInfoLog(handle, log_len, nullptr, buf.data());
            log = buf.data();
        }
        throw std::runtime_error(std::string(stage) + " compile failed:\n" + log);
	}

    void check_program(GLuint handle) {
        GLint ok = 0;
        glGetProgramiv(handle, GL_LINK_STATUS, &ok);
        if (ok)
            return;
        GLint log_len = 0;
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_len);
        std::string log;
        if (log_len > 0) {
            std::vector<char> buf(static_cast<size_t>(log_len));
            glGetProgramInfoLog(handle, log_len, nullptr, buf.data());
            log = buf.data();
        }
        throw std::runtime_error("program link failed:\n" + log);
    }
};