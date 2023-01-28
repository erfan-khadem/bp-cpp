#include <fstream>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#include <unistd.h>

#include "SDL.h"
#include "SDL_image.h"

#include "utils/error_handling.h"
#include "utils/common.h"
#include "music-player.h"
#include "ball.hpp"
#include "shelik.hpp"

using namespace std;

#define BALL_TEXT_COUNT 5

static bool should_record_map = false;

void parse_arguments(const int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "t")) != -1) {
        switch (opt) {
        case 't':
            should_record_map = true;
            break;
        default:
            cerr << "The only valid flag is -t for creating a new map" << endl;
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char **argv)
{
    parse_arguments(argc, argv);

    char buf[100];
    double counter_for_loc = 0;  
    double counter_for_firedball = 0;
    s64 prev_time = 0;
    s64 curr_time = 0;

    vector<int> Dx;
    vector<int> Dy;
    vector <int> diff_cntrloc;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    SDL_Rect gun_rect = {.x=600, .y=320, .w=80, .h=160};

    SDL_Texture *gun_texture;
    SDL_Texture *map_texture;

    vector<SDL_Texture*> ball_textures;
    vector<pair<int,int>> locations;
    vector<Ball> balls;
    vector<Shelik> fired_balls;

    /*
    reading existing road -> (the first line of the "locations_file.txt"
    shows the existence of road , second line is number of coordinates);
    if you made new road , add "1" and number of coordinates.
    */
    ifstream map_file;
    ofstream new_map_location;

    map_file.open("../art/maps/map01.txt");
    int i_want_new_map = 0;
    int number_of_coordinates = 0;
    {
        int is_map_available;
        map_file >> is_map_available;
        if(is_map_available == 1){
            map_file >> number_of_coordinates;
            for (int i = 0; i < number_of_coordinates;i++)
            {
                int x, y;
                map_file >> x >> y;
                locations.emplace_back(x,y);
            }
        }
    }
    map_file.close();

    MusicPlayer *music_player = nullptr;

    // TODO: Delete me!
    pdd last_mouse_pos = {SCREEN_W >> 1, SCREEN_H >> 1};// Do not change!
    SDL_Color the_line_color = {30, 0, 255, 255};

    const pdd center = last_mouse_pos;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Couldn't initialize SDL: %s",
            SDL_GetError()
        );
        return 3;
    }

    music_player = new MusicPlayer();
    if (!INIT_SUCCESS(*music_player))
    {
        return 1;
    }

    window = SDL_CreateWindow("BP-cpp",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_W, SCREEN_H, SDL_WINDOW_OPENGL);

    renderer = SDL_CreateRenderer(window,
                                  -1,
                                  SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    if (!window || !renderer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    // setup and initialize sdl2_image library
    {
        int flag_IMG = IMG_INIT_PNG;
        int IMG_init_status = IMG_Init(flag_IMG);
        if ((IMG_init_status & flag_IMG) != flag_IMG)
        {
            cerr << "sdl2image format not available" << endl;
        }
    }

    music_player->play_music(0); // Start from the first music
    SDL_Surface *background_01 = IMG_Load("../art/images/ground_02.jpg");
    SDL_Texture *background_01_tex = SDL_CreateTextureFromSurface(renderer, background_01);
    if (!(background_01 || background_01_tex))
    {
        SDL_ShowSimpleMessageBox(0, "background init error", SDL_GetError(), window);
        return 1;
    }
    for(int i = 1; i <= BALL_TEXT_COUNT; i++) {
        snprintf(buf, 90, "../art/images/ball%02d.png", i);
        auto surf = IMG_Load(buf);
        ball_textures.push_back(SDL_CreateTextureFromSurface(renderer, surf));
        assert(ball_textures.back() != nullptr);
    }
    {
        auto surf = IMG_Load("../art/maps/map01.png");
        map_texture = SDL_CreateTextureFromSurface(renderer, surf);
    }
    {
        SDL_Surface *gun_image;
        gun_image = IMG_Load("../art/images/gun01.png");
        gun_texture = SDL_CreateTextureFromSurface(renderer, gun_image);
    }
    {
        UID(color_gen, 0, ball_textures.size() - 1);
        for(int i = 0; i < 20; i++) {
            const int ball_color = color_gen(rng);
            balls.emplace_back(ball_color, ball_textures[ball_color], renderer, &locations);
        }
    }
    bool should_quit = false;
    
    //uncomment if want new road; 
    if(should_record_map){
        new_map_location.open("../art/maps/generator/new_locations_file.txt", ios::out);
        i_want_new_map = 1;
    }

    while (!should_quit)
    {
        while (!should_quit && SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                should_quit = true;
                break;

            case SDL_MOUSEMOTION:
                last_mouse_pos = {event.motion.x, event.motion.y};
                break;
            case SDL_MOUSEBUTTONDOWN:
                if(should_record_map && i_want_new_map == 1){
                    new_map_location << event.motion.x << " " << event.motion.y << endl;//making new map if you want new map; 
                }
                fired_balls.emplace_back(3, ball_textures[3], renderer);
                Dx.insert(Dx.begin() ,last_mouse_pos.first - center.first);
                Dy.insert(Dy.begin(),last_mouse_pos.second - center.second);
                diff_cntrloc.insert(diff_cntrloc.begin() , counter_for_firedball);
                fired_balls[fired_balls.size()-1].diff_cntrloc = counter_for_firedball;
                the_line_color = {255, 0, 255, 255};
                break;
            case SDL_MOUSEBUTTONUP:
                the_line_color = {30, 0, 255, 255};
                break;

            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_SPACE){
                    the_line_color = {255, 0, 255, 255};
                    i_want_new_map = 0;
                    cerr << "I stopped recording coordinates!" << endl;
                }
                break;

            case SDL_KEYUP:
                the_line_color = {30, 0, 255, 255};
                break;

            default:
                break;
            }    
        }
        curr_time = (s64) SDL_GetTicks64();
        double dt = curr_time - prev_time;
        counter_for_firedball += 10 * dt/1000 * 100;
        if(counter_for_loc < locations.size()){
            dt /= 1000;
            counter_for_loc += .5 * dt * 100; // 2 positions every 100ms
        }
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff); // sets the background color
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, background_01_tex, NULL, NULL);
        SDL_RenderCopy(renderer, map_texture, NULL, NULL);
        SDL_SetRenderDrawColor(renderer, the_line_color.r, the_line_color.g, the_line_color.b, 0xff);
        SDL_RenderDrawLine(renderer,
                           SCREEN_W >> 1, SCREEN_H >> 1,
                           last_mouse_pos.first, last_mouse_pos.second);
        pdd diff = last_mouse_pos - center;
        const double phase = vector_phase(diff) + (PI / 2.0);
        SDL_RenderCopyEx(renderer, gun_texture, NULL, &gun_rect, phase * RAD_TO_DEG, NULL, SDL_FLIP_NONE);//rotate and show wepon
        balls[0].move_to_location(counter_for_loc);
        for (int i = 0; i < int(fired_balls.size()); i++)
        {
            fired_balls[i].move_to_location(counter_for_firedball , Dy[i] , Dx[i] , diff_cntrloc[i]);
        }
        for(int i = 0; i < int(fired_balls.size()); i++) {
            fired_balls[i].draw_fired_ball();
        }
        for(int i = 1; i < int(balls.size()); i++) {
            const int pos = balls[i - 1].get_previous_ball_pos();
            balls[i].move_to_location(pos);
        }
        for(int i = 0; i < int(balls.size()); i++) {
            balls[i].draw_ball();
        }
        for(int i = 0; i < int(balls.size()); i++) {
            if(fired_balls.size()>0){
            if(balls[i].iscolided(fired_balls[0].position)){       
                if(balls[i].ball_color == fired_balls[0].ball_color){
                    cout<<balls[i].iscolided(fired_balls[0].position)<<endl;    
                    fired_balls.erase(fired_balls.begin());
                    balls.erase(balls.begin()+i);
                    cout<<i<<endl;
                    break;
                }
                else{
                    
                }
            }
          }
        }
        prev_time = curr_time;
        SDL_RenderPresent(renderer);
    }
    SDL_FreeSurface(background_01);
    SDL_DestroyTexture(background_01_tex);
    SDL_DestroyTexture(gun_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
