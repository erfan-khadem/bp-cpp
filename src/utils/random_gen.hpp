#include <random>

#define UID(uid_name, min, max) std::uniform_int_distribution<std::mt19937::result_type> (uid_name)(min, max)

std::random_device rand_dev;
std::mt19937 rng(rand_dev());