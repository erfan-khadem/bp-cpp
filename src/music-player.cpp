#include "music-player.h"

#include <filesystem>
#include <regex>

#include "utils/common.h"

using namespace std;

#define MUSIC_PATH "../art/music/"

static MusicPlayer* main_music_player = nullptr;

static void music_stopped(void) {
    assert(main_music_player != nullptr);
    main_music_player->play_music(-1);
}

MusicPlayer::MusicPlayer(){
    music = nullptr;
    stop_music = false;
    stop_sound = false;
    curr_music = 0;
    success = true;
    if (main_music_player != nullptr) {
        cerr << "Cannot have more than one music player at a time" << endl;
        success = false;
        return;
    }
    main_music_player = this;
    if (Mix_OpenAudio(48'000, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        cerr << "SDL_mixer not initialized: " << Mix_GetError() << endl;
        success = false;
        return;
    }
    // Add in game music files in here
    regex str_expr ("^music-(.*)$");
    for(const auto &entry: filesystem::directory_iterator(MUSIC_PATH)){
        if (!entry.is_regular_file()) {
            continue;
        }
        string name = entry.path().filename();
        if (regex_match(name.begin(), name.end(), str_expr)) {
            in_game_music.push_back(name);
        }
    }
    sort(in_game_music.begin(), in_game_music.end());
    assert(in_game_music.size() > 0);
    Mix_HookMusicFinished(&music_stopped);
}

MusicPlayer::~MusicPlayer() {
    if (music != nullptr){
        Mix_FreeMusic(music);
    }
    Mix_Quit();
}

int MusicPlayer::play_music(int index) {
    if(stop_music) {
        return -1;
    }
    UID(index_gen, 0, in_game_music.size());
    if(index < 0) {
        switch (index)
        {
        case -1:
            index = (curr_music + 1) % in_game_music.size();
            curr_music = index;
            break;
        
        case -2:
            index = index_gen(rng);
            curr_music = index;
            break;

        case -3:
            index = curr_music;
            break;
        
        default:
            cerr << "Invalid music index: " << index << ". ";
            cerr << "Selecting a song randomly." << endl;
            UID(index_gen, 0, in_game_music.size() - 1);
            index = index_gen(rng);
            curr_music = index;
            break;
        }
    }
    assert(index < (int)in_game_music.size());
    if (music != nullptr) {
        Mix_FreeMusic(music);
    }
    const string file_name = MUSIC_PATH + in_game_music[index];
    music = Mix_LoadMUS(file_name.c_str());
    if (music == nullptr) {
        cerr << "Could not load music with index " << index << " and name "
             << in_game_music[index] << endl;
        return -1;
    }
    Mix_PlayMusic(music, 0);
    return index;
}

int MusicPlayer::play_sound(const string &filename){
    if(this->stop_sound) {
        return 0;
    }
    Mix_Chunk *tmp = nullptr;
    if(effects_cache.find(filename) != effects_cache.end()) {
        tmp = effects_cache[filename];
    } else {
        const string comp_fn = MUSIC_PATH + filename;
        tmp = Mix_LoadWAV(comp_fn.c_str());
        effects_cache[filename] = tmp;
    }
    if (tmp == nullptr) {
        cerr << "Could not load wav with filename "
             << filename << endl;
        return -1;
    }
    int ret = Mix_PlayChannel(-1, tmp, 0);
    if (ret < 0) {
        cerr << "Could not play wav with filename "
            << filename << endl;
        return -2;
    }
    return 0;
}

void MusicPlayer::pause_music() {
    this->stop_music = true;
    Mix_HaltMusic();
}

void MusicPlayer::unpause_music() {
    this->stop_music = false;
    this->play_music(-3);
}

void MusicPlayer::pause_sound() {
    this->stop_sound = true;
}

void MusicPlayer::unpause_sound() {
    this->stop_sound = false;
}