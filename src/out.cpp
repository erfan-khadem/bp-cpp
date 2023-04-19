#pragma once

#include <map>
#include <utility>

#include "utils/common.h"

#include "SDL.h"
#include "SDL_image.h"

using std::map;

#define BALL_TEXT_COUNT 5
#define BALL_SIZE 64
#define BALL_PNG_SIZE 1280
// This is the size of the circle drawn on the screen
#define REAL_BALL_SIZE 50
#define GLOBAL_SPEED_FACTOR 0.6
#define SHOOTING_BALL_SPEED_COEFF (3000.0 * GLOBAL_SPEED_FACTOR)

#define MULTI_COLOR (1 << 16)
#define UNCOLORED (1 << 18)
#define FROZEN_COLOR (1 << 19)
#define COLOR_MSK 0xffff

static const double sq_2x_radius = BALL_SIZE * BALL_SIZE;

double prev_ball_dist = 0.0;

bool good_luck = false;

typedef const vector<pii> *PointerType;

vector<SDL_Texture *> ball_textures;

SDL_Texture *greyball_txt = nullptr;
SDL_Texture *freeze_txt = nullptr;
SDL_Texture *pause_txt = nullptr;
SDL_Texture *reverse_txt = nullptr;
SDL_Texture *colordots_txt = nullptr;
SDL_Texture *bomb_txt = nullptr;
SDL_Texture *fire_txt = nullptr;
SDL_Texture *uncolored_txt = nullptr;
SDL_Texture *frozenball_txt = nullptr;

class Ball {
public:
  double loc_index;
  int ball_color;
  bool should_render;
  bool freeze = false;
  bool pause = false;
  bool reverse = false;
  bool uncolored = false;
  bool frozen = false;
  bool is_bomb = false;
  bool is_fire = false;

  SDL_Rect position;
  SDL_Rect draw_position;

  SDL_Texture *ball_txt;
  SDL_Texture *render_txt;
  SDL_Renderer *renderer;

  const vector<pii> *locations;

  char buf[100];

  // _locs can be NULL
  Ball(const int _ball_color, SDL_Texture *cache_txt, SDL_Renderer *rend,
       const vector<pii> *const _locs) {
    loc_index = 0.0;
    locations = _locs;
    ball_color = _ball_color;
    should_render = true;
    renderer = rend;
    if (cache_txt == nullptr) {
      snprintf(buf, 90, "../art/images/ball%02d.png", ball_color);
      ball_txt = IMG_LoadTexture(renderer, buf);
    } else {
      ball_txt = cache_txt;
    }
    position.h = 1.25 * BALL_SIZE;
    position.w = 1.25 * BALL_SIZE;
    if (_locs == nullptr) {
      position.x = SCREEN_W >> 1;
      position.y = SCREEN_H >> 1;
    } else {
      assert(locations->size());
      position.x = locations->at(0).first;
      position.y = locations->at(0).second;
    }
    // Should not be a center ball
    URD(special_power, 0, 1);
    if (locations != nullptr) {
      if (special_power(rng) < 0.1) {
        double my_rnd = special_power(rng);
        if (my_rnd < 0.3) {
          reverse = true;
        } else if (my_rnd < 0.6) {
          pause = true;
        } else {
          freeze = true;
        }
      } else if(special_power(rng) < 0.1) {
        if(special_power(rng) < 0.5) {
          uncolored = true;
          ball_color = UNCOLORED;
          ball_txt = greyball_txt;
        } else {
          frozen = true;
          ball_color |= FROZEN_COLOR;
        }
      }
    } else {
      // Any color effect is for center ball only
      if (special_power(rng) < (good_luck ? 0.3 : 0.1)) {
        double val = special_power(rng);
        if(val < 0.5) {
          ball_color = MULTI_COLOR;
          ball_txt = greyball_txt;
        } else if(val < 0.75) {
          is_fire = true;
        } else {
          is_bomb = true;
        }
      }
    }
    render_txt = nullptr;
    create_texture();
  }

  ~Ball() {
    // DO NOT destroy ball_texture!
    SDL_DestroyTexture(render_txt);
  }

  void create_texture() {
    if(render_txt == nullptr){
      render_txt = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET, 1280, 1280);
    }
    assert(render_txt != nullptr);
    assert(SDL_SetRenderTarget(renderer, render_txt) == 0);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, ball_txt, NULL, NULL);
    auto rect = SDL_Rect{.x = 384, .y = 384, .w = 512, .h = 512};
    if (ball_color & MULTI_COLOR) {
      auto rect2 = SDL_Rect{.x = 384, .y = 417, .w = 512, .h = 445};
      SDL_RenderCopy(renderer, colordots_txt, NULL, &rect2);
    } else if(is_bomb) {
      SDL_RenderCopy(renderer, bomb_txt, NULL, &rect);
    } else if(is_fire) {
      SDL_RenderCopy(renderer, fire_txt, NULL, &rect);
    }
    if (pause) {
      SDL_RenderCopy(renderer, pause_txt, NULL, &rect);
    } else if (freeze) {
      SDL_RenderCopy(renderer, freeze_txt, NULL, &rect);
    } else if (reverse) {
      SDL_RenderCopy(renderer, reverse_txt, NULL, &rect);
    }
    if (frozen) {
      SDL_RenderCopy(renderer, frozenball_txt, NULL, &rect);
    } else if (uncolored) {
      SDL_RenderCopy(renderer, uncolored_txt, NULL, &rect);
    }
    SDL_SetRenderTarget(renderer, NULL);
    SDL_SetTextureBlendMode(render_txt, SDL_BLENDMODE_BLEND);

  }

  pii get_center() const {
    return pii{position.x + (BALL_SIZE >> 1), position.y + (BALL_SIZE >> 1)};
  }

  double get_previous_ball_pos() {
    return loc_index - prev_ball_dist;
  }

  void move_to_location(const double pos) {
    assert(locations != nullptr);
    if (pos < 0 || pos >= int(locations->size())) {
      position.x = -1000;
      position.y = -1000;
      should_render = false;
      return;
    }
    loc_index = pos;
    should_render = true;
    position.x = locations->at((int)pos).first;
    position.y = locations->at((int)pos).second;
  }

  void draw_ball() {
    draw_position = position;
    draw_position.x -= position.h >> 1;
    draw_position.y -= position.h >> 1;
    if (should_render) {
      SDL_RenderCopy(renderer, render_txt, NULL, &draw_position);
    }
  }

  bool collides(const Ball &other) const {
    int d = square_euclid_dist(pii{position.x, position.y},
                               pii{other.position.x, other.position.y});
    if (d <= (REAL_BALL_SIZE * REAL_BALL_SIZE)) {
      return true;
    }
    return false;
  }

  bool unfreeze() {
    if(ball_color & FROZEN_COLOR) {
      ball_color ^= FROZEN_COLOR;
      frozen = false;
      create_texture();
      return true;
    }
    return false;
  }

  bool recolor(const Ball &other) {
    if(ball_color & UNCOLORED) {
      assert(other.uncolored == false);
      ball_color ^= UNCOLORED;
      ball_color = other.ball_color & COLOR_MSK;
      ball_txt = other.ball_txt;
      uncolored = false;
      create_texture();
      return true;
    }
    return false;
  }

  bool same_color(Ball &other) {
    if(other.unfreeze() | this->unfreeze()) {
      return false;
    }
    if(recolor(other) | other.recolor(*this)) {
      return false;
    }
    bool result = false;
    result |= (other.ball_color | ball_color) & MULTI_COLOR;
    result |= (other.ball_color & COLOR_MSK) == (ball_color & COLOR_MSK);
    return result;
  }
};

