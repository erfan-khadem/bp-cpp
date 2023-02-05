#pragma once

#include <map>
#include <cmath>
#include <cassert>
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <iostream>

#define SCREEN_W 1280
#define SCREEN_H 800
#define HOLDER_DIM 112

#define PI ((double)3.141592653589793)
#define RAD_TO_DEG ((double)180.0 / PI)
#define DEG_TO_RAD (PI / (double)180.0)

#define RENDER_COLOR(rd, col) SDL_SetRenderDrawColor( \
        (rd), (col).r, (col).g, (col).b, (col).a);

using std::cerr;
using std::cout;
using std::endl;
using std::vector;
using std::pair;
using std::string;
using std::map;
using std::endl;

typedef int64_t s64;
typedef uint64_t u64;

typedef std::pair<int, int> pii;
typedef int64_t s64;

enum GameStatus{
    IN_GAME = 0b1,
    PLAYING = 0b11,
    PAUSED = 0b01,
    NOT_STARTED = 0b0,
    LOST = 0b0100,
    WON = 0b1100,
};

#define PUP_BOMB "Bomb"
#define PUP_FIRE "Fire"

const vector<string> POWER_UPS = {
    PUP_BOMB, PUP_FIRE
};

#include "vec_utils.hpp"
#include "phase.hpp"
#include "geometry.hpp"
#include "random_gen.hpp"