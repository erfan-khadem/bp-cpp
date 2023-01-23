#pragma once

#include <utility>

using std::pair;

template<typename T, typename S>
double square_euclid_dist(const pair<T, T> a, const pair<S, S> b) {
    const auto diff_x = a.first - b.first;
    const auto diff_y = a.second - b.second;

    return (diff_x * diff_x) + (diff_y * diff_y);
}