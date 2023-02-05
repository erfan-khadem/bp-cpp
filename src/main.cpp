#include <fstream>
#include <cmath>
#include <list>
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
//#include "user.h"
#include "map.hpp"
#include "scheduler.hpp"
#include "gui.hpp"
#include "slideshow.hpp"

using namespace std;

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error ImGUI requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#define BALL_TEXT_COUNT 5
#define NUM_BALLS 40
#define DEBUG true
//#define DEBUG false

static bool should_record_map = false;
static int user_score = 0;
MusicPlayer *music_player = nullptr;

char buf[100];

int selected_map = 0;
int curr_game_status = GameStatus::NOT_STARTED;
int idx_selected_power_up;

double map_location = 0;  
double map_location_step = 0;
double game_speed_factor = 1.0;

bool pause_cooldown = false;
bool play_music = false;
bool play_sfx = true;
bool schedule_game_status = false;
bool reverse_effect = false;
bool pause_effect = false;
bool slowdown_effect = false;

string maps_combo_choices;
string user_power_ups;
string selected_power_up;

s64 prev_time = 0;
s64 curr_time = 0;
s64 game_start_time = 0;

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Event event;
SDL_Rect holder_rect = {.x=(SCREEN_W - HOLDER_DIM)/2, .y=(SCREEN_H - HOLDER_DIM)/2,
                        .w=HOLDER_DIM, .h=HOLDER_DIM};

SDL_Texture *holder_texture;
SDL_Texture *frost_texture;

vector<SDL_Texture*> ball_textures;
vector<Map*> maps;

list<Ball*> balls;
list<ShootingBall*> shot_balls;

NewMap *new_map = nullptr;
Map *curr_map = nullptr;
UserMap users = get_user_list();
User* curr_user = nullptr;
ShootingBall* center_ball = nullptr;

pdd last_mouse_pos = {SCREEN_W >> 1, SCREEN_H >> 1}; // Do not change!
const pdd center = last_mouse_pos;

auto reset_game_status = [](){
    if(schedule_game_status == false) {
        Scheduler::schedule.emplace(
            curr_time + 5'000,
            [&](const s64 _) {
                (void)_;
                schedule_game_status = false;
                curr_game_status = GameStatus::NOT_STARTED;
            }
        );
        schedule_game_status = true;
    }
    game_speed_factor = 1.0;
    balls.clear();
    shot_balls.clear();
    if(center_ball != nullptr) {
        delete center_ball;
        center_ball = nullptr;
    }
    user_score = 0;
};
auto enable_reverse_effect = []() {
    if(reverse_effect) {
        return;
    }
    reverse_effect = true;
    pause_effect = false;
    slowdown_effect = false;
    music_player->play_sound("rewind.wav");
    Scheduler::schedule.emplace(
        curr_time + 5'000,
        [&](const s64 _) {
            (void)_;
            reverse_effect = false;
        }
    );
};
auto enable_pause_effect = []() {
    if(reverse_effect || pause_effect) {
        return;
    }
    pause_effect = true;
    slowdown_effect = false;
    music_player->play_sound("pause.wav");
    Scheduler::schedule.emplace(
        curr_time + 5'000,
        [&](const s64 _) {
            (void)_;
            pause_effect = false;
        }
    );
};
auto enable_slowdown_effect = []() {
    if(slowdown_effect || pause_effect) {
        return;
    }
    slowdown_effect = true;
    music_player->play_sound("freeze.wav");
    Scheduler::schedule.emplace(
        curr_time + 5'000,
        [&](const s64 _) {
            (void)_;
            slowdown_effect = false;
        }
    );
};
auto remake_user_power_ups = []() {
    user_power_ups = "";
    int curr_size = 0;
    user_power_ups += "None\0";
    for(const auto &[name, cnt]:curr_user->power_ups) {
        if(cnt <= 0) {
            continue;
        }
        curr_size = name.size();
        user_power_ups += name;
        assert(curr_size < 32);
        while(curr_size < 32) {
            user_power_ups.push_back(' ');
        }
        user_power_ups += to_string(cnt);
        user_power_ups += '\0';
    }
    user_power_ups += '\0';
};
auto remake_center_ball = [](){
    if(center_ball == nullptr) {
        UID(cball_color, 0, ball_textures.size() - 1);
        int col = cball_color(rng);
        center_ball = new ShootingBall(col, ball_textures[col], renderer, NULL);
    }
};

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

