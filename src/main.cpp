#include "SDL.h"

#include "utils/error_handling.h"
#include "utils/common.h"
#include "music-player.h"

using namespace std;

int main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {
	SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface __attribute__((unused));
    SDL_Event event;

    MusicPlayer *music_player = nullptr;

    pii last_mouse_pos = {SCREEN_W >> 1, SCREEN_H >> 1};

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    music_player = new MusicPlayer();
    if(!INIT_SUCCESS(*music_player)) {
        return 1;
    }

    window = SDL_CreateWindow("BP-cpp",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_W, SCREEN_H, SDL_WINDOW_OPENGL);
    
    renderer = SDL_CreateRenderer(window,
        -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    if (!window || !renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }
    music_player->play_music(0); // Start from the first music

    bool should_quit = false;

    while (!should_quit) {
        while(!should_quit && SDL_PollEvent(&event)){
            switch (event.type)
            {
            case SDL_QUIT:
                should_quit = true;
                break;

            case SDL_MOUSEMOTION:
                last_mouse_pos = {event.motion.x, event.motion.y};
                break;
            
            default:
                break;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff); // sets the background color
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xff, 0xff);
        SDL_RenderDrawLine(renderer,
            SCREEN_W >> 1, SCREEN_H >> 1,
            last_mouse_pos.first, last_mouse_pos.second);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
