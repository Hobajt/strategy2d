# Strategy 2d

My attempts at a 2D strategy game, "inspired" by Warcraft II.

## Gameplay Demo
<img src="sample_compressed.gif">

Less compressed version is <a href="sample.gif">here</a>.

**3rd party libraries:**
* <a href="https://github.com/glfw/glfw">GLFW</a> (3.4)
* <a href="https://github.com/Dav1dde/glad">Glad</a>
* <a href="https://github.com/g-truc/glm">GLM</a> (0.9.9.8)
* <a href="https://github.com/nothings/stb">stb_image</a> (v2.27)
* <a href="https://github.com/ocornut/imgui">Imgui</a> (v1.89)
* <a href="https://github.com/nlohmann/json">nlohmann/json</a> (v3.11.2)
* <a href="https://gitlab.freedesktop.org/freetype/freetype">FreeType</a> (2.13.0)
* <a href="https://github.com/gabime/spdlog">spdlog</a> (v1.11.0)
* <a href="https://github.com/mackron/miniaudio">miniaudio</a> (0.11.11)
* <a href="https://github.com/TeamHypersomnia/rectpack2D">rectpack2D</a>

Libraries are included in the repo itself or are added as submodules, use `git clone --recursive` when cloning.

The game expects OpenGL version 4.5 or higher (might work with lower, but you'd have to change version values in the source).

Expects to be launched from the project root (in order to properly locate the resources).

Game starts in 640x480 resolution, but can be made fullscreen when launched with ```--fullscreen``` argument.

## Notes
- This project was intended as a learning/hobby project of mine and, as such, is not a 1:1 copy of the original game
- I did however try to at least keep it holistically similar to the original game
- Things that are different:
    - Limited number of campaign & custom maps (only 2 of each)
    - Timings are off or are sped up (building construction, attack speeds, etc.)
    - Pathfinding works differently
    - There's no AI (apart from unit responses - no faction controlling AI)
    - Sprite sizes are off for some units (footman smaller than peasant for example)
    - Various bugs that I didn't find, but that are no doubt present in my version
- Apart from the game itself, there's also an **editor**, which can be used to generate game maps
    - Editor requires to be built with ImGui support (the game does not)

- Although the game does support **sound playback**, there are no sounds included in the repo, because I wasn't sure if it was legal to include the original Warcraft II sounds (and didn't want to use any different ones)
    - The sounds can however be extracted if you own Warcraft II copy using <a href="http://www.zezula.net/en/mpq/main.html">MPQ Editor</a> (more on how to make this work <a href='MPQ_EXPORT.md'>here</a>)
    - Same reason why all the backgrounds are redrawn (poorly)

## Building

- Requirements:
    - CMake 3.16 or later
    - OpenGL 4.5 or later
    - C++17
- Howto:
    - Generate the project:
        - ```cmake -B ./build -S ./```
        - can add additional CMake defines or use CMake GUI
    - Then build:
        - ```cmake --build ./build --config Release```

## CMake Defines

<table>
    <tr>
        <th>Name</th>
        <th>Description</th>
    </tr>
    <tr>
        <td>BUILD_EDITOR</td>
        <td>Build editor exectutable (requires ENGINE_ENABLE_GUI)</td>
    </tr>
    <tr>
        <td>ENGINE_ENABLE_LOGGING</td>
        <td>Enables debug logging into the console</td>
    </tr>
    <tr>
        <td>ENGINE_ENABLE_ASSERTS</td>
        <td>Enables assertions</td>
    </tr>
    <tr>
        <td>ENGINE_ENABLE_GUI</td>
        <td>Enables ImGui debugging overlay</td>
    </tr>
    <tr>
        <td>BUILD_GLFW_FROM_SOURCE</td>
        <td>Whether to use locally installed GLFW or build from source as part of the project</td>
    </tr>
</table>

## Debug notes
- Hotkeys:
    - T - toggle fullscreen
    - O - freeze camera panning by mouse (has effect in game only)
    - P - toggle debug GUI (if built, otherwise has no effect)
    - Q - terminate the game

## Code was tested on:
* Win10 21H1 (MSBuild 17.3.0)
* Ubuntu 22.04 (gcc 11.2.0)