class ShootingBall : public Ball {
private:
public:
  bool is_moving = false;
  pdd direction = {0.0, 0.0};
  pdd fine_position = {SCREEN_W >> 1, SCREEN_H >> 1};

  using Ball::Ball;

  void step_dt(const double dt, bool set_int_pos = true) {
    fine_position += direction * (dt * SHOOTING_BALL_SPEED_COEFF);
    if (set_int_pos) {
      move_to_fine_position();
    }
  }

  void reset_fine_position() {
    fine_position.first = position.x;
    fine_position.second = position.y;
  }

  void move_to_fine_position() {
    position.x = fine_position.first;
    position.y = fine_position.second;
  }

  bool in_screen() const {
    bool ok = true;
    ok &= position.x < (SCREEN_W + 50) && position.x > -50;
    ok &= position.y < (SCREEN_H + 50) && position.y > -50;
    return ok;
  }

  Ball &to_normal_ball(const vector<pii> *const _locs, const int _b_col,
                       SDL_Texture *col_txt) {
    move_to_fine_position();
    assert(locations == nullptr);
    assert(_locs != nullptr);
    locations = _locs;
    ball_color = _b_col;
    assert(SDL_SetRenderTarget(renderer, render_txt) == 0);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, col_txt, NULL, NULL);
    SDL_SetRenderTarget(renderer, NULL);
    return *this;
  }
  Ball &to_normal_ball(const vector<pii> *const _locs) {
    move_to_fine_position();
    assert(locations == nullptr);
    assert(_locs != nullptr);
    locations = _locs;
    return *this;
  }
};#pragma once

#include "SDL.h"
#include "SDL_image.h"

#include "utils/common.h"
void enable_reverse_effect() {
  if (reverse_effect) {
    return;
  }
  reverse_effect = true;
  pause_effect = false;
  slowdown_effect = false;
  music_player->play_sound("rewind.wav");
  Scheduler::schedule.emplace(curr_time + 5'000, [&](const s64 _) {
    (void)_;
    reverse_effect = false;
  });
}

void enable_pause_effect() {
  if (reverse_effect || pause_effect) {
    return;
  }
  pause_effect = true;
  slowdown_effect = false;
  music_player->play_sound("pause.wav");
  Scheduler::schedule.emplace(curr_time + 5'000, [&](const s64 _) {
    (void)_;
    pause_effect = false;
  });
}

void enable_slowdown_effect() {
  if (slowdown_effect || pause_effect || reverse_effect) {
    return;
  }
  slowdown_effect = true;
  music_player->play_sound("freeze.wav");
  Scheduler::schedule.emplace(curr_time + 5'000, [&](const s64 _) {
    (void)_;
    slowdown_effect = false;
  });
}

void remake_user_power_ups() {
  user_power_ups = "";
  user_power_ups += "None";
  user_power_ups += '\0';
  for (const auto &[name, cnt] : curr_user->power_ups) {
    if (cnt <= 0) {
      continue;
    }
    int curr_size = name.size();
    user_power_ups += name;
    assert(curr_size < 16);
    while (curr_size < 16) {
      user_power_ups.push_back(' ');
      curr_size++;
    }
    user_power_ups += to_string(cnt);
    user_power_ups += "\0";
  }
  user_power_ups += "\0";
}

void remake_center_ball() {
  if (center_ball == nullptr) {
    UID(cball_color, 0, ball_textures.size() - 1);
    int col = cball_color(rng);
    while (in_game_ball_colors.size() &&
           in_game_ball_colors.find(col) == in_game_ball_colors.end()) {
      col = cball_color(rng);
    }
    center_ball = new ShootingBall(col, ball_textures[col], renderer, NULL);
  }
}

void reset_game_status() {
  if (schedule_game_status == false) {
    schedule_game_status = true;
    Scheduler::schedule.clear();
    Scheduler::schedule.emplace(curr_time + 5'000, [&](const s64 _) {
      (void)_;
      schedule_game_status = false;
      curr_game_status = GameStatus::NOT_STARTED;
      user_score = 0;
      good_luck = false;
    });
    for (auto &b : balls) {
      delete b;
    }
    for (auto &b : shot_balls) {
      delete b;
    }
    balls.clear();
    shot_balls.clear();
    if (center_ball != nullptr) {
      delete center_ball;
      center_ball = nullptr;
    }
    if (curr_user != nullptr) {
      if (curr_user->max_score < user_score) {
        curr_user->max_score_time = get_utc_secs();
        curr_user->max_score = user_score;
      }
      if (user_score >= 1000)
        curr_user->power_ups[PUP_LUCK]++;

      remake_user_power_ups();
    }
  }
}

void generate_game_balls(const int cnt_balls) {
  UID(color_gen, 0, ball_textures.size() - 1);
  for (int i = 0; i < cnt_balls; i++) {
    const int ball_color = color_gen(rng);
    balls.push_back(new Ball(ball_color, ball_textures.at(ball_color), renderer,
                             &(curr_map->locations)));
    if (i > 0) {
      balls.back()->should_render = false;
    }
  }
}

void show_login_screen() {
  if (DEBUG) {
    curr_user = &users.begin()->second;
  } else {
    curr_user = login_or_register(users);
  }
  if (curr_user != nullptr) {
    if(curr_user->play_music != play_music){
      if(curr_user->play_music) {
        music_player->unpause_music();
      } else {
        music_player->pause_music();
      }
    }
    if(curr_user->play_sfx != play_sfx) {
      if(curr_user->play_sfx) {
        music_player->unpause_sound();
      } else {
        music_player->pause_sound();
      }
    }
    curr_user->load_settings();
    remake_user_power_ups();
  }
}

