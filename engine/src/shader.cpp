#include "engine/core/shader.h"

#include "engine/utils/utils.h"

#define SHADER_VALIDATION_CHECK() ASSERT_MSG(program != 0, "Attempting to use uninitialized shader object ({}).", name.c_str())
#define ACTIVE_SHADER_VALIDATION_CHECK() DBGONLY(ASSERT_MSG(activeProgram == program, "Attempting to use unbound shader ({}), use shader.Bind() first.", name.c_str()))

namespace eng {

    //Compile a shader from provided glsl source code, returns the shader's handle.
    GLuint CompileSource(const char* source, GLenum shaderType);
    //Link vertex and fragment shader into a shader program.
    GLuint LinkProgram(GLuint vertex, GLuint fragment);

    //======= Shader =======

    DBGONLY(GLuint Shader::activeProgram = 0);

    Shader::Shader(const std::string& filepath) : Shader(filepath + ".vert", filepath + ".frag") {}

    Shader::Shader(const std::string& vertexFilepath, const std::string& fragmentFilepath)
        : Shader(ReadFile(vertexFilepath.c_str()), ReadFile(fragmentFilepath.c_str()), fragmentFilepath.substr(0, fragmentFilepath.size() - 5)) {}

    Shader::Shader(const std::string& vertexSource, const std::string& fragmentSource, const std::string& name_) : name(name_) {
        GLuint vertex = CompileSource(vertexSource.c_str(), GL_VERTEX_SHADER);
        GLuint fragment = CompileSource(fragmentSource.c_str(), GL_FRAGMENT_SHADER);

        program = LinkProgram(vertex, fragment);

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        ENG_LOG_TRACE("[R] Shader '{}' successfully compiled & linked.", name.c_str());
        ENG_LOG_TRACE("[C] Shader '{}' ({})", name.c_str(), program);
    }

    Shader::~Shader() {
        Release();
    }

    Shader::Shader(Shader&& s) noexcept {
        Move(std::move(s));
    }

    Shader& Shader::operator=(Shader&& s) noexcept {
        Release();
        Move(std::move(s));
        return *this;
    }

    void Shader::Bind() const {
        SHADER_VALIDATION_CHECK();
        DBGONLY(activeProgram = program);
        glUseProgram(program);
    }

    void Shader::Unbind() {
        DBGONLY(activeProgram = 0);
        glUseProgram(0);
    }

    void Shader::SetVec2(const char* varName, const glm::vec2& value) const {
        SHADER_VALIDATION_CHECK();
        ACTIVE_SHADER_VALIDATION_CHECK();
        glUniform2fv(glGetUniformLocation(program, varName), 1, (const GLfloat*)&value);
    }

    void Shader::SetVec3(const char* varName, const glm::vec3& value) const {
        SHADER_VALIDATION_CHECK();
        ACTIVE_SHADER_VALIDATION_CHECK();
        glUniform3fv(glGetUniformLocation(program, varName), 1, (const GLfloat*)&value);
    }

    void Shader::SetVec4(const char* varName, const glm::vec4& value) const {
        SHADER_VALIDATION_CHECK();
        ACTIVE_SHADER_VALIDATION_CHECK();
        glUniform4fv(glGetUniformLocation(program, varName), 1, (const GLfloat*)&value);
    }

    void Shader::SetMat2(const char* varName, const glm::mat2& value) const {
        SHADER_VALIDATION_CHECK();
        ACTIVE_SHADER_VALIDATION_CHECK();
        glUniformMatrix2fv(glGetUniformLocation(program, varName), 1, GL_FALSE, glm::value_ptr(value));
    }

    void Shader::SetMat3(const char* varName, const glm::mat3& value) const {
        SHADER_VALIDATION_CHECK();
        ACTIVE_SHADER_VALIDATION_CHECK();
        glUniformMatrix3fv(glGetUniformLocation(program, varName), 1, GL_FALSE, glm::value_ptr(value));
    }

    void Shader::SetMat4(const char* varName, const glm::mat4& value) const {
        SHADER_VALIDATION_CHECK();
        ACTIVE_SHADER_VALIDATION_CHECK();
        glUniformMatrix4fv(glGetUniformLocation(program, varName), 1, GL_FALSE, glm::value_ptr(value));
    }

    void Shader::SetInt(const char* varName, int value) const {
        SHADER_VALIDATION_CHECK();
        ACTIVE_SHADER_VALIDATION_CHECK();
        glUniform1i(glGetUniformLocation(program, varName), value);
    }

    void Shader::SetBool(const char* varName, bool value) const {
        SHADER_VALIDATION_CHECK();
        ACTIVE_SHADER_VALIDATION_CHECK();
        glUniform1i(glGetUniformLocation(program, varName), (int)value);
    }

    void Shader::SetFloat(const char* varName, float value) const {
        SHADER_VALIDATION_CHECK();
        ACTIVE_SHADER_VALIDATION_CHECK();
        glUniform1f(glGetUniformLocation(program, varName), value);
    }

    void Shader::SetUint(const char* varName, unsigned int value) const {
        SHADER_VALIDATION_CHECK();
        ACTIVE_SHADER_VALIDATION_CHECK();
        glUniform1ui(glGetUniformLocation(program, varName), value);
    }

#ifdef ENGINE_BINDLESS_TEXTURES
    void Shader::SetARBHandle(const char* varName, uint64_t value) const {
        SHADER_VALIDATION_CHECK();
        ACTIVE_SHADER_VALIDATION_CHECK();
        glUniformHandleui64ARB(glGetUniformLocation(program, varName), value);
    }
#endif

    void Shader::InitTextureSlots(int textureCount) {
        char buf[256];
        Bind();
        for(int i = 0; i < textureCount; i++) {
            snprintf(buf, sizeof(buf), "textures[%d]", i);
            SetInt(buf, i);
        }
    }

    void Shader::Release() noexcept {
        if (program != 0) {
            ENG_LOG_TRACE("[D] Shader '{}' ({})\n", name.c_str(), program);
            glDeleteProgram(program);
            program = 0;
        }
    }

    void Shader::Move(Shader&& s) noexcept {
        program = s.program;
        name = s.name;

        s.program = 0;
    }

    //============================================
    
    GLuint CompileSource(const char* source, GLenum shaderType) {
        const char* shaderTypeStr = (shaderType == GL_FRAGMENT_SHADER) ? "Fragment" : "Vertex";

        GLuint shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        int success;
        char infoLog[512];

        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
            ENG_LOG_WARN("Shader::CompileSource - {} shader failed to compile: {}", shaderTypeStr, infoLog);
            throw std::exception();
        }

        ENG_LOG_TRACE("{} shader successfully compiled.", shaderTypeStr);
        return shader;
    }

    GLuint LinkProgram(GLuint vertex, GLuint fragment) {
        GLuint shader = glCreateProgram();
        glAttachShader(shader, vertex);
        glAttachShader(shader, fragment);
        glLinkProgram(shader);

        int success;
        char infoLog[512];

        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, sizeof(infoLog), NULL, infoLog);
            ENG_LOG_WARN("Shader::LinkProgram - Program failed to link: {}", infoLog);
            glDeleteProgram(shader);
            throw std::exception();
        }

        ENG_LOG_TRACE("Shader successfully linked.");
        return shader;
    }

}//namespace eng
