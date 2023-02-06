#pragma once

#include <functional>
#include <map>

#include "utils/common.h"

using std::function;

typedef function<void(const s64)> ScheduledFunction;

namespace Scheduler {
map<s64, ScheduledFunction> schedule;

void run(const s64 ts) {
  for (auto ptr = schedule.begin(); ptr != schedule.end();) {
    if (ptr->first > ts) {
      return;
    }
    ptr->second(ts);
    ptr = schedule.erase(ptr);
  }
}
} // namespace Scheduler