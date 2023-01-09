#include "time.h"

time_t get_utc_secs()
{
    return time(NULL); // Number of seconds since the epoch (UTC)
}

// Gets current time if tval == 0
string format_utc_time(time_t tval)
{
    if (tval == 0) {
        tval = time(NULL);
    }
    struct tm* tm_res = (tm*) malloc(sizeof(tm));
    string result = "";
    result.resize(30, '\0');
    localtime_r(&tval, tm_res);
    strftime(&result[0], 28, "%F %T", tm_res);
    auto nb = result.begin() + result.find('\0');
    result.erase(nb, result.end());
    return result;
}