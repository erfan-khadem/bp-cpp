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

using namespace std;

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error ImGUI requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#define BALL_TEXT_COUNT 5

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

void render_balls(vector<Ball> &balls, const double counter_for_loc) {
    if(balls.empty()) {
        return;
    }
    balls[0].move_to_location(counter_for_loc);
    for(int i = 1; i < int(balls.size()); i++) {
        const int pos = balls[i - 1].get_previous_ball_pos();
        balls[i].move_to_location(pos);
    }
    for(int i = 0; i < int(balls.size()); i++) {
        balls[i].draw_ball();
    }
}

int main(int argc, char **argv)
{
    parse_arguments(argc, argv);

    char buf[100];
    double counter_for_loc = 0;  

    s64 prev_time = 0;
    s64 curr_time = 0;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    SDL_Rect gun_rect = {.x=600, .y=320, .w=80, .h=160};

    SDL_Texture *gun_texture;
    SDL_Texture *map_texture;
    SDL_Texture *background_texture;

    vector<SDL_Texture*> ball_textures;
    vector<Ball> balls;
    vector<pii> locations;
    vector<pii> recorded_locations;

    /*
    reading existing road -> (the first line of the "locations_file.txt"
    shows the existence of road , second line is number of coordinates);
    if you made new road , add "1" and number of coordinates.
    */
    ifstream map_file;
    ofstream new_map_location;

    map_file.open("../art/maps/map01.txt");
    bool continue_creating_new_map = false;
    {
        int number_of_coordinates = 0;
        int is_map_available;

        map_file >> is_map_available;
        if(is_map_available == 1){
            map_file >> number_of_coordinates;
            for (int i = 0; i < number_of_coordinates;i++)
            {
                int x, y;
                map_file >> x >> y;
                locations.emplace_back(x,y);
            }
        }
    }
    map_file.close();

    MusicPlayer *music_player = nullptr;
    UserMap users = get_user_list();

    // TODO: Delete me!
    pdd last_mouse_pos = {SCREEN_W >> 1, SCREEN_H >> 1};// Do not change!
    SDL_Color the_line_color = {30, 0, 255, 255};

    const pdd center = last_mouse_pos;


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

    // Use OpenGL acceleration and allow High-DPI for ImGUI
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

    //music_player->play_music(0); // Start from the first music
    {
        SDL_Surface *background_01 = IMG_Load("../art/images/ground_02.jpg");
        background_texture = SDL_CreateTextureFromSurface(renderer, background_01);
        if(!background_texture) {
            cerr << "Failed to create background texture" << endl;
            return 2;
        }
    }
    for(int i = 1; i <= BALL_TEXT_COUNT; i++) {
        snprintf(buf, 90, "../art/images/ball%02d.png", i);
        auto surf = IMG_Load(buf);
        ball_textures.push_back(SDL_CreateTextureFromSurface(renderer, surf));
        assert(ball_textures.back() != nullptr);
    }
    {
        auto surf = IMG_Load("../art/maps/map01.png");
        map_texture = SDL_CreateTextureFromSurface(renderer, surf);
    }
    {
        SDL_Surface *gun_image;
        gun_image = IMG_Load("../art/images/gun01.png");
        gun_texture = SDL_CreateTextureFromSurface(renderer, gun_image);
    }
    // TODO: I move me to a dedicated function
    {
        UID(color_gen, 0, ball_textures.size() - 1);
        for(int i = 0; i < 20; i++) {
            const int ball_color = color_gen(rng);
            balls.emplace_back(ball_color, ball_textures[ball_color], renderer, &locations);
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
    
    //uncomment if want new road; 
    if(should_record_map){
        new_map_location.open("../art/maps/generator/new_locations_file.txt", ios::out);
        continue_creating_new_map = true;
    }

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
                if(should_record_map && continue_creating_new_map && !io.WantCaptureMouse){
                    new_map_location << event.motion.x << " " << event.motion.y << endl;//making new map if you want new map;
                    recorded_locations.emplace_back(event.motion.x, event.motion.y);
                }
                the_line_color = {255, 0, 255, 255};
                break;

            case SDL_MOUSEBUTTONUP:
                the_line_color = {30, 0, 255, 255};
                break;

            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_SPACE && !io.WantCaptureKeyboard){
                    the_line_color = {255, 0, 255, 255};
                    continue_creating_new_map = false;
                    cerr << "I stopped recording coordinates!" << endl;
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
            dt /= 1000; // dt is now in seconds
            counter_for_loc += 2.0 * dt * 100; // move 2 locations forward every 100ms
        }

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
        } else {
            ImGui::Begin("New Map");
            ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
            ImGui::Text("Current mouse position: (%04d, %04d)",
                (int)last_mouse_pos.first, (int)last_mouse_pos.second);

            ImGui::Text("Previous locations:");
            ImGui::Indent(10.0);
            for(const auto &[x, y]:recorded_locations) {
                ImGui::Text("(%04d, %04d)", x, y);
            }
            ImGui::Unindent(10.0);
            ImGui::End();
        }

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff); // sets the background color
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, background_texture, NULL, NULL);
        SDL_RenderCopy(renderer, map_texture, NULL, NULL);
        SDL_SetRenderDrawColor(renderer, the_line_color.r, the_line_color.g, the_line_color.b, 0xff);
        SDL_RenderDrawLine(renderer,
                           SCREEN_W >> 1, SCREEN_H >> 1,
                           last_mouse_pos.first, last_mouse_pos.second);

        pdd diff = last_mouse_pos - center;
        const double phase = vector_phase(diff) + (PI / 2.0);
        SDL_RenderCopyEx(renderer, gun_texture, NULL, &gun_rect, phase * RAD_TO_DEG, NULL, SDL_FLIP_NONE);//rotate and show wepon

        render_balls(balls, counter_for_loc);

        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());

        prev_time = curr_time;

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(background_texture);
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
