#include "user.h"
#include "utils/time.hpp"

User::User(const string &name) {
    this->name = name;
    this->register_time = get_utc_secs();
    this->max_score_time = register_time;
    this->max_score = 0;
}

User::User(const string &name,
         const time_t reg_time,
         const u64 max_score,
         const time_t max_score_time):
    name(name), register_time(reg_time), max_score(max_score), max_score_time(max_score_time){}

void to_json(json &j, const User &u){
    j = json{
        {"name", u.name},
        {"register_time", u.register_time},
        {"max_score", u.max_score},
        {"max_score_time", u.max_score_time}
    };
}

void from_json(const json &j, User &u){
    j.at("name").get_to(u.name);
    j.at("register_time").get_to(u.register_time);
    j.at("max_score").get_to(u.max_score);
    j.at("max_score_time").get_to(u.max_score_time);
}