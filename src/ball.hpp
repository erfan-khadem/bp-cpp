#pragma once

#include <map>

#include "utils/common.h"

#include "SDL.h"
#include "SDL_image.h"

using std::map;

#define BALL_SIZE 64

static const double sq_2x_radius = BALL_SIZE * BALL_SIZE;

typedef const vector<pii>* PointerType;

map<PointerType, s64> address_users;
map<PointerType, vector<int>> calc_prev;

class Ball{
public:
    int loc_index;
    int ball_color;
    bool should_render;

    SDL_Rect position;
    SDL_Rect draw_position;

    SDL_Texture* ball_txt;
    SDL_Renderer* renderer;

    const vector<pii> *locations;

    char buf[100];

    Ball(const int _ball_color, SDL_Texture* cache_txt, SDL_Renderer* rend, const vector<pii>* const _locs){
        loc_index = 0;
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
        position.x = locations->at(loc_index).first;
        position.y = locations->at(loc_index).second;

        if(address_users.find(locations) == address_users.end()) {
            address_users[locations] = 0;
        }

        address_users[locations]++;
    }
    
    ~Ball() {
        // DO NOT destroy ball_texture!
        if((--address_users[locations]) == 0){
            calc_prev.erase(calc_prev.find(locations));
        }
    }

    pii get_center() const {
        return pii{
            position.x + (BALL_SIZE >> 1),
            position.y + (BALL_SIZE >> 1)
        };
    }

    int get_previous_ball_pos() {
        const int cpos = loc_index;
        if(!should_render) {
            return -1;
        }
        auto &vec = calc_prev[locations];
        if(int(vec.size()) > cpos && vec[cpos] != -2) {
            return vec[cpos];
        }
        if(cpos >= (int)vec.size())
            vec.resize(cpos + 1, -2);

        const auto pos = pii{position.x, position.y};

        for(int i = cpos - 1; i >= 0; i--) {
            if(square_euclid_dist(locations->at(i), pos) >= sq_2x_radius){
                vec[cpos] = i;
                return i;
            }
        }
        vec[cpos] = -1;
        return -1;
    }

    void move_to_location(const int pos) {
        if(pos < 0 || pos >= int(locations->size())) {
            position.x = -1000;
            position.y = -1000;
            should_render = false;
            return;
        }
        loc_index = pos;
        should_render = true;
        position.x = locations->at(pos).first;
        position.y = locations->at(pos).second;
    }


    void draw_ball() {
        draw_position = position;
        draw_position.x -= position.h >> 1;
        draw_position.y -= position.h >> 1;
        if(should_render){
            SDL_RenderCopy(renderer , ball_txt, NULL , &draw_position);
        }
    }
};