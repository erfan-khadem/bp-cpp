#pragma once

#include "imgui.h"
#include "utils/common.h"

#include "user.h"

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error ImGUI requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

SDL_Texture *motivational_txt = nullptr;

void AlignForWidth(float width, float alignment = 0.5f) {
  float avail = ImGui::GetContentRegionAvail().x;
  float off = (avail - width) * alignment;
  if (off > 0.0f)
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}

User *login_or_register(UserMap &umap) {
  static bool should_register = false;
  static bool should_show_password = false;
  static ImGuiInputTextFlags password_flags = ImGuiInputTextFlags_Password;

  static string username;
  static string password;
  static string retype_pass;

  bool passwords_match = false;

  username.resize(32, 0x00);
  password.resize(32, 0x00);
  retype_pass.resize(32, 0x00);

  ImGui::Begin("Login or Register", NULL, 0);

  ImGui::InputText("User Name", &username[0], username.size());
  ImGui::InputText("Password", &password[0], password.size(),
                   should_show_password ? 0 : password_flags);
  ImGui::SameLine();
  ImGui::Checkbox("Show Password", &should_show_password);

  ImGui::Checkbox("Register", &should_register);

  if (should_register && !should_show_password) {
    ImGui::InputText("Retype Password", &retype_pass[0], retype_pass.size(),
                     should_show_password ? 0 : password_flags);
  }

  float button_width = 0;
  button_width += 100;
  button_width += should_register ? 200 : 0;

  AlignForWidth(button_width);

  if (ImGui::Button("Login", ImVec2(100, 30))) {
    auto usr = umap.find(username);
    if (usr == umap.end()) {
      float width = ImGui::CalcTextSize("User Not Found!").x;
      AlignForWidth(width);
      ImGui::TextColored({1.0, 0.2, 0.2, 1.0}, "User Not Found");
      goto show_registration;
    }
    auto &user = usr->second;
    if (verify_user(user, password)) {
      ImGui::End();
      return &user;
    }
  }
show_registration:
  if (should_register) {
    passwords_match = should_show_password ||
                      (strcmp(password.c_str(), retype_pass.c_str()) == 0);
    ImGui::SameLine();
    if (!passwords_match) {
      ImGui::BeginDisabled();
    }
    if (ImGui::Button("Register User", ImVec2(200, 30))) {
      User *user = nullptr;
      if (umap.find(username) != umap.end()) {
        float width = ImGui::CalcTextSize("User Already Exists!").x;
        AlignForWidth(width);
        ImGui::TextColored({1.0, 0.2, 0.2, 1.0}, "User Already Exists!");
        goto end_draw;
      }
      user = new User(username, password);
      umap[username] = *user;
      ImGui::End();
      return user;
    }
    if (!passwords_match) {
      ImGui::EndDisabled();
      float width = ImGui::CalcTextSize("Passwords do not match!").x;
      AlignForWidth(width);
      ImGui::TextColored({1.0, 0.2, 0.2, 1.0}, "Passwords do not match!");
    }
  }
end_draw:
  ImGui::End();

  return NULL;
}

void user_lost() {
  ImGui::Begin("You Lost", NULL, ImGuiWindowFlags_NoTitleBar);
  float width = ImGui::CalcTextSize("You Lost :(").x;
  AlignForWidth(width);
  ImGui::TextColored(ImVec4(0.87, 0.10, 0.14, 1.0), "You Lost :(");
  AlignForWidth(SCREEN_W / 3);
  ImGui::Image((void *)motivational_txt, ImVec2(SCREEN_W / 3, SCREEN_W / 3));
  ImGui::End();
}

void user_won(u64 user_score, vector<Map *> &maps, Map *curr_map) {
  static char buf[100];
  static u64 prev_score = UINT64_MAX;
  ImGui::Begin("You Won!", NULL, ImGuiWindowFlags_NoTitleBar);
  float width = ImGui::CalcTextSize("You Won!").x;
  AlignForWidth(width);
  ImGui::TextColored(ImVec4(0.18, 0.76, 0.49, 1.0), "You Won!");
  if(user_score != prev_score){
    snprintf(buf, 90, "Score: %lu", user_score);
    prev_score = user_score;
  }
  width = ImGui::CalcTextSize(buf).x;
  AlignForWidth(width);
  ImGui::TextColored(ImVec4(0.96, 0.76, 0.07, 1.0), buf);
  if(user_score >= 1000) {
    width = ImGui::CalcTextSize("New Power UP: GoodLuck").x;
    AlignForWidth(width);
    ImGui::TextColored(ImVec4(0.96, 0.76, 0.07, 1.0), "New Power UP: GoodLuck");
  }
  width = ImGui::CalcTextSize("Try Other maps:").x;
  AlignForWidth(width);
  ImGui::Text("Try Other maps:");
  AlignForWidth((2 * SCREEN_W) / 3);
  int cnt = 0;
  for(int i = 0; i < (int)maps.size() && cnt != 2; i++) {
    if(maps.at(i) == curr_map) {
      continue;
    }
    if(cnt >= 1) {
      ImGui::SameLine();
    }
    cnt++;
    ImGui::Image((void*)maps.at(i)->blended_txt, ImVec2(SCREEN_W / 3, SCREEN_H / 3));
  }
  ImGui::End();
}

void draw_music_play_status(bool &play_music, bool &play_sfx,
                            MusicPlayer *music_player) {
  if (ImGui::Checkbox("Play Music", &play_music)) {
    if (play_music) {
      music_player->unpause_music();
    } else {
      music_player->pause_music();
    }
  }

  if (ImGui::Checkbox("Play SFX", &play_sfx)) {
    if (play_sfx) {
      music_player->unpause_sound();
    } else {
      music_player->pause_sound();
    }
  }
}

void draw_users_table(const UserMap &users) {
  const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
  vector<User> sorted_users;
  char max_score[32];
  for (const auto &[_, u] : users) {
    sorted_users.push_back(u);
  }
  stable_sort(
      sorted_users.begin(), sorted_users.end(),
      [](const User &a, const User &b) { return a.max_score > b.max_score; });
  if (ImGui::BeginTable("Top Scores", 4, flags)) {
    // Add headers
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Max Score");
    ImGui::TableSetupColumn("Score Date");
    ImGui::TableSetupColumn("Register Date");
    ImGui::TableHeadersRow();

    for (size_t i = 0; i < 10 && i < sorted_users.size(); i++) {
      ImGui::TableNextRow();
      const auto &u = sorted_users[i];
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(u.name.c_str());

      ImGui::TableSetColumnIndex(1);
      snprintf(max_score, 30, "%lu", u.max_score);
      ImGui::TextUnformatted(max_score);

      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(format_utc_time(u.max_score_time).c_str());

      ImGui::TableSetColumnIndex(3);
      ImGui::TextUnformatted(format_utc_time(u.register_time).c_str());
    }
    ImGui::EndTable();
  }
}