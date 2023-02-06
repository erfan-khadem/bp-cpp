#pragma once

#include <random>

#define UID(uid_name, min, max)                                                \
  std::uniform_int_distribution<int> uid_name(min, max)
#define URD(urd_name, min, max)                                                \
  std::uniform_real_distribution<double> urd_name(min, max)

static std::random_device rand_dev;
static std::mt19937 rng(rand_dev());