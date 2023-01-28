#pragma once

#include "utils/common.h"
#include "SDL.h"
#include "SDL_image.h"

#define BALL_SIZE 64

class Shelik{
public:

    int ball_color;
    int diff_cntrloc;
    bool should_render = true;

    SDL_Rect position;
    SDL_Rect draw_position;

    SDL_Surface* surface;
    SDL_Texture* ball_txt;
    SDL_Renderer* renderer;

    char buf[100];

    Shelik(const int _ball_color, SDL_Texture* cache_txt, SDL_Renderer* rend){
        ball_color = _ball_color;
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
        position.x = SCREEN_W >> 1;
        position.y = SCREEN_H >> 1;
    }


    void move_to_location(int pos,const int Dy,const int Dx , int diff) {
        pos -= diff;
        position.x = pos*Dx/200 + SCREEN_W >> 1;
        position.y = int(pos*Dy/200) + SCREEN_H >> 1;
        if(position.x>1000 || position.x < 0 || position.y>500 || position.y<0){
            should_render = true;
        }
    }
    void draw_fired_ball() {
        draw_position = position;
        draw_position.x -= position.h >> 1;
        draw_position.y -= position.h >> 1;
        if(should_render){
            SDL_RenderCopy(renderer , ball_txt, NULL , &draw_position);
        }
    }
    ~Shelik(){
        // Surface and ball_txt shall not be free'd here!
    }
};