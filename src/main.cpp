#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <list>
#include <set>
#include <unistd.h>

#include <sodium.h>

#include "SDL.h"
#include "SDL_image.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include "ball.hpp"
#include "map.hpp"
#include "music-player.h"
#include "utils/common.h"
#include "utils/error_handling.h"
#include "gui.hpp"
#include "scheduler.hpp"
#include "slideshow.hpp"

using namespace std;

static bool should_record_map = false;
u64 user_score = 0;
MusicPlayer *music_player = nullptr;

char buf[100];

int selected_map = 0;
int curr_game_status = GameStatus::NOT_STARTED;
int game_mode = GameMode::FinishUp;
int idx_selected_power_up;
int target_num_balls = 0;

double map_location_step = 0;
double game_speed_factor = 1.0;

bool pause_cooldown = false;
bool play_music = true;
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
SDL_Rect holder_rect = {.x = (SCREEN_W - HOLDER_DIM) / 2,
                        .y = (SCREEN_H - HOLDER_DIM) / 2,
                        .w = HOLDER_DIM,
                        .h = HOLDER_DIM};

SDL_Texture *holder_texture;
SDL_Texture *frost_texture;

vector<Map *> maps;

list<Ball *> balls;
list<ShootingBall *> shot_balls;

set<int> in_game_ball_colors;

NewMap *new_map = nullptr;
Map *curr_map = nullptr;
UserMap users = get_user_list();
User *curr_user = nullptr;
ShootingBall *center_ball = nullptr;

pdd last_mouse_pos = {SCREEN_W >> 1, SCREEN_H >> 1}; // Do not change!
const pdd center = last_mouse_pos;

#include "functions.hpp"

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

