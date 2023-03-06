#include "intro.h"

#define GAME_LOGO_DISPLAY_TIME 2.f

using namespace eng;

static std::string stage_names[] = { "INVALID", "GAME_START", "COMPANY_LOGO", "CINEMATIC", "GAME_LOGO", "CINEMATIC_REPLAY" };

IntroController::IntroController(const eng::TextureRef& gameLogoTexture_) : state(IntroState::GAME_START), gameLogoTexture(gameLogoTexture_) {}

void IntroController::Update() {
    TransitionHandler* th = GetTransitionHandler();
    float currentTime = Input::Get().CurrentTime();

    float duration = interrupted ? TransitionDuration::SHORT : TransitionDuration::MID;

    //TODO: force override transition when interrupted (to allow skipping even while the fade-in is still in progress)

    switch(state) {
        case IntroState::COMPANY_LOGO:
            //TODO: only trigger after the video is almost at the end
            if(interrupted || (timeStart + 2.f < currentTime && !th->TransitionInProgress())) {
                th->InitTransition(TransitionParameters(duration, TransitionType::FADE_OUT, GameStageName::INTRO, IntroState::CINEMATIC, true));
            }
            break;
        case IntroState::CINEMATIC:
            //TODO: only trigger after the video is almost at the end
            if(interrupted || (timeStart + 2.f < currentTime && !th->TransitionInProgress())) {
                th->InitTransition(TransitionParameters(duration, TransitionType::FADE_OUT, GameStageName::INTRO, IntroState::GAME_LOGO, true));
            }
            break;
        case IntroState::GAME_LOGO:
            if(interrupted || (timeStart + GAME_LOGO_DISPLAY_TIME < currentTime && !th->TransitionInProgress())) {
                th->InitTransition(TransitionParameters(duration, TransitionType::FADE_OUT, GameStageName::MAIN_MENU, -1, true));
            }
            break;
        case IntroState::CINEMATIC_REPLAY:
            //same stuff as in cinematic, but skip over to the main menu instead
            if(interrupted || (timeStart + 2.f < currentTime && !th->TransitionInProgress())) {
                th->InitTransition(TransitionParameters(duration, TransitionType::FADE_OUT, GameStageName::MAIN_MENU, -1, true));
            }
            break;
    }

    interrupted = false;
}

void IntroController::Render() {
    FontRef font = Font::Default();

    switch(state) {
        default:
        case IntroState::INVALID:
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f, 1.f), glm::vec4(1.f, 0.f, 1.f, 1.f), nullptr));
            break;
        case IntroState::COMPANY_LOGO:
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f, 1.f), glm::vec4(1.f, 0.f, 0.f, 1.f), nullptr));
            font->RenderTextCentered("Intro 1", glm::vec2(0.f), 10.f, glm::vec4(0.f, 0.f, 0.f, 1.f));
            break;
        case IntroState::CINEMATIC:
        case IntroState::CINEMATIC_REPLAY:
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f, 1.f), glm::vec4(0.f, 0.f, 1.f, 1.f), nullptr));
            font->RenderTextCentered("Cinematic", glm::vec2(0.f), 10.f, glm::vec4(0.f, 0.f, 0.f, 1.f));
            break;
        case IntroState::GAME_LOGO:
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f, 1.f), glm::vec4(1.f), gameLogoTexture));
            break;
    }
}

void IntroController::OnPreStart(int prevStageID, int data) {
    TransitionHandler* th = GetTransitionHandler();
    
    switch(prevStageID) {
        case GameStageName::INVALID:
            //intro stage was triggered from game starting state
            state = IntroState::COMPANY_LOGO;
            GetTransitionHandler()->InitTransition(TransitionParameters(TransitionDuration::MID, TransitionType::FADE_IN, GameStageName::INTRO, IntroState::CINEMATIC, false));
            //TODO: start playing the video here
            break;
        case GameStageName::INTRO:
            //intro stage triggered by itself - run next substage
            state = data;
            break;
        case GameStageName::MAIN_MENU:
            //intro stage triggered from main menu - replay cinematic issued
            state = IntroState::CINEMATIC_REPLAY;
            break;
    }

    Input::Get().AddKeyCallback(-1, [](int keycode, int modifiers, void* data){
        static_cast<IntroController*>(data)->KeyPressCallback(keycode, modifiers);
    }, true, this);

    LOG_INFO("GameStage = Intro (substage: {})", stage_names[state]);
}

void IntroController::OnStart(int prevStageID, int data) {
    timeStart = Input::Get().CurrentTime();
}

void IntroController::OnStop() {}

void IntroController::KeyPressCallback(int keycode, int modifiers) {
    interrupted = true;
}
