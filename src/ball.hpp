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
      }
      if(special_power(rng) < 0.1) {
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
};