#include "gui.h"

GUI::GUI()
{
}

GUI::~GUI()
{
    if (initialized) {
        shutdown();
    }
}

void GUI::init(SDL_Window* window, SDL_GLContext context)
{
    if (initialized) return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 430");

    initialized = true;
}

void GUI::shutdown()
{
    if (!initialized) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    initialized = false;
}

void GUI::beginFrame()
{
    if (!initialized) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void GUI::endFrame()
{
    if (!initialized) return;

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GUI::processEvent(const SDL_Event& event)
{
    if (!initialized) return;

    ImGui_ImplSDL3_ProcessEvent(&event);
}
