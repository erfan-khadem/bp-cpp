#pragma once

#include "common.h"

typedef pair<double, double> pdd;

inline double vector_phase(const pdd vec) {
  double res = 0;
  double div_res = 0;
  if (abs(vec.first) < 1e-10) {
    res = vec.second > 0 ? PI / 2 : -PI / 2;
    res = vec.first < 0 ? -res : res;
  } else {
    div_res = vec.second / vec.first;
    res = atan(div_res);
    if (vec.first < 0) {
      res += PI;
    }
  }
  return res;
}