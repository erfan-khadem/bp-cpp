#pragma once

#include "imgui.h"

#include "user.h"

void draw_users_table(const UserMap &users) {
    const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
    vector<User> sorted_users;
    char max_score[32];
    for(const auto &[_, u]: users) {
        sorted_users.push_back(u);
    }
    stable_sort(sorted_users.begin(), sorted_users.end(), 
        [](const User &a, const User &b) {
            return a.max_score > b.max_score;
        });
    if (ImGui::BeginTable("Top Scores", 4, flags)){
        // Add headers
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Max Score");
        ImGui::TableSetupColumn("Score Date");
        ImGui::TableSetupColumn("Register Date");
        ImGui::TableHeadersRow();

        for(size_t i = 0; i < 10 && i < sorted_users.size(); i++) {
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