#pragma once

#include <vector>
#include <utility>
#include <array>

using std::vector;
using std::pair;
using std::array;

template<typename T>
inline pair<T, T> operator+(const pair<T, T> a, const pair<T, T> b) {
    return {a.first + b.first, a.second + b.second};
}

template<typename T>
inline pair<T, T> operator-(const pair<T, T> a, const pair<T, T> b) {
    return {a.first - b.first, a.second - b.second};
}
