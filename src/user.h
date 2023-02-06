#pragma once

#include <nlohmann/json.hpp>

#include "utils/common.h"
#include "utils/time.h"

using json = nlohmann::json;

typedef map<string, int> PowerUps;

class User {
private:
public:
  string name;
  time_t register_time;
  u64 max_score;
  time_t max_score_time;
  string pw_hash;

  int selected_map = 0;
  int game_mode = GameMode::FinishUp;
  int idx_pup = 0;

  bool play_music = true;
  bool play_sfx = true;

  PowerUps power_ups;

  User();
  User(const string &name, const string &pw); // Registers the user
  User(const string &name, const time_t reg_time, const u64 max_score,
       const time_t max_score_time, const string &pw_hash);

  void load_settings() const;
  void save_settings();
};

typedef map<string, User> UserMap;

void to_json(json &j, const User &u);
void from_json(const json &j, User &u);

bool verify_user(const User &u, const string &pw);

UserMap get_user_list();
void save_user_list(const UserMap &mp);