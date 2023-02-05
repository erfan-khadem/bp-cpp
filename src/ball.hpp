#pragma once

#include <map>
#include <utility>

#include "utils/common.h"

#include "SDL.h"
#include "SDL_image.h"

using std::map;

#define BALL_SIZE 64
#define BALL_PNG_SIZE 1280
// This is the size of the circle drawn on the screen
#define REAL_BALL_SIZE 50
#define GLOBAL_SPEED_FACTOR 1.0
#define SHOOTING_BALL_SPEED_COEFF (3000.0 * GLOBAL_SPEED_FACTOR)

#define MULTI_COLOR (1 << 16)
#define COLOR_MSK 0xffff

static const double sq_2x_radius = BALL_SIZE * BALL_SIZE;

typedef const vector<pii>* PointerType;

map<PointerType, s64> address_users;
map<PointerType, vector<int>> calc_prev;

SDL_Texture* greyball_txt = nullptr;
SDL_Texture* freeze_txt = nullptr;
SDL_Texture* pause_txt = nullptr;
SDL_Texture* reverse_txt = nullptr;
SDL_Texture* colordots_txt = nullptr;

class Ball{
public:
    double loc_index;
    int ball_color;
    bool should_render;
    bool freeze = false;
    bool pause = false;
    bool reverse = false;

    SDL_Rect position;
    SDL_Rect draw_position;

    SDL_Texture* ball_txt;
    SDL_Texture* render_txt;
    SDL_Renderer* renderer;

    const vector<pii> *locations;

    char buf[100];

    // _locs can be NULL
    Ball(const int _ball_color, SDL_Texture* cache_txt, SDL_Renderer* rend, const vector<pii>* const _locs){
        loc_index = 0.0;
        locations = _locs;
        ball_color = _ball_color;
        should_render = true;
        renderer = rend;
        if(cache_txt == nullptr) {
            snprintf(buf, 90, "../art/images/ball%02d.png", ball_color);
            ball_txt = IMG_LoadTexture(renderer, buf);
        } else {
            ball_txt = cache_txt;
        }
        position.h = 1.25 * BALL_SIZE;
        position.w = 1.25 * BALL_SIZE;
        if(_locs == nullptr){
            position.x = SCREEN_W >> 1;
            position.y = SCREEN_H >> 1;
        } else {
            assert(locations->size());
            position.x = locations->at(0).first;
            position.y = locations->at(0).second;
            if(address_users.find(locations) == address_users.end()) {
                address_users[locations] = 0;
            }
            address_users[locations]++;
        }
        // Should not be a center ball
        URD(special_power, 0, 1);
        if(locations != nullptr) {
            if(special_power(rng) < 0.1) {
                double my_rnd = special_power(rng);
                if(my_rnd < 0.3) {
                    reverse = true;
                } else if(my_rnd < 0.6){
                    pause = true;
                } else {
                    freeze = true;
                }
            }
        } else {
            // Any color effect is for center ball only
            if(special_power(rng) < 0.1) {
                ball_color = MULTI_COLOR;
                ball_txt = greyball_txt;
            }
        }
        render_txt = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET, 1280, 1280);
        assert(render_txt != nullptr);
        assert(SDL_SetRenderTarget(rend, render_txt) == 0);
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
        SDL_RenderClear(rend);
        SDL_RenderCopy(rend, ball_txt, NULL, NULL);
        auto rect = SDL_Rect{.x=384, .y=384, .w=512, .h=512};
        if(ball_color & MULTI_COLOR) {
            auto rect2 = SDL_Rect{.x=384, .y=417, .w=512, .h=445};
            SDL_RenderCopy(rend, colordots_txt, NULL, &rect2);
        }
        if(pause) {
            SDL_RenderCopy(rend, pause_txt, NULL, &rect);
        } else if(freeze) {
            SDL_RenderCopy(rend, freeze_txt, NULL, &rect);
        } else if(reverse) {
            SDL_RenderCopy(rend, reverse_txt, NULL, &rect);
        }
        SDL_SetRenderTarget(rend, NULL);
        SDL_SetTextureBlendMode(render_txt, SDL_BLENDMODE_BLEND);
    }
    
    ~Ball() {
        // DO NOT destroy ball_texture!
        if(locations != nullptr && (--address_users[locations]) == 0){
            calc_prev.erase(calc_prev.find(locations));
        }
        SDL_DestroyTexture(render_txt);
    }

    pii get_center() const {
        return pii{
            position.x + (BALL_SIZE >> 1),
            position.y + (BALL_SIZE >> 1)
        };
    }

    double get_previous_ball_pos() {
        const int cpos = loc_index;
        if(!should_render) {
            return -1;
        }
        auto &vec = calc_prev[locations];
        if(int(vec.size()) > cpos && vec[cpos] != -2) {
            return vec[cpos] + loc_index - floor(loc_index);
        }
        if(cpos >= (int)vec.size())
            vec.resize(cpos + 1, -2);

        const auto pos = pii{position.x, position.y};

        for(int i = cpos - 1; i >= 0; i--) {
            if(square_euclid_dist(locations->at(i), pos) >= sq_2x_radius){
                vec[cpos] = cpos > 0 ? std::max(i, vec[cpos-1]) : i;
                return vec[cpos] + loc_index - floor(loc_index);
            }
        }
        vec[cpos] = -1;
        return -1;
    }

    void move_to_location(const double pos) {
        assert(locations != nullptr);
        if(pos < 0 || pos >= int(locations->size())) {
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
        if(should_render){
            SDL_RenderCopy(renderer , render_txt, NULL , &draw_position);
        }
    }

    bool collides(const Ball &other) const {
        int d = square_euclid_dist(pii{position.x, position.y},
            pii{other.position.x, other.position.y});
        if(d <= (REAL_BALL_SIZE * REAL_BALL_SIZE)) {
            return true;
        }
        return false;
    }

    bool same_color(const Ball &other) const {
        bool result = false;
        result |= (other.ball_color | ball_color) & MULTI_COLOR;
        result |= (other.ball_color & COLOR_MSK) == (ball_color & COLOR_MSK);
        return result;
    }
};

class ShootingBall: public Ball {
private:
public:
    bool is_moving = false;
    pdd direction = {0.0, 0.0};
    pdd fine_position = {SCREEN_W >> 1, SCREEN_H >> 1};

    using Ball::Ball;

    void step_dt(const double dt, bool set_int_pos=true) {
        fine_position += direction * (dt * SHOOTING_BALL_SPEED_COEFF);
        if(set_int_pos){
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

    Ball& to_normal_ball(const vector<pii>* const _locs, const int _b_col, SDL_Texture* col_txt) {
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
        if(address_users.find(locations) == address_users.end()) {
            address_users[locations] = 0;
        }
        address_users[locations]++;
        return *this;
    }
    Ball& to_normal_ball(const vector<pii>* const _locs) {
        move_to_fine_position();
        assert(locations == nullptr);
        assert(_locs != nullptr);
        locations = _locs;
        if(address_users.find(locations) == address_users.end()) {
            address_users[locations] = 0;
        }
        address_users[locations]++;
        return *this;
    }
};