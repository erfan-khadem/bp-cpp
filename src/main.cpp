#include "SDL.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include "utils/error_handling.h"
#include "utils/common.h"
#include "music-player.h"

using namespace std;

#if !SDL_VERSION_ATLEAST(2,0,17)
#error ImGUI requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

int main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {
	SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface __attribute__((unused));
    SDL_Event event;

    MusicPlayer *music_player = nullptr;

    pii last_mouse_pos = {SCREEN_W >> 1, SCREEN_H >> 1};

    // Init video, timer and audio subsystems. Other subsystems like event are
    // initialized automatically by these options.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    music_player = new MusicPlayer();
    if(!INIT_SUCCESS(*music_player)) {
        return 1;
    }

    // Use OpenGL acceleration and allow High-DPI for ImGUI
    window = SDL_CreateWindow("BP-cpp",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    
    // Use VSync to limit framerate
    renderer = SDL_CreateRenderer(window,
        -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    if (!window || !renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }
    music_player->play_music(0); // Start from the first music

    // Now let's setup ImGUI:
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsLight();
    
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer_Init(renderer);

    io.Fonts->AddFontFromFileTTF("../art/fonts/roboto-medium.ttf", 20.0f);

    bool should_quit = false;

    while (!should_quit) {
        while(!should_quit && SDL_PollEvent(&event)){
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type)
            {
            case SDL_QUIT:
                should_quit = true;
                break;

            case SDL_MOUSEMOTION:
                last_mouse_pos = {event.motion.x, event.motion.y};
                break;
            
            default:
                break;
            }
        }
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        {
            static int counter = 0;

            ImGui::Begin("Settings");
            ImGui::Text("This is some useful text.");
            if (ImGui::Button("Button")) {
                counter++;
            }
            ImGui::SameLine();
            ImGui::Text("Counter = %d", counter);
            ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
            ImGui::End();
        }

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff); // sets the background color
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xff, 0xff);
        SDL_RenderDrawLine(renderer,
            SCREEN_W >> 1, SCREEN_H >> 1,
            last_mouse_pos.first, last_mouse_pos.second);
        
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