int main(int argc, char **argv) {
  parse_arguments(argc, argv);

  // --- Starting Initialization!
  if (sodium_init() < 0) {
    cerr << "Could not initialize libsodium, quiting" << endl;
    return 1;
  }

  // Init video, timer and audio subsystems. Other subsystems like event are
  // initialized automatically by these options.
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s",
                 SDL_GetError());
    return 3;
  }

  music_player = new MusicPlayer();
  if (!INIT_SUCCESS(*music_player)) {
    return 1;
  }

  // Force OpenGL acceleration and allow High-DPI for ImGUI
  window = SDL_CreateWindow("BP-cpp", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, SCREEN_W, SCREEN_H,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

  // Use VSync to limit framerate
  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

  if (!window || !renderer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Couldn't create window and renderer: %s", SDL_GetError());
    return 3;
  }

  // setup and initialize sdl2_image library
  {
    int flag_IMG = IMG_INIT_PNG | IMG_INIT_JPG;
    int IMG_init_status = IMG_Init(flag_IMG);
    if ((IMG_init_status & flag_IMG) != flag_IMG) {
      cerr << "Required formats for SDL2Image where not initialized" << endl;
      return 3;
    }
  }
  // --- Initialization Done!

  if (should_record_map) {
    cerr << "I want to record a new map!" << endl;
    new_map = new NewMap(renderer);
  }

  if (play_music) {
    music_player->play_music(0); // Start from the first music
  }

  maps = get_all_maps(renderer);
  for (auto m : maps) {
    maps_combo_choices += m->path_name;
    maps_combo_choices += '\0';
  }

  curr_map = maps.at(selected_map);
  setup_textures();

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

  while (!should_quit) {
    while (!should_quit && SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      switch (event.type) {
      case SDL_QUIT:
        should_quit = true;
        break;

      case SDL_MOUSEMOTION:
        last_mouse_pos = {event.motion.x, event.motion.y};
        break;

      case SDL_MOUSEBUTTONDOWN:
        if (!io.WantCaptureMouse) {
          if (should_record_map) {
            new_map->add_point(last_mouse_pos);
          } else {
            if (last_mouse_pos == center) {
              last_mouse_pos.first += 1;
            }
            auto diff = last_mouse_pos - center;
            auto sz_sq = dot(diff, diff);
            auto dir = diff / sqrt(sz_sq);
            remake_center_ball();
            center_ball->direction = dir;
            center_ball->is_moving = dir.first != 0 || dir.second != 0;
            if (center_ball->is_moving) {
              music_player->play_sound("shoot.wav");
            }
          }
        }
        break;

      case SDL_MOUSEBUTTONUP:
        break;

      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_SPACE && should_record_map) {
          cerr << "I stopped recording coordinates!" << endl;
          should_quit = true;
        } else if (event.key.keysym.sym == SDLK_ESCAPE && !pause_cooldown &&
                   curr_game_status & GameStatus::IN_GAME) {
          if (curr_game_status == GameStatus::PLAYING) {
            curr_game_status = GameStatus::PAUSED;
          } else if (curr_game_status == GameStatus::PAUSED) {
            curr_game_status = GameStatus::PLAYING;
          }
          pause_cooldown = true;
          Scheduler::schedule.emplace(curr_time + 100, [&](const s64 _) {
            (void)_;
            pause_cooldown = false;
          });
        } else if (event.key.keysym.sym == SDLK_m) {
          music_player->play_music(-1); // go to the next music
        } else if (event.key.keysym.sym == SDLK_n &&
                   curr_game_status & GameStatus::IN_GAME) {
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
    curr_time = (s64)SDL_GetTicks64();
    double dt = curr_time - prev_time;
    dt /= 1000; // dt should be in seconds
    Scheduler::run(curr_time);

    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (!should_record_map) {
      if (curr_user == nullptr) {
        show_login_screen();
      } else {
        if (curr_game_status == GameStatus::NOT_STARTED) {
          show_main_menu();
        } else if (curr_game_status == GameStatus::LOST) {
          user_lost();
          reset_game_status();
        } else if (curr_game_status == GameStatus::WON) {
          user_won(user_score, maps, curr_map);
          reset_game_status();
        } else if (curr_game_status == GameStatus::PLAYING) {
          ImGui::Begin("Pause Game", NULL,
                       ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoFocusOnAppearing);
          if (ImGui::Button("Pause")) {
            curr_game_status = GameStatus::PAUSED;
          }
          ImGui::End();
        } else if (curr_game_status == GameStatus::PAUSED) {
          ImGui::Begin("Game Paused", NULL,
                       ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoFocusOnAppearing);
          draw_music_play_status(play_music, play_sfx, music_player);
          if (ImGui::Button("Resume", ImVec2(100, 30))) {
            curr_game_status = GameStatus::PLAYING;
          }
          if (ImGui::Button("Exit", ImVec2(100, 30))) {
            curr_game_status = GameStatus::NOT_STARTED;
            reset_game_status();
          }
          ImGui::End();
        }
      }
    }

    ImGui::Render();

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00,
                           0xff); // sets the background color
    SDL_RenderClear(renderer);

    if (curr_game_status & GameStatus::IN_GAME) {
      curr_map->draw_background();
      if (curr_game_status == GameStatus::PLAYING && center_ball != nullptr &&
          center_ball->is_moving) {
        shot_balls.push_back(center_ball);
        center_ball = nullptr;
      }
      remake_center_ball();
      pdd diff = last_mouse_pos - center;
      const double phase = vector_phase(diff);
      SDL_RenderCopyEx(renderer, holder_texture, NULL, &holder_rect,
                       phase * RAD_TO_DEG, NULL,
                       SDL_FLIP_NONE); // rotate and show weapon
      auto r = render_balls(balls, shot_balls, curr_game_status, dt,
                            map_location_step);
      if (!(r & GameStatus::IN_GAME)) {
        curr_game_status = r;
      }
      assert(center_ball != nullptr);
      if (slowdown_effect) {
        SDL_RenderCopy(renderer, frost_texture, NULL, NULL);
      }
      center_ball->draw_ball();
    } else if (curr_game_status == GameStatus::NOT_STARTED) {
      step_slideshow(renderer, dt);
    }

    if (should_record_map) {
      new_map->draw_status(last_mouse_pos);
    }

    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());

    prev_time = curr_time;

    SDL_RenderPresent(renderer);
  }

  if (new_map != nullptr) {
    delete new_map;
  }

  for (auto &x : maps) {
    delete x;
  }

  if (curr_user != nullptr) {
    curr_user->save_settings();
  }

  destroy_textures();
  cleanup_slideshow();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  IMG_Quit();
  SDL_Quit();

  save_user_list(users);

  return 0;
}
