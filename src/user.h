#pragma once

#include <nlohmann/json.hpp>

#include "utils/common.h"

using json = nlohmann::json;

class User
{
private:
public:
    string name;
    time_t register_time;
    time_t max_score_time;
    u64 max_score;

    User(const string &name); // Registers the user
    User(const string &name,
         const time_t reg_time,
         const u64 max_score,
         const time_t max_score_time);
};

void to_json(json &j, const User &u);
void from_json(const json &j, User &u);