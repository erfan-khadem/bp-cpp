#include <bits/stdc++.h>
#include "SDL.h"

#include "utils/error_handling.h"
#include "music-player.h"

using namespace std;

int main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {
	SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface __attribute__((unused));
    SDL_Event event;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if (SDL_CreateWindowAndRenderer(1280, 800, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    auto mp = MusicPlayer();
    if(!INIT_SUCCESS(mp)) {
        return 1;
    }
    mp.play_music(0); // Start from the first music

    while (1) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
