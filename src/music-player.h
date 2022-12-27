#pragma once

#include "SDL_mixer.h"
#include <vector>
#include <map>
#include <string>

void basic_music();

class MusicPlayer {
private:
    Mix_Music *music;

    /* The index of the background music currently playing */
    int curr_music;
    bool stop_music;
    bool stop_sound;
public:
    std::vector<std::string> in_game_music;
    bool success;
    /* Plays background music. If `index` is -1, it selects the next song
    *  else if `index` is -2, it selects a song randomly.
    *  else if `index` is -3, it continues from the last song it was playing
    *  Returns the index of the currently playing song.
    */
    int play_music(int index);

    /*
    * Plays a .wav files.
    * Returns zero on success.
    */
    int play_sound(const std::string &filename);
    void pause_music();
    void unpause_music();
    void pause_sound();
    void unpause_sound();

    MusicPlayer();
    ~MusicPlayer();
};