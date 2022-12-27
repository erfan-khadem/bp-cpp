#include "rng.h"
#include <cstdlib>
#include <ctime>
int make_random(const int lower, const int upper){
    srand(time(NULL));
    return (rand()%(upper-lower+1))+lower;
}