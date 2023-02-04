#include <sodium.h>

#include "user.h"
#include "utils/time.h"

#define USERS_FILE "../storage/users.json"

User::User(){
    this->name = "";
    this->register_time = 0;
    this->max_score_time = register_time;
    this->max_score = 0;
    this->pw_hash.resize(crypto_pwhash_STRBYTES, '\0');
}

User::User(const string &name, const string &pw) {
    this->name = name;
    this->register_time = get_utc_secs();
    this->max_score_time = register_time;
    this->max_score = 0;

    this->pw_hash.resize(crypto_pwhash_STRBYTES, '\0');
    if (crypto_pwhash_str(
            &pw_hash[0],
            pw.c_str(), pw.size(),
            crypto_pwhash_OPSLIMIT_SENSITIVE, crypto_pwhash_MEMLIMIT_SENSITIVE
    )) {
        cerr << "Out of memory for password hashing!" << endl;
        exit(1);
    }
}

User::User(const string &name,
         const time_t reg_time,
         const u64 max_score,
         const time_t max_score_time,
         const string &pw_hash):
    name(name), register_time(reg_time), max_score(max_score),
    max_score_time(max_score_time), pw_hash(pw_hash){}

void to_json(json &j, const User &u){
    j = json{
        {"name", u.name},
        {"register_time", u.register_time},
        {"max_score", u.max_score},
        {"max_score_time", u.max_score_time},
        {"pw_hash", u.pw_hash},
        {"power_ups", u.power_ups}
    };
}

void from_json(const json &j, User &u){
    j.at("name").get_to(u.name);
    j.at("register_time").get_to(u.register_time);
    j.at("max_score").get_to(u.max_score);
    j.at("max_score_time").get_to(u.max_score_time);
    j.at("pw_hash").get_to(u.pw_hash);
    j.at("power_ups").get_to(u.power_ups);
}

bool verify_user(const User &u, const string &pw) {
    if (crypto_pwhash_str_verify(
        u.pw_hash.c_str(), pw.c_str(), pw.size()
    )) {
        return false; // Password verification failed
    }
    return true;
}

UserMap get_user_list() {
    std::ifstream user_file(USERS_FILE);
    json j = json::parse(user_file);
    vector<User> tmp_result;
    j.get_to(tmp_result);
    UserMap result;
    for(auto &x:tmp_result) {
        result[x.name] = x;
    }
    return result;
}

void save_user_list(const UserMap &mp){
    json j = json::array();
    for(const auto &[_, u]: mp){
        j.push_back(u);
    }
    std::ofstream user_file(USERS_FILE);
    user_file << std::setw(4) << j << endl;
}