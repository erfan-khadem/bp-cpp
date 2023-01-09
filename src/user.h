#pragma once

#include <nlohmann/json.hpp>

#include "utils/time.h"
#include "utils/common.h"

using json = nlohmann::json;

class User
{
private:
public:
    string name;
    time_t register_time;
    u64 max_score;
    time_t max_score_time;
    string pw_hash;

    User();
    User(const string &name, const string &pw); // Registers the user
    User(const string &name,
         const time_t reg_time,
         const u64 max_score,
         const time_t max_score_time,
         const string &pw_hash);
};

typedef map<string, User> UserMap;

void to_json(json &j, const User &u);
void from_json(const json &j, User &u);

bool verify_user(const User &u, const string &pw);

UserMap get_user_list();
void save_user_list(const UserMap &mp);