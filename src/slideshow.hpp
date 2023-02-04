#pragma once

#include <regex>
#include <algorithm>
#include <filesystem>

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
    vector<SDL_Texture*> slides;
}

vector<SDL_Texture*> get_slideshow_images(SDL_Renderer *rend) {
    vector<SDL_Texture*> result;
    regex str_expr ("^(.*)\\.png$");
    for(const auto &entry: std::filesystem::directory_iterator(SLIDE_SHOW_PATH)) {
        if(!entry.is_regular_file()) {
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
    if(curr_pos > PI) {
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
    for(auto txt:SlideShow::slides) {
        SDL_DestroyTexture(txt);
    }
}