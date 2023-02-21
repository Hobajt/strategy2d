#pragma once

#include <memory>
#include <string>

#include "engine/utils/mathdefs.h"
#include "engine/utils/setup.h"

#include <glad/glad.h>

namespace eng {

    class Shader;
    using ShaderRef = std::shared_ptr<Shader>;

    //Wrapper for shader program management.
    class Shader {
    public:
        Shader(const std::string& filepath);
        Shader(const std::string& vertexFilepath, const std::string& fragmentFilepath);
        Shader(const std::string& vertexSource, const std::string& fragmentSource, const std::string& name);

        Shader() = default;
        ~Shader();

        //copy disabled
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        //move enabled
        Shader(Shader&&) noexcept;
        Shader& operator=(Shader&&) noexcept;

        void Bind() const;
	    static void Unbind();

        //=== Uniform manipulation methods ===

        void SetVec2(const char* varName, const glm::vec2& value) const;
        void SetVec3(const char* varName, const glm::vec3& value) const;
        void SetVec4(const char* varName, const glm::vec4& value) const;

        void SetMat2(const char* varName, const glm::mat2& value) const;
        void SetMat3(const char* varName, const glm::mat3& value) const;
        void SetMat4(const char* varName, const glm::mat4& value) const;

        void SetInt(const char* varName, int value) const;
        void SetBool(const char* varName, bool value) const;
        void SetFloat(const char* varName, float value) const;
        void SetUint(const char* varName, unsigned int value) const;

#ifdef ENGINE_BINDLESS_TEXTURES
        void SetARBHandle(const char* varName, uint64_t value) const;
#endif
        //Initializes textures[i] array in the shader to point to texture slot[i].
        //Required for shaders that are used in the Renderer class.
        void InitTextureSlots(int textureCount);
    private:
        void Release() noexcept;
        void Move(Shader&&) noexcept;
    private:
        GLuint program = 0;
        std::string name;

        DBGONLY(static GLuint activeProgram);
    };

}//namespace eng
