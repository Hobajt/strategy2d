#include "game.h"

#include "menu.h"

using namespace eng;

//TODO: add some precursor to main menu
//TODO: add OnDrag() to GUI detection - for map implementation (or some similar name, drag, down, hold, whatever)
//TODO: make logging initialization through singleton -> static objects cannot really use logging now since they get initialized before the logging

//TODO: remove SelectionHandler completely - use GUI hierarchy for GUI selection and cells for map selection
//TODO: think through how to handle resources - for example during menu initializations ... how to pass various things to correct places
//TODO: do input key handling through callbacks

//TODO: add some config file that will act as a persistent storage for options and stuff

//less immediate future:
//TODO: add scroll menu to gui
//TODO: add basic play button with proper stage change & setup
//TODO: add cmdline args to launch in debug mode, skip menus, etc.
//TODO: stage switching, stage transitions (once there's ingame stage controller)
//TODO: add intro & loading screen (either using stages or in some other way)

//immediate future:
//TODO: figure out how to generate (nice) button textures - with borders and marble like (or whatever it is they have)
//TODO: move the button texture generation somewhere else

/*scroll menu impl:
    - there will be fixed number of buttons in it
    - buttons themselves won't move, scrolling will just move content among the generated buttons
        - buttons not moving -> can use regular mouse selection method
    - there will be 3 special buttons -> scroll slider (up,down & slider button)
        - can maybe wrap this in one single class
*/

constexpr float fontScale = 0.055f;

glm::u8vec3* GenerateButtonTexture(int width, int height, bool flipShading);

Game::Game() : App(640, 480, "game") {}

void Game::OnResize(int width, int height) {
    LOG_INFO("Resize triggered ({}x{})", width, height);
    font->Resize(fontScale * height);
}

void Game::OnInit() {
    try {
        font = std::make_shared<Font>("res/fonts/PermanentMarker-Regular.ttf", int(fontScale * Window::Get().Height()));

        texture = std::make_shared<Texture>("res/textures/test2.png");
        backgroundTexture = std::make_shared<Texture>("res/textures/TitleMenu_BNE.png");

        ReloadShaders();
    } catch(std::exception& e) {
        LOG_INFO("ERROR: {}", e.what());
        LOG_ERROR("Failed to load resources; Terminating...");
        throw e;
    }

    // Window::Get().SetFullscreen(true);

    
    //size = window_size * button_size (in menu) - match the size ratio to have nice and even borders
    int width = 212, height = 48;

    //regular button texture
    glm::u8vec3* data = GenerateButtonTexture(width, height, false);
    btnTexture = std::make_shared<Texture>(TextureParams::CustomData(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE), (void*)data);
    delete[] data;

    //pushed-in button texture
    data = GenerateButtonTexture(width, height, true);
    btnTextureClick = std::make_shared<Texture>(TextureParams::CustomData(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE), (void*)data);
    delete[] data;

    stageController.Initialize({ 
        std::make_shared<MainMenuController>(font, btnTexture, btnTextureClick, backgroundTexture)
    });
}

static InputButton t = InputButton(GLFW_KEY_T);

void Game::OnUpdate() {
    Window& window = Window::Get();
    Input& input = Input::Get();
    Camera& camera = Camera::Get();

    input.Update();
    camera.Update();

    Renderer::StatsReset();

    if(window.GetKeyState(GLFW_KEY_Q))
        window.Close();

    t.Update();
    if(t.down()) {
        static bool fullscreen = false;
        fullscreen = !fullscreen;
        LOG_INFO("SETTING FULLSCREEN = {}", fullscreen);
        window.SetFullscreen(fullscreen);
    }

    stageController.Update();

    Renderer::Begin(shader, true);
    stageController.Render();
    Renderer::End();
}

void Game::OnGUI() {
#ifdef ENGINE_ENABLE_GUI
    Audio::DBG_GUI();
    Camera::Get().DBG_GUI();

    ImGui::Begin("General");
    ImGui::Text("FPS: %.1f", Input::Get().fps);
    ImGui::Text("Draw calls: %d | Total quads: %d (%d wasted)", Renderer::Stats().drawCalls, Renderer::Stats().totalQuads, Renderer::Stats().wastedQuads);
    if(ImGui::Button("Reload shaders")) {
        try {
            ReloadShaders();
        }
        catch(std::exception& e) {
            LOG_ERROR("ReloadShaders failed...");
            //throw e;
        }
        LOG_INFO("Shaders reloaded.");
    }
    ImGui::End();
#endif
}

void Game::ReloadShaders() {
    shader = std::make_shared<Shader>("res/shaders/test_shader");
    shader->InitTextureSlots(Renderer::TextureSlotsCount());
}

glm::u8vec3* GenerateButtonTexture(int width, int height, bool flipShading) {
    glm::u8vec3 fillColor = glm::u8vec3(150,0,0);
    glm::u8vec3 borderColor = glm::u8vec3(30, 0, 0);
    glm::u8vec3 lightColorBase = glm::u8vec3(200,0,0);
    glm::u8vec3 shadowColorBase = glm::u8vec3(100,0,0);
    glm::u8vec3 darkShadowColor = glm::u8vec3(80, 0, 0);

    glm::u8vec3 lightColor = lightColorBase;
    glm::u8vec3 shadowColor = shadowColorBase;

    if(flipShading) {
        fillColor = glm::u8vec3(110, 0, 0);
        lightColor = darkShadowColor;
        shadowColor = fillColor;
    }

    int borderWidth = 3;

    int bw_y = borderWidth;
    int bw_x = int(borderWidth / Window::Get().Ratio());

    glm::u8vec3* data = new glm::u8vec3[width * height];

    //fill
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            data[y * width + x] = fillColor;
        }
    }

    //borders + 3d outlines - top & bottom
    for(int y = 0; y < bw_y; y++) {
        for(int x = 0; x < width; x++) {
            data[y * width + x] = borderColor;
            data[(height-1-y) * width + x] = borderColor;

            data[(y+bw_y) * width + x] = lightColor;
            data[(height-1-y-bw_y) * width + x] = shadowColor;
        }
    }
    //borders + 3d outlines - left & right
    for(int y = bw_y; y < height-bw_y; y++) {
        for(int x = 0; x < bw_x; x++) {
            data[y * width + x] = borderColor;
            data[y * width + width-1-x] = borderColor;

            data[y * width + x+bw_x] = lightColor;
            data[y * width + width-1-x-bw_x] = shadowColor;
        }
    }
    //3d outlines - corners (botLeft & topRight)
    for(int y = 0; y < bw_y; y++) {
        for(int x = 0; x < bw_x; x++) {
            if(atan2(y, x) > glm::pi<double>() * 0.25) {
                data[(y+bw_y) * width + width-1-x-bw_x] = shadowColor;
                data[(height-1-y-bw_y) * width + x+bw_x] = lightColor;
            }
            else {
                data[(y+bw_y) * width + width-1-x-bw_x] = lightColor;
                data[(height-1-y-bw_y) * width + x+bw_x] = shadowColor;
            }
        }
    }

    return data;
}