void show_main_menu() {
  ImGui::Begin("Settings", NULL, 0);
  ImGui::Text("Logged in as:");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.96, 0.76, 0.07, 1.0), "%s",
                     curr_user->name.c_str());
  if (ImGui::Combo("Select Map", &selected_map, maps_combo_choices.c_str())) {
    curr_map = maps[selected_map];
  }
  {
    float avail = ImGui::GetContentRegionAvail().x;
    avail -= SCREEN_W / 3;
    ImGui::SetCursorPosX(avail / 2);
  }
  ImGui::Image((void *)curr_map->blended_txt,
               ImVec2(SCREEN_W / 3, SCREEN_H / 3));
  draw_music_play_status(play_music, play_sfx, music_player);
  if (ImGui::CollapsingHeader("Scores")) {
    draw_users_table(users);
  }
  ImGui::Combo("Select Power Up", &idx_selected_power_up,
               user_power_ups.c_str());
  ImGui::Combo("Select Game Mode", &game_mode, GAMEMODES);
  ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 2 - 100);
  if (ImGui::Button("Start", ImVec2(100, 30))) {
    // TODO: Fix
    if (idx_selected_power_up > 0) {
      auto ptr = curr_user->power_ups.begin();
      int cidx = 1;
      while (cidx != idx_selected_power_up) {
        ptr++;
        while (ptr->second == 0) {
          ptr++;
        }
        cidx++;
      }
      selected_power_up = ptr->first;
      curr_user->power_ups[selected_power_up]--;
    } else {
      selected_power_up = "None";
    }
    assert(curr_map != nullptr);
    target_num_balls = curr_map->locations.size() / 40;
    if (game_mode == GameMode::FinishUp) {
      generate_game_balls(target_num_balls);
    } else if (game_mode == GameMode::Timed) {
      generate_game_balls(200);
    }
    game_start_time = curr_time;
    curr_game_status = GameStatus::PLAYING;
    prev_ball_dist = curr_map->prev_ball_dist;
    game_speed_factor = curr_map->prev_ball_dist / 30;
  }
  if (selected_power_up == PUP_LUCK) {
    good_luck = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Logout", ImVec2(100, 30))) {
    curr_user->save_settings();
    curr_user = nullptr;
  }
  ImGui::End();
}

