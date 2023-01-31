#include <fstream>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

#include <sodium.h>

#include "SDL.h"
#include "SDL_image.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include "utils/error_handling.h"
#include "utils/common.h"
#include "music-player.h"
#include "ball.hpp"
#include "user.h"
#include "draw_users_table.hpp"
#include "map.hpp"
#include "scheduler.hpp"

using namespace std;

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error ImGUI requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#define BALL_TEXT_COUNT 5
#define SPEED_FACTOR 1.0

static bool should_record_map = false;

void parse_arguments(const int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "t")) != -1) {
        switch (opt) {
        case 't':
            should_record_map = true;
            break;
        default:
            cerr << "The only valid flag is -t for creating a new map" << endl;
            exit(EXIT_FAILURE);
        }
    }
}

void render_balls(vector<Ball*> &balls, const double step, const double map_location) {
    if(balls.empty()) {
        return;
    }
    balls[0]->move_to_location(map_location);
    for(int i = 1; i < int(balls.size()); i++) {
        if(balls[i]->should_render){
            double new_pos = balls[i]->loc_index + step;
            balls[i]->move_to_location(new_pos);
        } else {
            const double pos = balls[i - 1]->get_previous_ball_pos();
            balls[i]->move_to_location(pos);
        }
    }
    for(int i = 0; i < int(balls.size()); i++) {
        balls[i]->draw_ball();
    }
}

int main(int argc, char **argv)
{
    parse_arguments(argc, argv);

    char buf[100];
    int selected_map = 1;
    double map_location = 0;  
    double map_location_step = 0;

    s64 prev_time = 0;
    s64 curr_time = 0;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    SDL_Rect gun_rect = {.x=600, .y=320, .w=80, .h=160};

    SDL_Texture *gun_texture;

    vector<SDL_Texture*> ball_textures;
    vector<Map*> maps;
    vector<Ball*> balls;

    MusicPlayer *music_player = nullptr;
    NewMap *new_map = nullptr;
    Map *curr_map = nullptr;
    UserMap users = get_user_list();

    // TODO: Delete me!
    pdd last_mouse_pos = {SCREEN_W >> 1, SCREEN_H >> 1}; // Do not change!
    SDL_Color the_line_color = {30, 0, 255, 255};

    const pdd center = last_mouse_pos;

    // --- Starting Initialization!
    if (sodium_init() < 0) {
        cerr << "Could not initialize libsodium, quiting" << endl;
        return 1;
    }

    // Init video, timer and audio subsystems. Other subsystems like event are
    // initialized automatically by these options.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0){
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Couldn't initialize SDL: %s",
            SDL_GetError()
        );
        return 3;
    }

    music_player = new MusicPlayer();
    if (!INIT_SUCCESS(*music_player)){
        return 1;
    }


    // Force OpenGL acceleration and allow High-DPI for ImGUI
    window = SDL_CreateWindow("BP-cpp",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_W, SCREEN_H,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

    // Use VSync to limit framerate
    renderer = SDL_CreateRenderer(window,
                                  -1,
                                  SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    if (!window || !renderer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    // setup and initialize sdl2_image library
    {
        int flag_IMG = IMG_INIT_PNG | IMG_INIT_JPG;
        int IMG_init_status = IMG_Init(flag_IMG);
        if ((IMG_init_status & flag_IMG) != flag_IMG){
            cerr << "Required formats for SDL2Image where not initialized" << endl;
            return 3;
        }
    }
    // --- Initialization Done!

    if(should_record_map) {
        cerr << "I want to record a new map!" << endl;
        new_map = new NewMap(renderer);
    }

    //TODO: remove this comment, let's play some music!
    //music_player->play_music(0); // Start from the first music

    maps = get_all_maps(renderer);

    //TODO: Do this properly!
    curr_map = maps.at(selected_map);

    for(int i = 1; i <= BALL_TEXT_COUNT; i++) {
        snprintf(buf, 90, "../art/images/ball%02d.png", i);
        ball_textures.push_back(IMG_LoadTexture(renderer, buf));
        assert(ball_textures.back() != nullptr);
    }

    gun_texture = IMG_LoadTexture(renderer, "../art/images/gun01.png");
    assert(gun_texture != nullptr);
    // TODO: I move me to a dedicated function
    {
        UID(color_gen, 0, ball_textures.size() - 1);
        for(int i = 0; i < 20; i++) {
            const int ball_color = color_gen(rng);
            balls.push_back(new Ball(ball_color, ball_textures.at(ball_color), renderer, &(curr_map->locations)));
            if(i > 0) {
                balls[i]->should_render = false;
            }
        }
    }
    // Now let's setup ImGUI:
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsLight();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer_Init(renderer);

    io.Fonts->AddFontFromFileTTF("../art/fonts/roboto-medium.ttf", 20.0f);

    bool should_quit = false;

    while (!should_quit)
    {
        while (!should_quit && SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type)
            {
            case SDL_QUIT:
                should_quit = true;
                break;

            case SDL_MOUSEMOTION:
                last_mouse_pos = {event.motion.x, event.motion.y};
                break;

            case SDL_MOUSEBUTTONDOWN:
                if(should_record_map && !io.WantCaptureMouse){
                    new_map->add_point(last_mouse_pos);
                }
                the_line_color = {255, 0, 255, 255};
                break;

            case SDL_MOUSEBUTTONUP:
                the_line_color = {30, 0, 255, 255};
                break;

            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_SPACE && should_record_map && !io.WantCaptureKeyboard){
                    the_line_color = {255, 0, 255, 255};
                    cerr << "I stopped recording coordinates!" << endl;
                    should_quit = true;
                }
                break;

            case SDL_KEYUP:
                the_line_color = {30, 0, 255, 255};
                break;

            default:
                break;
            }    
        }
        curr_time = (s64) SDL_GetTicks64();
        {
            double dt = curr_time - prev_time;
            dt /= 1000; // dt should be in seconds
            map_location_step = SPEED_FACTOR * dt * 100;
            map_location += map_location_step;
        }
        Scheduler::run(curr_time);

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if(!should_record_map) {
            static int counter = 0;

            ImGui::Begin("Settings");
            ImGui::Text("This is some useful text.");
            if (ImGui::Button("Button"))
            {
                counter++;
            }
            ImGui::SameLine();
            ImGui::Text("Counter = %d", counter);
            ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
            draw_users_table(users);
            ImGui::End();
        }

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff); // sets the background color
        SDL_RenderClear(renderer);

        curr_map->draw_background();

        SDL_SetRenderDrawColor(renderer, the_line_color.r, the_line_color.g, the_line_color.b, 0xff);
        SDL_RenderDrawLine(renderer,
                           SCREEN_W >> 1, SCREEN_H >> 1,
                           last_mouse_pos.first, last_mouse_pos.second);

        pdd diff = last_mouse_pos - center;
        const double phase = vector_phase(diff) + (PI / 2.0);
        SDL_RenderCopyEx(renderer, gun_texture, NULL, &gun_rect, phase * RAD_TO_DEG, NULL, SDL_FLIP_NONE);//rotate and show wepon

        render_balls(balls, map_location_step, map_location);

        if(should_record_map) {
            new_map->draw_status(last_mouse_pos);
        }

        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());

        prev_time = curr_time;

        SDL_RenderPresent(renderer);
    }

    if(new_map != nullptr) {
        delete new_map;
    }

    for(auto &x:maps) {
        delete x;
    }

    SDL_DestroyTexture(gun_texture);

    for(auto &txt:ball_textures) {
        SDL_DestroyTexture(txt);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    save_user_list(users);

    return 0;
}
