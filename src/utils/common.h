#pragma once

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
#include "vec_utils.hpp"