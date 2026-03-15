#ifndef GUI_H
#define GUI_H

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <SDL3/SDL.h>
#include <string>

class GUI {
public:
    GUI();
    ~GUI();

    void init(SDL_Window* window, SDL_GLContext context);
    void shutdown();

    void buildPanel(std::string panelJson);
    
    void beginFrame();
    void endFrame();
    void processEvent(const SDL_Event& event);

    ImGuiIO& getIO() { return ImGui::GetIO(); }
    
private:
    bool initialized = false;
};

#endif
