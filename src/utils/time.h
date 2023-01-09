#pragma once

#include <ctime>
#include <string>

using std::string;

time_t get_utc_secs();

// Gets current time if tval == 0
string format_utc_time(time_t tval);