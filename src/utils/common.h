#pragma once

#include <cmath>
#include <cassert>
#include <string>
#include <vector>
#include <utility>
#include <iostream>

#define SCREEN_W 1280
#define SCREEN_H 800

#define RAD_TO_DEG ((double)180.0 / (double)3.141592653589793)
#define DEG_TO_RAD ((double)3.141592653589793 / (double)180.0)
#define PI ((double)3.141592653589793)

using std::cerr;
using std::cout;
using std::endl;
using std::vector;
using std::pair;
using std::string;

typedef std::pair<int, int> pii;
typedef int64_t s64;

#include "vec_utils.hpp"
#include "phase.hpp"
#include "geometry.hpp"
#include "random_gen.hpp"