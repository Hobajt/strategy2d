#include "engine/core/renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "engine/utils/setup.h"
#include "engine/core/texture.h"

namespace eng {

    namespace Renderer {

        //TODO: move somewhere and make it modifiable
        static constexpr int BATCH_SIZE = 1000;

        //TODO: maybe query from the driver... glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
        static constexpr int MAX_TEXTURE_COUNT = 8;

        //Looks up texture's slot (or assigns new one if the texture doesn't have one).
        //Can also triggers Flush(), if there isn't any space left for additional texture.
        uint32_t ResolveTextureIdx(const TextureRef& tex);

        //Carries the internal renderer state.
        struct RendererInstance {
            GLuint vao = 0;
            GLuint vbo = 0;
            GLuint ebo = 0;

            ShaderRef shader = nullptr;
            FramebufferRef fbo = nullptr;

            QuadVertices* quadBuffer = nullptr;
            QuadIndices* indexBuffer = nullptr;

            RenderStats stats = {};
            bool inProgress = false;

            int idx = 0;

            int texIdx = 1;
            int texIdxEnd = MAX_TEXTURE_COUNT-1;

            TextureRef textures[MAX_TEXTURE_COUNT];
            TextureRef blankTexture = nullptr;

            int lastFlush_wastedQuads = 0;
        public:
            RendererInstance() = default;
            ~RendererInstance();

            void Initialize();
            bool IsInitialized() const;
        };

        static RendererInstance instance = {};

        //========== Renderer interface functions ==========

        const RenderStats& Stats() {
            return instance.stats;
        }

        void StatsReset() {
            instance.stats = {};
        }

        int TextureSlotsCount() {
            return MAX_TEXTURE_COUNT;
        }

        //======================

        void Initialize() {
            if(!instance.IsInitialized())
                instance.Initialize();
        }

        void Release() {
            instance = {};
        }

        //======================

        void Begin(const ShaderRef& shader, bool clearFBO, const FramebufferRef& fbo) {
            Begin(shader, fbo, clearFBO);
        }

        void Begin(const ShaderRef& shader, const FramebufferRef& fbo, bool clearFBO) {
            if (instance.inProgress) {
                ENG_LOG_WARN("Renderer - Multiple Begin() calls without calling End().");
            }

            instance.inProgress = true;

            //first call -> initialize the renderer
            if(!instance.IsInitialized())
                instance.Initialize();

            if(shader == nullptr) {
                ENG_LOG_ERROR("Renderer requires a valid shader in order to render.");
                throw std::exception();
            }
            instance.shader = shader;

            //reset all texture slots to blank texture
            for (int i = 0; i < MAX_TEXTURE_COUNT; i++) {
                instance.textures[i] = instance.blankTexture;
            }
            instance.idx = 0;
            instance.texIdx = 1;
            instance.texIdxEnd = MAX_TEXTURE_COUNT - 1;
            instance.lastFlush_wastedQuads = 0;

            //bind fbo & clear; move to Flush() maybe? (in case some fbo switching happens during the render)
            instance.fbo = fbo;
            if(instance.fbo != nullptr)
                instance.fbo->Bind();
            else
                Framebuffer::Unbind();

            if(clearFBO) {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
        }

        void End() {
            if (!instance.inProgress) {
                ENG_LOG_WARN("Renderer - Multiple End() calls without Begin().");
            }
            instance.inProgress = false;

            Flush();
            // ENG_LOG_INFO("Draw calls: {}", instance.stats.drawCalls);
        }

        void Flush() {
            if (instance.idx > 0) {
                ASSERT_MSG(instance.shader != nullptr, "\tRenderer requires shader to function.\n");

                glEnable(GL_PRIMITIVE_RESTART);
                glPrimitiveRestartIndex((unsigned int)-1);

                instance.shader->Bind();
                glBindVertexArray(instance.vao);

                //upload quad vertex buffer
                glBindBuffer(GL_ARRAY_BUFFER, instance.vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(QuadVertices) * instance.idx, instance.quadBuffer);

                //upload quad index buffer
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, instance.ebo);
                glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(QuadIndices) * instance.idx, instance.indexBuffer);

                //bind all used textures into proper slots
                // ENG_LOG_INFO("Draw call textures:");
                for (int i = 0; i < MAX_TEXTURE_COUNT; i++) {
                    instance.textures[i]->Bind(i);
                    // ENG_LOG_INFO("[{}] - '{}' ({})", i, instance.textures[i]->Name(), instance.textures[i]->Handle());
                }
                // ENG_LOG_INFO("----");

                //draw the quads
                glDrawElements(GL_TRIANGLE_STRIP, instance.idx * 5, GL_UNSIGNED_INT, nullptr);
                instance.stats.drawCalls++;
                instance.stats.totalQuads += instance.idx;
                instance.stats.wastedQuads += instance.lastFlush_wastedQuads;
                instance.lastFlush_wastedQuads = BATCH_SIZE - instance.idx;
            }

