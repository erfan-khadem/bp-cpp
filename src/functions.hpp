#pragma once

#include "SDL.h"
#include "SDL_image.h"

#include "utils/common.h"

void reset_game_status() {
  if (schedule_game_status == false) {
    Scheduler::schedule.emplace(curr_time + 5'000, [&](const s64 _) {
      (void)_;
      schedule_game_status = false;
      curr_game_status = GameStatus::NOT_STARTED;
      game_speed_factor = 1.0;
      user_score = 0;
      good_luck = false;
    });
    for(auto &b: balls){
      delete b;
    }
    for(auto &b: shot_balls) {
      delete b;
    }
    balls.clear();
    shot_balls.clear();
    if (center_ball != nullptr) {
      delete center_ball;
      center_ball = nullptr;
    }
    if (curr_user != nullptr) {
      if(curr_user->max_score < user_score) {
        curr_user->max_score_time = get_utc_secs();
        curr_user->max_score = user_score;
      }
    }
    schedule_game_status = true;
  }
}
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
  int curr_size = 0;
  user_power_ups += "None\0";
  for (const auto &[name, cnt] : curr_user->power_ups) {
    if (cnt <= 0) {
      continue;
    }
    curr_size = name.size();
    user_power_ups += name;
    assert(curr_size < 32);
    while (curr_size < 32) {
      user_power_ups.push_back(' ');
    }
    user_power_ups += to_string(cnt);
    user_power_ups += '\0';
  }
  user_power_ups += '\0';
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
};

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
    curr_user->load_settings();
    remake_user_power_ups();
    idx_selected_power_up = 0;
    selected_power_up = "";
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
      }
      selected_power_up = ptr->first;
      curr_user->power_ups[selected_power_up]--;
    } else {
      selected_power_up = "None";
    }
    assert(curr_map != nullptr);
    target_num_balls = curr_map->locations.size() / 125;
    if (game_mode == GameMode::FinishUp) {
      generate_game_balls(target_num_balls);
    } else if (game_mode == GameMode::Timed) {
      generate_game_balls(200);
    }
    game_start_time = curr_time;
    curr_game_status = GameStatus::PLAYING;
    prev_ball_dist = curr_map->prev_ball_dist;
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
    step = GLOBAL_SPEED_FACTOR * dt * 100 * (slowdown_effect ? 0.5 : 1.0);
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
        if((*sptr)->is_fire) {
          music_player->play_sound("fire.wav");
          (*sptr)->is_fire = false;
          (*sptr)->create_texture();
          auto tmp = balls.begin();
          while(tmp != balls.end()) {
            if((*tmp)->should_render == false && (*tmp)->loc_index < 10) {
              break;
            }
            (*tmp)->unfreeze();
            tmp++;
          }
        } else if((*sptr)->is_bomb) {
          music_player->play_sound("bomb.wav");
          auto fptr = ptr;
          for(int i = 0; i < 3 && fptr != balls.begin(); i++) {
            fptr--;
            (*fptr)->ball_color = 0;
            (*fptr)->frozen = false;
            (*fptr)->uncolored = false;
          }
          fptr = ptr;
          for(int i = 0; i < 3 && fptr != balls.end(); i++, fptr++) {
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
      if((*ptr)->should_render == false && (*ptr)->loc_index < 10) {
        break;
      }
    }
  }
  in_game_ball_colors.clear();
  ptr = balls.begin();
  while(ptr != balls.end()){
    auto b = *ptr;
    if (b->loc_index > 100 && b->should_render == false &&
        game_mode == GameMode::FinishUp) {
      return GameStatus::LOST;
    } else if(b->loc_index > 100 && b->should_render == false) {
      delete b;
      ptr = balls.erase(ptr);
      continue;
    }
    in_game_ball_colors.insert(b->ball_color & COLOR_MSK);
    b->draw_ball();
    ptr++;
  }
  return GameStatus::PLAYING;
}