bool clear_balls(list<Ball*> &balls, list<Ball*>::iterator &it) {
    auto mn_ptr = it;
    auto mx_ptr = it;
    auto pptr = it;
    int cnt = 2;
    while(mn_ptr != balls.begin()) {
        pptr = mn_ptr;
        mn_ptr--;
        if(!(*mn_ptr)->same_color(*(*pptr))) {
            mn_ptr++;
            break;
        }
        cnt++;
    }

    while(mx_ptr != balls.end()) {
        pptr = mx_ptr;
        mx_ptr++;
        if(mx_ptr == balls.end() || !(*mx_ptr)->same_color(*(*pptr))){
            mx_ptr--;
            break;
        }
        cnt++;
    }
    if(cnt >= 3) {
        assert((*mn_ptr)->same_color(**mx_ptr));
        assert(it != mx_ptr || it != mn_ptr);
        mx_ptr++;
        it = mx_ptr;
        for(auto pt = mn_ptr; pt != mx_ptr;) {
            if((*pt)->freeze) {
                enable_slowdown_effect();
            } else if((*pt)->pause) {
                enable_pause_effect();
            } else if((*pt)->reverse) {
                enable_reverse_effect();
            }
            pt = balls.erase(pt);
        }
        assert(it == mx_ptr);
        user_score += 10 * cnt;
        return true;
    }
    return false;
}

int render_balls(list<Ball*> &balls, list<ShootingBall*> &shot_balls,
    int gs, const double dt, double &step, double &map_location) {
    if(balls.empty()) {
        return GameStatus::WON;
    }
    if(gs == GameStatus::PLAYING && !pause_effect){
        step = GLOBAL_SPEED_FACTOR * dt * 100 * (slowdown_effect ? 0.5 : 1.0);
        step = reverse_effect ? -step:step;
        map_location += step ;
    } else {
        step = 0;
    }
    if(gs == GameStatus::PLAYING) {
        auto ptr = shot_balls.begin();
        for(auto b:shot_balls) {
            b->step_dt(dt);
            b->draw_ball();
        }
        while(ptr != shot_balls.end()) {
            (*ptr)->step_dt(dt);
            if(!(*ptr)->in_screen()) {
                delete *ptr;
                ptr = shot_balls.erase(ptr);
            } else {
                (*ptr)->draw_ball();
                ptr++;
            }
        }
    }
    balls.front()->move_to_location(map_location);
    auto ptr = balls.begin();
    for(int i = 1; i < int(balls.size()); i++) {
        ptr++;
        if((*ptr)->should_render){
            double new_pos = (*ptr)->loc_index + step;
            (*ptr)->move_to_location(new_pos);
            if((*ptr)->should_render == false && (*ptr)->loc_index > 100) {
                return GameStatus::LOST;
            }
        } else {
            const double pos = (*(--ptr))->get_previous_ball_pos();
            (*(++ptr))->move_to_location(pos);
        }
    }
    ptr = balls.begin();
    auto sptr = shot_balls.begin();
    bool recalculate = false;
    for(; ptr != balls.end(); ptr++) {
        sptr = shot_balls.begin();
        while(sptr != shot_balls.end()) {
            if((*ptr)->collides(**sptr)) {
                recalculate = true;
                if((*ptr)->same_color(**sptr) && clear_balls(balls, ptr)) {
                    sptr = shot_balls.erase(sptr);
                    ptr--;
                } else if(++ptr != balls.end() && (*ptr)->same_color(**sptr) && clear_balls(balls, ptr)){
                    sptr = shot_balls.erase(sptr);
                    ptr--;
                    ptr--;
                } else {
                    auto bptr = ptr;
                    ptr--;
                    if((*sptr)->ball_color == MULTI_COLOR){
                        balls.insert(++ptr, &(*sptr)->to_normal_ball((*bptr)->locations,
                            (*bptr)->ball_color, (*bptr)->ball_txt));
                    } else {
                        balls.insert(++ptr, &(*sptr)->to_normal_ball((*bptr)->locations));
                    }
                    sptr = shot_balls.erase(sptr);
                    ptr--;
                }
            } else {
                sptr++;
            }
        }
    }
    if(balls.empty()) {
        return GameStatus::WON;
    }
    if(recalculate){
        music_player->play_sound("collision.wav");
        ptr = balls.begin();
        map_location = (*ptr)->loc_index;
        for(int i = 1; i < int(balls.size()); i++) {
            const double pos = (*ptr)->get_previous_ball_pos();
            (*(++ptr))->move_to_location(pos);
        }
    }
    for(auto b:balls) {
        if(b->loc_index > 100 && b->should_render == false) {
            return GameStatus::LOST;
        }
        b->draw_ball();
    }
    return GameStatus::PLAYING;
}

