#pragma once

#include "utils/common.h"
#include "utils/phase.hpp"
#include "SDL.h"
#include "SDL_image.h"

#define BALL_SIZE 64

static const double sq_2x_radius = BALL_SIZE * BALL_SIZE;

class Ball{
public:
    int loc_index;
    int ball_color;
    bool should_render;

    SDL_Rect position;
    SDL_Rect draw_position;

    SDL_Surface* surface;
    SDL_Texture* ball_txt;
    SDL_Renderer* renderer;

    vector<pii> *locations;

    char buf[100];

    Ball(const int _ball_color, SDL_Texture* cache_txt, SDL_Renderer* rend, vector<pii>* const _locs){
        loc_index = 0;
        locations = _locs;
        ball_color = _ball_color;
        should_render = true;
        renderer = rend;
        if(cache_txt == nullptr) {
            snprintf(buf, 90, "../art/images/ball%02d.png", ball_color);
            surface = IMG_Load(buf);
            ball_txt = SDL_CreateTextureFromSurface(renderer, surface);
        } else {
            ball_txt = cache_txt;
        }
        position.h = 1.25 * BALL_SIZE;
        position.w = 1.25 * BALL_SIZE;
        position.x = locations->at(loc_index).first;
        position.y = locations->at(loc_index).second;
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

        const auto pos = pii{position.x, position.y};

        for(int i = cpos - 1; i >= 0; i--) {
            if(square_euclid_dist(locations->at(i), pos) >= sq_2x_radius){
                return i;
            }
        }
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
    int iscolided(SDL_Rect fired_ball_rect){
        pair<double , double> a;
        pair<double , double> b; 
        if((fired_ball_rect.x - position.x)*(fired_ball_rect.x - position.x) + (fired_ball_rect.y - position.y)*(fired_ball_rect.y - position.y) < sq_2x_radius){
            a = {fired_ball_rect.x - SCREEN_W>>1 , fired_ball_rect.y - SCREEN_H>>1};
            b = {position.x - SCREEN_W>>1 , position.y - SCREEN_H>>1};
            if(vector_phase(b)<vector_phase(a)){
                return 1;
            }
            else{
                return -1;
            }
        }
        else{
            return 0;
        }
    }
    void draw_ball() {
        draw_position = position;
        draw_position.x -= position.h >> 1;
        draw_position.y -= position.h >> 1;
        if(should_render){
            SDL_RenderCopy(renderer , ball_txt, NULL , &draw_position);
        }
    }

    ~Ball(){
        // Surface and ball_txt shall not be free'd here!
    }
};