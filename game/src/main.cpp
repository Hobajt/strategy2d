#include <stdio.h>

#include <engine/engine.h>

int main(int argc, char** argv) {
    eng::Log::Initialize();

    eng::Window& window = eng::Window::Get();
    window.Initialize(640, 480, "test");

    while(!window.ShouldClose()) {
        window.SwapAndPoll();
    }
    
    return 0;
}