int main(int argc, char **argv)
{
    parse_arguments(argc, argv);

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

    if(play_music){
        music_player->play_music(0); // Start from the first music
    }

    maps = get_all_maps(renderer);
    for(auto m:maps) {
        maps_combo_choices += m->path_name;
        maps_combo_choices += '\0';
    }

    curr_map = maps.at(selected_map);

    for(int i = 1; i <= BALL_TEXT_COUNT; i++) {
        snprintf(buf, 90, "../art/images/ball%02d.png", i);
        ball_textures.push_back(IMG_LoadTexture(renderer, buf));
        assert(ball_textures.back() != nullptr);
        SDL_SetTextureBlendMode(ball_textures.back(), SDL_BLENDMODE_NONE);
    }

    holder_texture = IMG_LoadTexture(renderer, "../art/images/holder.png");
    frost_texture = IMG_LoadTexture(renderer, "../art/images/frost-effect.png");
    freeze_txt = IMG_LoadTexture(renderer, "../art/images/freeze.png");
    pause_txt = IMG_LoadTexture(renderer, "../art/images/pause.png");
    reverse_txt = IMG_LoadTexture(renderer, "../art/images/reverse.png");
    colordots_txt = IMG_LoadTexture(renderer, "../art/images/color-dots.png");
    greyball_txt = IMG_LoadTexture(renderer, "../art/images/ball06.png");

    assert(holder_texture != nullptr);
    SlideShow::slides = get_slideshow_images(renderer);
    // Now let's setup ImGUI:
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();

    style.WindowRounding = 12.0f;
    style.FrameRounding = 6.0f;
    style.ChildRounding = 12.0f;
    style.PopupRounding = 12.0f;
    style.GrabRounding = 1.0f;

    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 8.0f);

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
                if(!io.WantCaptureMouse){
                    if(should_record_map){
                        new_map->add_point(last_mouse_pos);
                    } else {
                        if(last_mouse_pos == center) {
                            last_mouse_pos.first += 1;
                        }
                        auto diff = last_mouse_pos - center;
                        auto sz_sq = dot(diff, diff);
                        auto dir = diff / sqrt(sz_sq);
                        remake_center_ball();
                        center_ball->direction = dir; 
                        center_ball->is_moving = dir.first != 0 || dir.second != 0;
                        if(center_ball->is_moving) {
                            music_player->play_sound("shoot.wav");
                        }
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP:
                break;

            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_SPACE && should_record_map){
                    cerr << "I stopped recording coordinates!" << endl;
                    should_quit = true;
                } else if(event.key.keysym.sym == SDLK_ESCAPE && !pause_cooldown && curr_game_status & GameStatus::IN_GAME) {
                    if(curr_game_status == GameStatus::PLAYING) {
                        curr_game_status = GameStatus::PAUSED;
                    } else if(curr_game_status == GameStatus::PAUSED) {
                        curr_game_status = GameStatus::PLAYING;
                    }
                    pause_cooldown = true;
                    Scheduler::schedule.emplace(curr_time + 100,
                    [&](const s64 _){
                        (void)_;
                        pause_cooldown = false;
                    });
                } else if(event.key.keysym.sym == SDLK_m) {
                    music_player->play_music(-1); // go to the next music
                } else if(event.key.keysym.sym == SDLK_n && curr_game_status & GameStatus::IN_GAME) {
                    delete center_ball;
                    center_ball = nullptr;
                    remake_center_ball();
                }
                break;

            case SDL_KEYUP:
                break;

            default:
                break;
            }    
        }
        curr_time = (s64) SDL_GetTicks64();
        double dt = curr_time - prev_time;
        dt /= 1000; // dt should be in seconds
        Scheduler::run(curr_time);

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if(!should_record_map){
            if(curr_user == nullptr) {
                if(DEBUG) {
                    curr_user = &users.begin()->second;
                } else {
                    curr_user = login_or_register(users);
                }
                if(curr_user != nullptr) {
                    remake_user_power_ups();
                    idx_selected_power_up = 0;
                    selected_power_up = "";
                }
            } else {
                if(curr_game_status == GameStatus::NOT_STARTED) {
                    ImGui::Begin("Settings", NULL, 0);
                    ImGui::Text("Logged in as:");
                    ImGui::TextColored(ImVec4(0.96, 0.76, 0.07, 1.0), "%s", curr_user->name.c_str());
                    if(ImGui::Combo("Select Map", &selected_map, maps_combo_choices.c_str())){
                        curr_map = maps[selected_map];
                    }
                    {
                        float avail = ImGui::GetContentRegionAvail().x;
                        avail -= SCREEN_W / 3;
                        ImGui::SetCursorPosX(avail / 2);
                    }
                    ImGui::Image((void*)curr_map->blended_txt, ImVec2(SCREEN_W / 3, SCREEN_H / 3));
                    draw_music_play_status(play_music, play_sfx, music_player);
                    if(ImGui::CollapsingHeader("Scores")){
                        draw_users_table(users);
                    }
                    if(ImGui::Combo("Select Power Up", &idx_selected_power_up, user_power_ups.c_str())) {
                        if(idx_selected_power_up > 0) {
                            auto ptr = curr_user->power_ups.begin();
                            int cidx = 1;
                            while(cidx != idx_selected_power_up) {
                                ptr++;
                                while(ptr->second == 0){
                                    ptr++;
                                }
                            }
                            selected_power_up = ptr->first;
                        } else {
                            selected_power_up = "None";
                        }
                    }
                    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 2 - 100);
                    if(ImGui::Button("Start", ImVec2(100, 30))) {
                        if(selected_power_up != "None"){
                            curr_user->power_ups[selected_power_up]--;
                        }
                        UID(color_gen, 0, ball_textures.size() - 1);
                        for(int i = 0; i < NUM_BALLS; i++) {
                            const int ball_color = color_gen(rng);
                            balls.push_back(new Ball(ball_color, ball_textures.at(ball_color), renderer, &(curr_map->locations)));
                            if(i > 0) {
                                balls.back()->should_render = false;
                            }
                        }
                        map_location = 0;
                        game_start_time = curr_time;
                        curr_game_status = GameStatus::PLAYING;
                    }
                    ImGui::SameLine();
                    if(ImGui::Button("Logout", ImVec2(100, 30))) {
                        curr_user = nullptr;
                    }

                    ImGui::End();
                } else if(curr_game_status == GameStatus::LOST) {
                    user_lost();
                    reset_game_status();
                } else if(curr_game_status == GameStatus::WON) {
                    user_won();
                    reset_game_status();
                } else if(curr_game_status == GameStatus::PLAYING) {
                    ImGui::Begin("Pause Game", NULL,
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing);
                    if(ImGui::Button("Pause")) {
                        curr_game_status = GameStatus::PAUSED;
                    }
                    ImGui::End();
                } else if(curr_game_status == GameStatus::PAUSED) {
                    ImGui::Begin("Game Paused", NULL,
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing);
                    draw_music_play_status(play_music, play_sfx, music_player);
                    if(ImGui::Button("Resume")) {
                        curr_game_status = GameStatus::PLAYING;
                    }
                    ImGui::End();
                }
            }
        }

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff); // sets the background color
        SDL_RenderClear(renderer);

        if(curr_game_status & GameStatus::IN_GAME) {
            curr_map->draw_background();
            if(curr_game_status == GameStatus::PLAYING && center_ball != nullptr && center_ball->is_moving) {
                shot_balls.push_back(center_ball);
                center_ball = nullptr;
            }
            remake_center_ball();
            pdd diff = last_mouse_pos - center;
            const double phase = vector_phase(diff);
            SDL_RenderCopyEx(renderer, holder_texture, NULL, &holder_rect, phase * RAD_TO_DEG, NULL, SDL_FLIP_NONE);//rotate and show wepon
            auto r = render_balls(balls, shot_balls, curr_game_status, dt, map_location_step, map_location);
            if(!(r & GameStatus::IN_GAME)) {
                curr_game_status = r;
            }
            assert(center_ball != nullptr);
            if(slowdown_effect) {
                SDL_RenderCopy(renderer, frost_texture, NULL, NULL);
            }
            center_ball->draw_ball();
        } else if(curr_game_status == GameStatus::NOT_STARTED) {
            step_slideshow(renderer, dt);
        }

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

    cleanup_slideshow();

    SDL_DestroyTexture(holder_texture);

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