void setup_textures() {
  motivational_txt =
      IMG_LoadTexture(renderer, "../art/images/motivational.png");
  for (int i = 1; i <= BALL_TEXT_COUNT; i++) {
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
  uncolored_txt = IMG_LoadTexture(renderer, "../art/images/color-circle.png");
  frozenball_txt = IMG_LoadTexture(renderer, "../art/images/snowflake.png");
  bomb_txt = IMG_LoadTexture(renderer, "../art/images/bomb.png");
  fire_txt = IMG_LoadTexture(renderer, "../art/images/fire.png");
}

void destroy_textures() {
  SDL_DestroyTexture(holder_texture);
  SDL_DestroyTexture(greyball_txt);
  SDL_DestroyTexture(freeze_txt);
  SDL_DestroyTexture(pause_txt);
  SDL_DestroyTexture(reverse_txt);
  SDL_DestroyTexture(colordots_txt);
  SDL_DestroyTexture(bomb_txt);
  SDL_DestroyTexture(fire_txt);
  SDL_DestroyTexture(motivational_txt);
  SDL_DestroyTexture(frozenball_txt);
  SDL_DestroyTexture(uncolored_txt);

  for (auto &txt : ball_textures) {
    SDL_DestroyTexture(txt);
  }
}

bool clear_balls(list<Ball *> &balls, list<Ball *>::iterator &it) {
  auto mn_ptr = it;
  auto mx_ptr = it;
  auto pptr = it;
  int cnt = 2;
  while (mn_ptr != balls.begin()) {
    pptr = mn_ptr;
    mn_ptr--;
    if (!(*mn_ptr)->same_color(**pptr)) {
      mn_ptr++;
      break;
    }
    cnt++;
  }

  while (mx_ptr != balls.end()) {
    pptr = mx_ptr;
    mx_ptr++;
    if (mx_ptr == balls.end() || !(*mx_ptr)->same_color(**pptr)) {
      mx_ptr--;
      break;
    }
    cnt++;
  }
  if (cnt >= 3) {
    assert((*mn_ptr)->same_color(**mx_ptr));
    assert(it != mx_ptr || it != mn_ptr);
    assert(mn_ptr != mx_ptr);
    mx_ptr++;
    it = mx_ptr;
    for (auto pt = mn_ptr; pt != mx_ptr;) {
      if ((*pt)->freeze) {
        enable_slowdown_effect();
      } else if ((*pt)->pause) {
        enable_pause_effect();
      } else if ((*pt)->reverse) {
        enable_reverse_effect();
      }
      delete *pt;
      pt = balls.erase(pt);
    }
    assert(it == mx_ptr);
    user_score += 10 * cnt;
    return true;
  }
  return false;
}

int render_balls(list<Ball *> &balls, list<ShootingBall *> &shot_balls, int gs,
                 const double dt, double &step) {
  if (balls.empty()) {
    if (game_mode == GameMode::Timed) {
      double time_sc = (FINISHUP_MAXTIME / GLOBAL_SPEED_FACTOR) -
                       (curr_time - game_start_time);
      time_sc = max(0.0, time_sc);
      user_score += time_sc / 10;
    }
    return GameStatus::WON;
  } else if (game_mode == GameMode::Timed &&
             (curr_time - game_start_time) > TIMED_MODE_MAXTIME) {
    double time_sc = (FINISHUP_MAXTIME / GLOBAL_SPEED_FACTOR) -
                     (curr_time - game_start_time);
    time_sc = max(0.0, time_sc);
    user_score += time_sc / 10;
    return GameStatus::WON;
  }
  if (gs == GameStatus::PLAYING && !pause_effect) {
    step = GLOBAL_SPEED_FACTOR * dt * game_speed_factor * 100 * (slowdown_effect ? 0.5 : 1.0);
    step = reverse_effect ? -step : step;
  } else {
    step = 0;
  }
  if (gs == GameStatus::PLAYING) {
    auto ptr = shot_balls.begin();
    for (auto b : shot_balls) {
      b->step_dt(dt);
      b->draw_ball();
    }
    while (ptr != shot_balls.end()) {
      (*ptr)->step_dt(dt);
      if (!(*ptr)->in_screen()) {
        delete *ptr;
        ptr = shot_balls.erase(ptr);
      } else {
        (*ptr)->draw_ball();
        ptr++;
      }
    }
  }
  balls.front()->move_to_location(balls.front()->loc_index + step);
  auto ptr = balls.begin();
  for (int i = 1; i < int(balls.size()); i++) {
    ptr++;
    if ((*ptr)->should_render) {
      double new_pos = (*ptr)->loc_index + step;
      (*ptr)->move_to_location(new_pos);
      if ((*ptr)->should_render == false && (*ptr)->loc_index > 100 &&
          game_mode == GameMode::FinishUp) {
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
  for (; ptr != balls.end() && (*ptr)->should_render; ptr++) {
    sptr = shot_balls.begin();
    while (sptr != shot_balls.end()) {
      if ((*ptr)->collides(**sptr)) {
        if ((*sptr)->is_fire) {
          music_player->play_sound("fire.wav");
          (*sptr)->is_fire = false;
          (*sptr)->create_texture();
          auto tmp = balls.begin();
          while (tmp != balls.end()) {
            if ((*tmp)->should_render == false && (*tmp)->loc_index < 10) {
              break;
            }
            (*tmp)->unfreeze();
            tmp++;
          }
        } else if ((*sptr)->is_bomb) {
          music_player->play_sound("bomb.wav");
          auto fptr = ptr;
          for (int i = 0; i < 3 && fptr != balls.begin(); i++) {
            fptr--;
            (*fptr)->ball_color = 0;
            (*fptr)->frozen = false;
            (*fptr)->uncolored = false;
          }
          fptr = ptr;
          for (int i = 0; i < 3 && fptr != balls.end(); i++, fptr++) {
            (*fptr)->ball_color = 0;
            (*fptr)->frozen = false;
            (*fptr)->uncolored = false;
          }
          (*sptr)->ball_color = 0;
          (*sptr)->is_bomb = false;
        }
        recalculate = true;
        if ((*ptr)->same_color(**sptr) && clear_balls(balls, ptr)) {
          delete *sptr;
          sptr = shot_balls.erase(sptr);
          ptr--;
        } else if (++ptr != balls.end() && (*ptr)->same_color(**sptr) &&
                   clear_balls(balls, ptr)) {
          delete *sptr;
          sptr = shot_balls.erase(sptr);
          ptr--;
          ptr--;
        } else {
          ptr--;
          auto bptr = ptr;
          if ((*sptr)->ball_color == MULTI_COLOR) {
            balls.insert(++ptr, &(*sptr)->to_normal_ball((*bptr)->locations,
                                                         (*bptr)->ball_color,
                                                         (*bptr)->ball_txt));
          } else {
            balls.insert(++ptr, &(*sptr)->to_normal_ball((*bptr)->locations));
          }
          // Do not delete sptr!
          sptr = shot_balls.erase(sptr);
          ptr--;
        }
      } else {
        sptr++;
      }
    }
  }
  if (balls.empty()) {
    return GameStatus::WON;
  }
  if (recalculate) {
    music_player->play_sound("collision.wav");
    ptr = balls.begin();
    for (int i = 1; i < int(balls.size()); i++) {
      const double pos = (*ptr)->get_previous_ball_pos();
      (*(++ptr))->move_to_location(pos);
      if ((*ptr)->should_render == false && (*ptr)->loc_index < 10) {
        break;
      }
    }
  }
  in_game_ball_colors.clear();
  ptr = balls.begin();
  while (ptr != balls.end()) {
    auto b = *ptr;
    if (b->loc_index > 100 && b->should_render == false &&
        game_mode == GameMode::FinishUp) {
      return GameStatus::LOST;
    } else if (b->loc_index > 100 && b->should_render == false) {
      delete b;
      ptr = balls.erase(ptr);
      continue;
    }
    in_game_ball_colors.insert(b->ball_color & COLOR_MSK);
    b->draw_ball();
    ptr++;
  }
  return GameStatus::PLAYING;
}#pragma once

#include "imgui.h"
#include "utils/common.h"

#include "user.h"

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error ImGUI requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

SDL_Texture *motivational_txt = nullptr;

void AlignForWidth(float width, float alignment = 0.5f) {
  float avail = ImGui::GetContentRegionAvail().x;
  float off = (avail - width) * alignment;
  if (off > 0.0f)
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}

User *login_or_register(UserMap &umap) {
  static bool should_register = false;
  static bool should_show_password = false;
  static ImGuiInputTextFlags password_flags = ImGuiInputTextFlags_Password;

  static string username;
  static string password;
  static string retype_pass;

  bool passwords_match = false;

  username.resize(32, 0x00);
  password.resize(32, 0x00);
  retype_pass.resize(32, 0x00);

  ImGui::Begin("Login or Register", NULL, 0);

  ImGui::InputText("User Name", &username[0], username.size());
  ImGui::InputText("Password", &password[0], password.size(),
                   should_show_password ? 0 : password_flags);
  ImGui::SameLine();
  ImGui::Checkbox("Show Password", &should_show_password);

  ImGui::Checkbox("Register", &should_register);

  if (should_register && !should_show_password) {
    ImGui::InputText("Retype Password", &retype_pass[0], retype_pass.size(),
                     should_show_password ? 0 : password_flags);
  }

  float button_width = 0;
  button_width += 100;
  button_width += should_register ? 200 : 0;

  AlignForWidth(button_width);

  if (ImGui::Button("Login", ImVec2(100, 30))) {
    auto usr = umap.find(username);
    if (usr == umap.end()) {
      float width = ImGui::CalcTextSize("User Not Found!").x;
      AlignForWidth(width);
      ImGui::TextColored({1.0, 0.2, 0.2, 1.0}, "User Not Found");
      goto end_draw;
    }
    auto &user = usr->second;
    if (verify_user(user, password)) {
      ImGui::End();
      return &user;
    }
  }
show_registration:
  if (should_register) {
    passwords_match = should_show_password ||
                      (strcmp(password.c_str(), retype_pass.c_str()) == 0);
    ImGui::SameLine();
    if (!passwords_match) {
      ImGui::BeginDisabled();
    }
    if (ImGui::Button("Register User", ImVec2(200, 30))) {
      User *user = nullptr;
      if (umap.find(username) != umap.end()) {
        float width = ImGui::CalcTextSize("User Already Exists!").x;
        AlignForWidth(width);
        ImGui::TextColored({1.0, 0.2, 0.2, 1.0}, "User Already Exists!");
        goto end_draw;
      }
      user = new User(username, password);
      umap[username] = *user;
      ImGui::End();
      return user;
    }
    if (!passwords_match) {
      ImGui::EndDisabled();
      float width = ImGui::CalcTextSize("Passwords do not match!").x;
      AlignForWidth(width);
      ImGui::TextColored({1.0, 0.2, 0.2, 1.0}, "Passwords do not match!");
    }
  }
end_draw:
  ImGui::End();

  return NULL;
}

void user_lost() {
  ImGui::Begin("You Lost", NULL, ImGuiWindowFlags_NoTitleBar);
  float width = ImGui::CalcTextSize("You Lost :(").x;
  AlignForWidth(width);
  ImGui::TextColored(ImVec4(0.87, 0.10, 0.14, 1.0), "You Lost :(");
  AlignForWidth(SCREEN_W / 3);
  ImGui::Image((void *)motivational_txt, ImVec2(SCREEN_W / 3, SCREEN_W / 3));
  ImGui::End();
}

void user_won(u64 user_score, vector<Map *> &maps, Map *curr_map) {
  static char buf[100];
  static u64 prev_score = UINT64_MAX;
  ImGui::Begin("You Won!", NULL, ImGuiWindowFlags_NoTitleBar);
  float width = ImGui::CalcTextSize("You Won!").x;
  AlignForWidth(width);
  ImGui::TextColored(ImVec4(0.18, 0.76, 0.49, 1.0), "You Won!");
  if(user_score != prev_score){
    snprintf(buf, 90, "Score: %lu", user_score);
    prev_score = user_score;
  }
  width = ImGui::CalcTextSize(buf).x;
  AlignForWidth(width);
  ImGui::TextColored(ImVec4(0.96, 0.76, 0.07, 1.0), buf);
  if(user_score >= 1000) {
    width = ImGui::CalcTextSize("New Power UP: GoodLuck").x;
    AlignForWidth(width);
    ImGui::TextColored(ImVec4(0.96, 0.76, 0.07, 1.0), "New Power UP: GoodLuck");
  }
  width = ImGui::CalcTextSize("Try Other maps:").x;
  AlignForWidth(width);
  ImGui::Text("Try Other maps:");
  AlignForWidth((2 * SCREEN_W) / 3);
  int cnt = 0;
  for(int i = 0; i < (int)maps.size() && cnt != 2; i++) {
    if(maps.at(i) == curr_map) {
      continue;
    }
    if(cnt >= 1) {
      ImGui::SameLine();
    }
    cnt++;
    ImGui::Image((void*)maps.at(i)->blended_txt, ImVec2(SCREEN_W / 3, SCREEN_H / 3));
  }
  ImGui::End();
}

void draw_music_play_status(bool &play_music, bool &play_sfx,
                            MusicPlayer *music_player) {
  if (ImGui::Checkbox("Play Music", &play_music)) {
    if (play_music) {
      music_player->unpause_music();
    } else {
      music_player->pause_music();
    }
  }

  if (ImGui::Checkbox("Play SFX", &play_sfx)) {
    if (play_sfx) {
      music_player->unpause_sound();
    } else {
      music_player->pause_sound();
    }
  }
}

void draw_users_table(const UserMap &users) {
  const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
  vector<User> sorted_users;
  char max_score[32];
  for (const auto &[_, u] : users) {
    sorted_users.push_back(u);
  }
  stable_sort(
      sorted_users.begin(), sorted_users.end(),
      [](const User &a, const User &b) { return a.max_score > b.max_score; });
  if (ImGui::BeginTable("Top Scores", 4, flags)) {
    // Add headers
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Max Score");
    ImGui::TableSetupColumn("Score Date");
    ImGui::TableSetupColumn("Register Date");
    ImGui::TableHeadersRow();

    for (size_t i = 0; i < 10 && i < sorted_users.size(); i++) {
      ImGui::TableNextRow();
      const auto &u = sorted_users[i];
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(u.name.c_str());

      ImGui::TableSetColumnIndex(1);
      snprintf(max_score, 30, "%lu", u.max_score);
      ImGui::TextUnformatted(max_score);

      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(format_utc_time(u.max_score_time).c_str());

      ImGui::TableSetColumnIndex(3);
      ImGui::TextUnformatted(format_utc_time(u.register_time).c_str());
    }
    ImGui::EndTable();
  }
}#include <cmath>
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
#pragma once

#include <filesystem>
#include <fstream>
#include <regex>

#include "SDL.h"
#include "SDL_image.h"

#include "utils/common.h"

#define MAPS_PATH "../art/maps/"
#define NEWMAP_NAME "generator/new_locations.txt"
#define NEWMAP_COLOR                                                           \
  SDL_Color { 0x35, 0x84, 0xe4, 0xff }
#define NEWMAP_SEC_COLOR                                                       \
  SDL_Color { 0xf6, 0xd3, 0x2d, 0xff }

static void strip_format(string &s) {
  auto ptr = s.rfind('.');
  s.erase(ptr);
}

static void normalize_name(string &s) {
  for (auto &c : s) {
    if (c == '-' || c == '_') {
      c = ' ';
    }
  }
}

using std::ifstream;
using std::ofstream;
using std::regex;

class Map {
public:
  int num_points;
  double prev_ball_dist;

  SDL_Renderer *rend;

  string path_name;
  string background_name;

  string locs_path;
  string path_path;
  string bkg_path;

  vector<pii> locations;

  SDL_Texture *path_txt;
  SDL_Texture *background_txt;
  SDL_Texture *blended_txt;

  Map(const string &_path_name, const string &_bkg_name,
      SDL_Renderer *const _renderer)
      : rend(_renderer), path_name(_path_name), background_name(_bkg_name) {

    normalize_name(path_name);
    normalize_name(background_name);

    path_path = MAPS_PATH + _path_name + ".png";
    bkg_path = MAPS_PATH + _bkg_name + ".png";

    path_txt = IMG_LoadTexture(rend, path_path.c_str());
    background_txt = IMG_LoadTexture(rend, bkg_path.c_str());
    blended_txt =
        SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888,
                          SDL_TEXTUREACCESS_TARGET, SCREEN_W, SCREEN_H);

    assert(path_txt != nullptr);
    assert(background_txt != nullptr);
    assert(blended_txt != nullptr);

    assert(SDL_SetRenderTarget(rend, blended_txt) == 0);
    SDL_RenderCopy(rend, background_txt, NULL, NULL);
    SDL_SetTextureBlendMode(path_txt, SDL_BLENDMODE_ADD);
    SDL_RenderCopy(rend, path_txt, NULL, NULL);
    SDL_SetRenderTarget(rend, NULL);

    locs_path = MAPS_PATH + _path_name + ".txt";

    ifstream map_file(locs_path);
    assert(map_file.is_open());
    {
      int number_of_coordinates = 0;

      map_file >> prev_ball_dist;
      map_file >> number_of_coordinates;
      for (int i = 0; i < number_of_coordinates; i++) {
        int x, y;
        map_file >> x >> y;
        locations.emplace_back(x, y);
      }
      map_file.close();
    }
    num_points = locations.size();
  }
  ~Map() {
    SDL_DestroyTexture(path_txt);
    SDL_DestroyTexture(background_txt);
    SDL_DestroyTexture(blended_txt);
  }

  void draw_background() { SDL_RenderCopy(rend, blended_txt, NULL, NULL); }
};

vector<Map *> get_all_maps(SDL_Renderer *rend) {
  vector<Map *> result;
  regex str_expr("^map-(.*)\\.txt$");
  for (const auto &entry : std::filesystem::directory_iterator(MAPS_PATH)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    string name = entry.path().filename();
    if (regex_match(name.begin(), name.end(), str_expr)) {
      strip_format(name);
      string bkg_name = "background" + name.substr(3);
      result.push_back(new Map(name, bkg_name, rend));
    }
  }
  return result;
}

class NewMap {
public:
  vector<pii> added_points;
  ofstream output_file;

  SDL_Renderer *rend;

  NewMap(SDL_Renderer *_rend) {
    output_file.open(MAPS_PATH NEWMAP_NAME);
    assert(output_file.good());

    rend = _rend;
  }
  ~NewMap() {
    output_file.close();
    char buf[100];
    char buf2[100];
    string result;
    UID(map_gen, 100, 999'999);
    int rnd = map_gen(rng);
    snprintf(buf, 90, "../art/maps/%%s-%06d.%%s", rnd);
    snprintf(buf2, 90, buf, "map", "txt");
    result = "python3 ../art/maps/generator/interpolation.py ";
    result += MAPS_PATH NEWMAP_NAME " ";
    result += buf2;
    system(result.c_str());

    result = "python3 ../art/maps/generator/mask.py ";
    result += buf2;
    result += ' ';
    snprintf(buf2, 90, buf, "map", "png");
    result += buf2;
    int res = system(result.c_str());

    if (res != 0) {
      return;
    }

    result = "cp ../art/maps/background-01.png ";
    snprintf(buf2, 90, buf, "background", "png");
    result += buf2;
    system(result.c_str());

    result = "cp " MAPS_PATH NEWMAP_NAME " ";
    snprintf(buf2, 90, buf, "raw_map", "txt");
    result += buf2;
    system(result.c_str());
  }

  void add_point(const pii location) {
    added_points.push_back(location);
    output_file << location.first << ' ' << location.second << endl;
  }

  void draw_status(const pii mouse_pos) {
    if (added_points.empty()) {
      return;
    }
    RENDER_COLOR(rend, NEWMAP_COLOR);
    for (int i = 1; i < (int)added_points.size(); i++) {
      SDL_RenderDrawLine(rend, added_points[i - 1].first,
                         added_points[i - 1].second, added_points[i].first,
                         added_points[i].second);
    }
    auto [x, y] = added_points.back();
    RENDER_COLOR(rend, NEWMAP_SEC_COLOR);
    SDL_RenderDrawLine(rend, x, y, mouse_pos.first, mouse_pos.second);
  }
};#include "music-player.h"

#include <filesystem>
#include <regex>

#include "utils/common.h"

using namespace std;

#define MUSIC_PATH "../art/music/"

static MusicPlayer *main_music_player = nullptr;

static void music_stopped(void) {
  assert(main_music_player != nullptr);
  main_music_player->play_music(-1);
}

MusicPlayer::MusicPlayer() {
  music = nullptr;
  stop_music = false;
  stop_sound = false;
  curr_music = 0;
  success = true;
  if (main_music_player != nullptr) {
    cerr << "Cannot have more than one music player at a time" << endl;
    success = false;
    return;
  }
  main_music_player = this;
  if (Mix_OpenAudio(48'000, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    cerr << "SDL_mixer not initialized: " << Mix_GetError() << endl;
    success = false;
    return;
  }
  // Add in game music files in here
  regex str_expr("^music-(.*)$");
  for (const auto &entry : filesystem::directory_iterator(MUSIC_PATH)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    string name = entry.path().filename();
    if (regex_match(name.begin(), name.end(), str_expr)) {
      in_game_music.push_back(name);
    }
  }
  shuffle(in_game_music.begin(), in_game_music.end(), rng);
  assert(in_game_music.size() > 0);
  Mix_HookMusicFinished(&music_stopped);
}

MusicPlayer::~MusicPlayer() {
  if (music != nullptr) {
    Mix_FreeMusic(music);
  }
  Mix_Quit();
}

int MusicPlayer::play_music(int index) {
  if (stop_music) {
    return -1;
  }
  UID(index_gen, 0, in_game_music.size());
  if (index < 0) {
    switch (index) {
    case -1:
      index = (curr_music + 1) % in_game_music.size();
      curr_music = index;
      break;

    case -2:
      index = index_gen(rng);
      curr_music = index;
      break;

    case -3:
      index = curr_music;
      break;

    default:
      cerr << "Invalid music index: " << index << ". ";
      cerr << "Selecting a song randomly." << endl;
      UID(index_gen, 0, in_game_music.size() - 1);
      index = index_gen(rng);
      curr_music = index;
      break;
    }
  }
  assert(index < (int)in_game_music.size());
  if (music != nullptr) {
    Mix_FreeMusic(music);
  }
  const string file_name = MUSIC_PATH + in_game_music[index];
  music = Mix_LoadMUS(file_name.c_str());
  if (music == nullptr) {
    cerr << "Could not load music with index " << index << " and name "
         << in_game_music[index] << endl;
    return -1;
  }
  Mix_PlayMusic(music, 0);
  return index;
}

int MusicPlayer::play_sound(const string &filename) {
  if (this->stop_sound) {
    return 0;
  }
  Mix_Chunk *tmp = nullptr;
  if (effects_cache.find(filename) != effects_cache.end()) {
    tmp = effects_cache[filename];
  } else {
    const string comp_fn = MUSIC_PATH + filename;
    tmp = Mix_LoadWAV(comp_fn.c_str());
    effects_cache[filename] = tmp;
  }
  if (tmp == nullptr) {
    cerr << "Could not load wav with filename " << filename << endl;
    return -1;
  }
  int ret = Mix_PlayChannel(-1, tmp, 0);
  if (ret < 0) {
    cerr << "Could not play wav with filename " << filename << endl;
    return -2;
  }
  return 0;
}

void MusicPlayer::pause_music() {
  this->stop_music = true;
  Mix_HaltMusic();
}

void MusicPlayer::unpause_music() {
  this->stop_music = false;
  this->play_music(-3);
}

void MusicPlayer::pause_sound() { this->stop_sound = true; }

void MusicPlayer::unpause_sound() { this->stop_sound = false; }#pragma once

#include <map>
#include <string>
#include <vector>

#include "SDL_mixer.h"

void basic_music();

class MusicPlayer {
private:
  Mix_Music *music;

  /* The index of the background music currently playing */
  int curr_music;
  bool stop_music;
  bool stop_sound;

public:
  std::vector<std::string> in_game_music;
  std::map<std::string, Mix_Chunk *> effects_cache;
  bool success;
  /* Plays background music. If `index` is -1, it selects the next song
   *  else if `index` is -2, it selects a song randomly.
   *  else if `index` is -3, it continues from the last song it was playing
   *  Returns the index of the currently playing song.
   */
  int play_music(int index);

  /*
   * Plays a .wav files.
   * Returns zero on success.
   */
  int play_sound(const std::string &filename);
  void pause_music();
  void unpause_music();
  void pause_sound();
  void unpause_sound();

  MusicPlayer();
  ~MusicPlayer();
};#pragma once

#include <functional>
#include <map>

#include "utils/common.h"

using std::function;

typedef function<void(const s64)> ScheduledFunction;

namespace Scheduler {
map<s64, ScheduledFunction> schedule;

void run(const s64 ts) {
  for (auto ptr = schedule.begin(); ptr != schedule.end();) {
    if (ptr->first > ts) {
      return;
    }
    ptr->second(ts);
    ptr = schedule.erase(ptr);
  }
}
} // namespace Scheduler#pragma once

#include <algorithm>
#include <filesystem>
#include <regex>

#include "utils/common.h"

#include "SDL.h"
#include "SDL_image.h"

// Show each picture for 2.0 seconds
#define SLIDE_SHOW_PERIOD 10.0
#define SLIDE_SHOW_PATH "../art/images/slideshow/"
#define SLIDE_SHOW_TIME_COEFF (PI / SLIDE_SHOW_PERIOD)

using std::regex;

namespace SlideShow {
double curr_pos = 0.0;
size_t ptr = 0;
vector<SDL_Texture *> slides;
} // namespace SlideShow

vector<SDL_Texture *> get_slideshow_images(SDL_Renderer *rend) {
  vector<SDL_Texture *> result;
  regex str_expr("^(.*)\\.png$");
  for (const auto &entry :
       std::filesystem::directory_iterator(SLIDE_SHOW_PATH)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    string name = entry.path().filename();
    if (regex_match(name.begin(), name.end(), str_expr)) {
      result.push_back(IMG_LoadTexture(rend, entry.path().c_str()));
      SDL_SetTextureBlendMode(result.back(), SDL_BLENDMODE_BLEND);
    }
  }
  shuffle(result.begin(), result.end(), rng);
  return result;
}

void step_slideshow(SDL_Renderer *rend, const double dt) {
  using SlideShow::curr_pos;
  using SlideShow::ptr;
  using SlideShow::slides;
  assert(slides.size());

  curr_pos += dt * SLIDE_SHOW_TIME_COEFF;
  if (curr_pos > PI) {
    curr_pos -= PI;
    ptr++;
    ptr %= slides.size();
  }
  int alpha = sin(curr_pos) * 255.0;
  alpha = std::min(alpha, 255);
  alpha = std::max(alpha, 0);

  auto txt = slides.at(ptr);
  assert(SDL_SetTextureAlphaMod(txt, alpha) == 0);
  SDL_RenderCopy(rend, txt, NULL, NULL);
}

void reset_slideshow() {
  SlideShow::curr_pos = 0;
  SlideShow::ptr = 0;
}

void cleanup_slideshow() {
  for (auto txt : SlideShow::slides) {
    SDL_DestroyTexture(txt);
  }
}#include <sodium.h>

#include "user.h"
#include "utils/time.h"

#define USERS_FILE "../storage/users.json"

User::User() {
  this->name = "";
  this->register_time = 0;
  this->max_score_time = register_time;
  this->max_score = 0;
  this->pw_hash.resize(crypto_pwhash_STRBYTES, '\0');
}

User::User(const string &name, const string &pw) {
  this->name = name;
  this->register_time = get_utc_secs();
  this->max_score_time = register_time;
  this->max_score = 0;

  this->pw_hash.resize(crypto_pwhash_STRBYTES, '\0');
  if (crypto_pwhash_str(&pw_hash[0], pw.c_str(), pw.size(),
                        crypto_pwhash_OPSLIMIT_SENSITIVE,
                        crypto_pwhash_MEMLIMIT_SENSITIVE)) {
    cerr << "Out of memory for password hashing!" << endl;
    exit(1);
  }
}

User::User(const string &name, const time_t reg_time, const u64 max_score,
           const time_t max_score_time, const string &pw_hash)
    : name(name), register_time(reg_time), max_score(max_score),
      max_score_time(max_score_time), pw_hash(pw_hash) {}

void to_json(json &j, const User &u) {
  j = json{{"name", u.name},
           {"register_time", u.register_time},
           {"max_score", u.max_score},
           {"max_score_time", u.max_score_time},
           {"pw_hash", u.pw_hash},
           {"power_ups", u.power_ups},
           {"selected_map", u.selected_map},
           {"game_mode", u.game_mode},
           {"idx_pup", u.idx_pup},
           {"play_music", u.play_music},
           {"play_sfx", u.play_sfx}};
}

void from_json(const json &j, User &u) {
  j.at("name").get_to(u.name);
  j.at("register_time").get_to(u.register_time);
  j.at("max_score").get_to(u.max_score);
  j.at("max_score_time").get_to(u.max_score_time);
  j.at("pw_hash").get_to(u.pw_hash);
  j.at("power_ups").get_to(u.power_ups);
  j.at("selected_map").get_to(u.selected_map);
  j.at("game_mode").get_to(u.game_mode);
  j.at("idx_pup").get_to(u.idx_pup);
  j.at("play_music").get_to(u.play_music);
  j.at("play_sfx").get_to(u.play_sfx);
}

void User::load_settings() const {
  extern int selected_map, game_mode, idx_selected_power_up;
  extern bool play_music, play_sfx;
  selected_map = this->selected_map;
  game_mode = this->game_mode;
  idx_selected_power_up = this->idx_pup;
  play_music = this->play_music;
  play_sfx = this->play_sfx;
}

void User::save_settings() {
  extern int selected_map, game_mode, idx_selected_power_up;
  extern bool play_music, play_sfx;
  this->selected_map = selected_map;
  this->game_mode = game_mode;
  this->idx_pup = idx_selected_power_up;
  this->play_music = play_music;
  this->play_sfx = play_sfx;
}

bool verify_user(const User &u, const string &pw) {
  if (crypto_pwhash_str_verify(u.pw_hash.c_str(), pw.c_str(), pw.size())) {
    return false; // Password verification failed
  }
  return true;
}

UserMap get_user_list() {
  std::ifstream user_file(USERS_FILE);
  json j = json::parse(user_file);
  vector<User> tmp_result;
  j.get_to(tmp_result);
  UserMap result;
  for (auto &x : tmp_result) {
    result[x.name] = x;
  }
  return result;
}

void save_user_list(const UserMap &mp) {
  json j = json::array();
  for (const auto &[_, u] : mp) {
    j.push_back(u);
  }
  std::ofstream user_file(USERS_FILE);
  user_file << std::setw(4) << j << endl;
}#pragma once

#include <nlohmann/json.hpp>

#include "utils/common.h"
#include "utils/time.h"

using json = nlohmann::json;

typedef map<string, int> PowerUps;

class User {
private:
public:
  string name;
  time_t register_time;
  u64 max_score;
  time_t max_score_time;
  string pw_hash;

  int selected_map = 0;
  int game_mode = GameMode::FinishUp;
  int idx_pup = 0;

  bool play_music = true;
  bool play_sfx = true;

  PowerUps power_ups;

  User();
  User(const string &name, const string &pw); // Registers the user
  User(const string &name, const time_t reg_time, const u64 max_score,
       const time_t max_score_time, const string &pw_hash);

  void load_settings() const;
  void save_settings();
};

typedef map<string, User> UserMap;

void to_json(json &j, const User &u);
void from_json(const json &j, User &u);

bool verify_user(const User &u, const string &pw);

UserMap get_user_list();
void save_user_list(const UserMap &mp);#pragma once

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#define SCREEN_W 1280
#define SCREEN_H 800
#define HOLDER_DIM 112
//#define DEBUG true
#define NUM_BALLS 40
#define DEBUG false

#define PI ((double)3.141592653589793)
#define RAD_TO_DEG ((double)180.0 / PI)
#define DEG_TO_RAD (PI / (double)180.0)

#define RENDER_COLOR(rd, col)                                                  \
  SDL_SetRenderDrawColor((rd), (col).r, (col).g, (col).b, (col).a);

#define GAMEMODES "Finish Up\0Timed\0"
#define FINISHUP_MAXTIME 60'000
#define TIMED_MODE_MAXTIME (3 * FINISHUP_MAXTIME)

using std::cerr;
using std::cout;
using std::endl;
using std::map;
using std::pair;
using std::string;
using std::vector;

typedef int64_t s64;
typedef uint64_t u64;

typedef std::pair<int, int> pii;
typedef int64_t s64;

enum GameStatus {
  IN_GAME = 0b1,
  PLAYING = 0b11,
  PAUSED = 0b01,
  NOT_STARTED = 0b0,
  LOST = 0b0100,
  WON = 0b1100,
};

enum GameMode { FinishUp, Timed };

#define PUP_LUCK "Good Luck"

const vector<string> POWER_UPS = {PUP_LUCK};

#include "geometry.hpp"
#include "phase.hpp"
#include "random_gen.hpp"
#include "vec_utils.hpp"#pragma once

#define INIT_SUCCESS(t) ((t).success)#pragma once

#include <utility>

using std::pair;

template <typename T, typename S>
double square_euclid_dist(const pair<T, T> a, const pair<S, S> b) {
  const auto diff_x = a.first - b.first;
  const auto diff_y = a.second - b.second;

  return (diff_x * diff_x) + (diff_y * diff_y);
}#pragma once

#include "common.h"

typedef pair<double, double> pdd;

inline double vector_phase(const pdd vec) {
  double res = 0;
  double div_res = 0;
  if (abs(vec.first) < 1e-10) {
    res = vec.second > 0 ? PI / 2 : -PI / 2;
    res = vec.first < 0 ? -res : res;
  } else {
    div_res = vec.second / vec.first;
    res = atan(div_res);
    if (vec.first < 0) {
      res += PI;
    }
  }
  return res;
}#pragma once

#include <random>

#define UID(uid_name, min, max)                                                \
  std::uniform_int_distribution<int> uid_name(min, max)
#define URD(urd_name, min, max)                                                \
  std::uniform_real_distribution<double> urd_name(min, max)

static std::random_device rand_dev;
static std::mt19937 rng(rand_dev());#include "time.h"

time_t get_utc_secs() {
  return time(NULL); // Number of seconds since the epoch (UTC)
}

// Gets current time if tval == 0
string format_utc_time(time_t tval) {
  if (tval == 0) {
    tval = time(NULL);
  }
  struct tm *tm_res = (tm *)malloc(sizeof(tm));
  string result = "";
  result.resize(30, '\0');
  localtime_r(&tval, tm_res);
  strftime(&result[0], 28, "%F %T", tm_res);
  auto nb = result.begin() + result.find('\0');
  result.erase(nb, result.end());
  free(tm_res);
  return result;
}#pragma once

#include <ctime>
#include <string>

using std::string;

time_t get_utc_secs();

// Gets current time if tval == 0
string format_utc_time(time_t tval);#pragma once

#include <array>
#include <utility>
#include <vector>

using std::array;
using std::pair;
using std::vector;

template <typename T>
inline pair<T, T> operator+(const pair<T, T> a, const pair<T, T> b) {
  return {a.first + b.first, a.second + b.second};
}

template <typename T>
inline pair<T, T> &operator+=(pair<T, T> &a, const pair<T, T> b) {
  a.first += b.first;
  a.second += b.second;
  return a;
}

template <typename T>
inline pair<T, T> operator-(const pair<T, T> a, const pair<T, T> b) {
  return {a.first - b.first, a.second - b.second};
}

template <typename T>
inline pair<T, T> operator/(const pair<T, T> a, const T s) {
  return {a.first / s, a.second / s};
}

template <typename T>
inline pair<T, T> operator*(const pair<T, T> a, const T s) {
  return {a.first * s, a.second * s};
}

template <typename T> inline T dot(const pair<T, T> a, const pair<T, T> b) {
  return (a.first * b.first) + (a.second * b.second);
}