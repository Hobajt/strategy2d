#include "intro.h"

#define GAME_LOGO_DISPLAY_TIME 2.f

using namespace eng;

static std::string stage_names[] = { "INVALID", "GAME_START", "COMPANY_LOGO", "CINEMATIC", "GAME_LOGO", "CINEMATIC_REPLAY", "COUNT" };

IntroController::IntroController(const eng::TextureRef& gameLogoTexture_) : state(IntroState::GAME_START), gameLogoTexture(gameLogoTexture_) {}

void IntroController::Update() {
    TransitionHandler* th = GetTransitionHandler();
    float currentTime = Input::Get().CurrentTime();

    //to also skip intros with mouse button clicks
    Input& input = Input::Get();
    interrupted |= (input.lmb.down() || input.rmb.down());

    static TransitionParameters transitions[IntroState::COUNT] = {
        TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::INTRO, IntroState::CINEMATIC, true),     //company logo
        TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::INTRO, IntroState::GAME_LOGO, true),     //cinematic
        TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::MAIN_MENU, -1, true),                    //game logo
        TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::MAIN_MENU, -1, true),                    //cinematic replay
    };

    bool condition = false;
    switch(state) {
        case IntroState::COMPANY_LOGO:
        case IntroState::CINEMATIC_REPLAY:
        case IntroState::CINEMATIC:
            //TODO: trigger instead when the video is almost at the end
            condition = timeStart + 2.f < currentTime;
            break;
        case IntroState::GAME_LOGO:
            condition = timeStart + GAME_LOGO_DISPLAY_TIME < currentTime;
            break;
    }

    if(interrupted && !th->CompareTransitions(transitions[state], TransitionDuration::SHORT)) {
        //key/mouse interrupt -> override transition (unless override already happened)
        th->InitTransition(transitions[state].WithDuration(TransitionDuration::SHORT), true);
    }
    else if(condition && !th->TransitionInProgress()) {
        //state switching condition met -> trigger transition
        th->InitTransition(transitions[state]);
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
