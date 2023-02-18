# Strategy 2d

My attempts as a 2D strategy game, inspired by Warcraft II.

**3rd party libraries:**
* <a href="https://github.com/glfw/glfw">GLFW</a> (3.4)
* <a href="https://github.com/Dav1dde/glad">Glad</a>
* <a href="https://github.com/g-truc/glm">GLM</a> (0.9.9.8)
* <a href="https://github.com/nothings/stb">stb_image</a> (v2.27)
* <a href="https://github.com/ocornut/imgui">Imgui</a> (v1.89)
* <a href="https://github.com/nlohmann/json">nlohmann/json</a> (v3.11.2)
* <a href="https://gitlab.freedesktop.org/freetype/freetype">FreeType</a> (2.12.1)

Libraries are included in the repo itself or are added as submodules, use `git clone --recursive` when cloning.

The game expects OpenGL version 4.5 or higher (might work with lower, but you'd have to change version values in the source).

Code was tested on:
* Win10 21H1 (MSBuild 17.3.0)
* Ubuntu 22.04 (gcc 11.2.0)
