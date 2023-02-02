#pragma once

#include "imgui.h"
#include "utils/common.h"

#include "user.h"

void AlignForWidth(float width, float alignment = 0.5f)
{
    float avail = ImGui::GetContentRegionAvail().x;
    float off = (avail - width) * alignment;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}

User* login_or_register(UserMap &umap) {
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

    if(should_register && !should_show_password) {
        ImGui::InputText("Retype Password", &retype_pass[0], retype_pass.size(),
            should_show_password ? 0 : password_flags);
    }

    float button_width = 0;
    button_width += ImGui::CalcTextSize("Login").x;
    button_width += should_register ? ImGui::CalcTextSize("Register User").x : 0.0;

    AlignForWidth(button_width);

    if(ImGui::Button("Login")) {
        auto usr = umap.find(username);
        if(usr == umap.end()) {
            goto show_registration;
        }
        auto &user = usr->second;
        if(verify_user(user, password)) {
            ImGui::End();
            return &user;
        }
    }
show_registration:
    if(should_register){
        passwords_match = should_show_password ||
            (strcmp(password.c_str(), retype_pass.c_str()) == 0);
        ImGui::SameLine();
        if(!passwords_match){
            ImGui::BeginDisabled();
        }
        if(ImGui::Button("Register User")) {
            User* user = nullptr;
            if(umap.find(username) != umap.end()) {
                goto end_draw;
            }
            user = new User(username, password);
            umap[username] = *user;
            ImGui::End();
            return user;
        }
        if(!passwords_match){
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
    ImGui::End();
}

void user_won() {
    ImGui::Begin("You Won!", NULL, ImGuiWindowFlags_NoTitleBar);
    float width = ImGui::CalcTextSize("You Won!").x;
    AlignForWidth(width);
    ImGui::TextColored(ImVec4(0.18, 0.76, 0.49, 1.0), "You Won!");
    ImGui::End();
}