            instance.idx = 0;
            instance.texIdx = 1;
        }

        //======================

        void RenderQuad(const Quad& quad) {
            //resolve needs to be done before adding to buffers, since it can trigger Flush()
            uint32_t texIdx = ResolveTextureIdx(quad.tex);

            instance.quadBuffer[instance.idx] = quad.vertices;
            instance.indexBuffer[instance.idx] = QuadIndices(instance.idx);

            instance.quadBuffer[instance.idx].UpdateTextureIdx(texIdx);

            instance.idx++;
            if(instance.idx >= BATCH_SIZE) {
                Flush();
            }
        }

        //============== RendererInstance ==============

        RendererInstance::~RendererInstance() {
            if(IsInitialized()) {
                delete[] quadBuffer;
                delete[] indexBuffer;

                glDeleteBuffers(1, &vbo);
                glDeleteBuffers(1, &ebo);
                glDeleteBuffers(1, &vao);

                ENG_LOG_TRACE("[D] RendererInstance");
            }
        }

        void RendererInstance::Initialize() {
            ASSERT_MSG(quadBuffer == nullptr, "Calling RendererInstance::Initialize() on already initialized object.");

            //quad vertices & indices buffers
            quadBuffer = new QuadVertices[BATCH_SIZE];
            indexBuffer = new QuadIndices[BATCH_SIZE];

            //their GPU counterparts
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            //proper vertex attributes setup
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
            glEnableVertexAttribArray(2);
            glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, textureID));
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, alphaFromTexture));
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(5, 4, GL_UNSIGNED_INT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, info));
            glEnableVertexAttribArray(5);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Quad) * BATCH_SIZE, nullptr, GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(QuadIndices) * BATCH_SIZE, nullptr, GL_DYNAMIC_DRAW);

            //empty texture
            uint8_t tmp[] = { 255,255,255,255 };
            blankTexture = std::make_shared<Texture>(TextureParams::CustomData(1, 1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE), (void*)&tmp, "renderer_blankTexture");

            ENG_LOG_TRACE("[C] RendererInstance");
        }

        bool RendererInstance::IsInitialized() const {
            return (quadBuffer != nullptr);
        }

        //===============================

        uint32_t ResolveTextureIdx(const TextureRef& texture) {
            uint32_t idx = 0;

            if(texture != nullptr) {
                //search for the texture in already queued textures
                //search only the textures queued in this draw call (higher index textures might get overriden)
                for (int i = 0; i < instance.texIdx; i++) {
                    if (*(instance.textures[i]) == *texture) {
                        idx = uint32_t(i);
                        break;
                    }
                }

                //not found, use new slot for this texture
                if (idx == 0) {
                    //trigger a draw call, if all the slots are already taken
                    if (instance.texIdx > instance.texIdxEnd) {
                        Flush();
                    }

                    idx = (uint32_t)instance.texIdx;
                    instance.textures[instance.texIdx] = texture;
                    instance.texIdx++;
                }
            }

            return idx;
        }

        void WireframeMode(bool enabled) {
            glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
        }

        int ForceBindTexture(const TextureRef& texture) {
            int idx = instance.texIdxEnd;
            instance.texIdxEnd--;

            if(idx >= 0) {
                if(instance.texIdx >= instance.texIdxEnd) {
                    Flush();
                }
                instance.textures[idx] = texture;
            }

            return idx;
        }
        
    }//namespace Renderer

}//namespace eng
