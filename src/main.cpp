#include "SDL.h"
#include "SDL_image.h"
#include "math.h"
#include "utils/error_handling.h"
#include "utils/common.h"
#include "music-player.h"
#include "fstream"
using namespace std;

int main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface __attribute__((unused));
    SDL_Event event;
    SDL_Rect Gun_rect;
    Gun_rect.h = 160;
    Gun_rect.w = 80;
    Gun_rect.x = 600;
    Gun_rect.y = 320;
    SDL_Rect balls_rect , balls_rect2;
    balls_rect.h = 40;
    balls_rect.w = 40;
    balls_rect.x = 100;
    balls_rect.y = 200;
    balls_rect2 = balls_rect;

    //reading existing road -> (the first line of the "locations_file.txt" shows the existence of road , second line is number of coordinates);
    //if you made new road , add "1" and number of coordinates.
    ifstream location_file;
    location_file.open("../src/locations_file.txt");
    int is_map_available;
    int i_want_new_map = 0;
    int number_of_coordinates = 0;
    location_file>>is_map_available;
    vector< pair <int,int>>locations;
    if(is_map_available == 1){
    location_file>>number_of_coordinates;
    for (int i = 0; i < number_of_coordinates;i++)
    {
        int x;
        location_file>>x;
        int y;
        location_file>>y;
        locations.push_back(make_pair(x,y));
    }
    }
    double counter_for_loc = 0;  
    location_file.close();

    //-----------
    MusicPlayer *music_player = nullptr;

    pdd last_mouse_pos = {SCREEN_W >> 1, SCREEN_H >> 1};// Do not change!
    SDL_Color the_line_color = {30, 0, 255};

    const pdd center = last_mouse_pos;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
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
    music_player->play_music(0); // Start from the first music
    SDL_Surface *background_01 = SDL_LoadBMP("../art/images/ground_02.bmp");
    SDL_Texture *background_01_tex = SDL_CreateTextureFromSurface(renderer, background_01);
    if (!(background_01 || background_01_tex))
    {
        SDL_ShowSimpleMessageBox(0, "background init error", SDL_GetError(), window);
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
    //----------------------------
    SDL_Surface *Gun_image;
    SDL_Surface *ball_01_image;
    SDL_Surface *ball_02_image;
    SDL_Surface *saye1_image;
    SDL_Surface *saye2_image;
    SDL_Surface *saye3_image;
    SDL_Surface *saye4_image;
    SDL_Surface *saye5_image;
    SDL_Surface *saye6_image;
    SDL_Surface *saye7_image;
    SDL_Surface *saye8_image;
    SDL_Surface *saye9_image;
    SDL_Surface *saye10_image;
    SDL_Surface *saye11_image;
    SDL_Surface *saye12_image;
    SDL_Surface *saye13_image;
    SDL_Surface *saye14_image;
    SDL_Surface *saye15_image;
    //SDL_Surface *saye16_image;

    Gun_image = IMG_Load("../art/images/Gun_01.png");
    ball_01_image = IMG_Load("../art/images/ball_01.png");
    ball_02_image = IMG_Load("../art/images/ball_02.png");
    saye1_image = IMG_Load("../art/images/s1.png");
    saye2_image = IMG_Load("../art/images/s2.png");
    saye3_image = IMG_Load("../art/images/s3.png");
    saye4_image = IMG_Load("../art/images/s4.png");
    saye5_image = IMG_Load("../art/images/s5.png");
    saye6_image = IMG_Load("../art/images/s6.png");
    saye7_image = IMG_Load("../art/images/s7.png");
    saye8_image = IMG_Load("../art/images/s8.png");
    saye9_image = IMG_Load("../art/images/s9.png");
    saye10_image = IMG_Load("../art/images/s10.png");
    saye11_image = IMG_Load("../art/images/s11.png");
    saye12_image = IMG_Load("../art/images/s12.png");
    saye13_image = IMG_Load("../art/images/s13.png");
    saye14_image = IMG_Load("../art/images/s14.png");
    saye15_image = IMG_Load("../art/images/s15.png");
    //saye16_image = IMG_Load("../art/images/s16.png");


    if (!(Gun_image || ball_01_image || ball_02_image || saye1_image))
    {
        cerr << "image not loaded" << endl;
    }

    SDL_Texture *Gun_Texture = SDL_CreateTextureFromSurface(renderer, Gun_image);
    SDL_Texture *ball_01_texture = SDL_CreateTextureFromSurface(renderer, ball_01_image);
    SDL_Texture *ball_02_texture = SDL_CreateTextureFromSurface(renderer, ball_02_image);
    SDL_Texture *saye1tex = SDL_CreateTextureFromSurface(renderer, saye1_image);
    SDL_Texture *saye2tex = SDL_CreateTextureFromSurface(renderer, saye2_image);
    SDL_Texture *saye3tex = SDL_CreateTextureFromSurface(renderer, saye3_image);
    SDL_Texture *saye4tex = SDL_CreateTextureFromSurface(renderer, saye4_image);
    SDL_Texture *saye5tex = SDL_CreateTextureFromSurface(renderer, saye5_image);
    SDL_Texture *saye6tex = SDL_CreateTextureFromSurface(renderer, saye6_image);
    SDL_Texture *saye7tex = SDL_CreateTextureFromSurface(renderer, saye7_image);
    SDL_Texture *saye8tex = SDL_CreateTextureFromSurface(renderer, saye8_image);
    SDL_Texture *saye9tex = SDL_CreateTextureFromSurface(renderer, saye9_image);
    SDL_Texture *saye10tex = SDL_CreateTextureFromSurface(renderer, saye10_image);
    SDL_Texture *saye11tex = SDL_CreateTextureFromSurface(renderer, saye11_image);
    SDL_Texture *saye12tex = SDL_CreateTextureFromSurface(renderer, saye12_image);
    SDL_Texture *saye13tex = SDL_CreateTextureFromSurface(renderer, saye13_image);
    SDL_Texture *saye14tex = SDL_CreateTextureFromSurface(renderer, saye14_image);
    SDL_Texture *saye15tex = SDL_CreateTextureFromSurface(renderer, saye15_image);
    //SDL_Texture *saye16tex = SDL_CreateTextureFromSurface(renderer, saye16_image);

    SDL_Texture *sayeha_araye[15] = {saye1tex,saye2tex,saye3tex,saye4tex,saye5tex,saye6tex,saye7tex,saye8tex,saye9tex,saye10tex,saye11tex,saye12tex,saye13tex,saye14tex,saye15tex};
    

    bool should_quit = false;
    int saye_index;
    double counter_for_saye = 0;
    
    //uncomment if want new road; 
    /*
    fstream location_file2;
    location_file2.open("../src/locations_file.txt", ios::out);
    i_want_new_map = 1;
    */

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
                    
                    //uncomment if want new road;
                    /*
                    if(i_want_new_map == 1){
	            	location_file2 <<event.motion.x<<" "<<event.motion.y<<endl;//making new map if you want new map;
	            	}
                    */
                break;
            case SDL_MOUSEBUTTONDOWN:
                the_line_color = {255, 0, 255};
                //uncomment if want new road
                //i_want_new_map = 0;
                break;
            case SDL_MOUSEBUTTONUP:
                the_line_color = {30, 0, 255};
                break;
            case SDLK_SPACE:
                break;
 

            default:
                break;
            }    
        }
        if(is_map_available == 1 && counter_for_loc<locations.size()){
            counter_for_loc+=.32;
            counter_for_saye += 1.3;
        }

        balls_rect.x = locations[counter_for_loc-1].first;
        balls_rect.y = locations[counter_for_loc-1].second;
        balls_rect2.x = locations[counter_for_loc-41].first;
        balls_rect2.y = locations[counter_for_loc-41].second;
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff); // sets the background color
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, background_01_tex, NULL, NULL);
        SDL_SetRenderDrawColor(renderer, the_line_color.r, the_line_color.g, the_line_color.b, 0xff);
        SDL_RenderDrawLine(renderer,
                           SCREEN_W >> 1, SCREEN_H >> 1,
                           last_mouse_pos.first, last_mouse_pos.second);
        pdd diff = last_mouse_pos - center;
        pdd diff_ball = {locations[counter_for_loc+5].first - locations[counter_for_loc-5].first , locations[counter_for_loc+5].second - locations[counter_for_loc-5].second};
        const double phase = vector_phase(diff) + (PI / 2.0);
        const double phase_ball = vector_phase(diff_ball);
        SDL_RenderCopy(renderer , ball_01_texture , NULL , &balls_rect);// show ball 1 in its position
        SDL_RenderCopy(renderer , ball_02_texture , NULL , &balls_rect2);// show ball 2 in its position

        if(counter_for_saye < 150){
            saye_index = counter_for_saye/10;
            SDL_RenderCopyEx(renderer , sayeha_araye[saye_index] , NULL , &balls_rect , phase_ball*RAD_TO_DEG, NULL , SDL_FLIP_NONE);
            }
        if(counter_for_saye>200){
            counter_for_saye = 0;
        }   

        SDL_RenderCopyEx(renderer, Gun_Texture, NULL, &Gun_rect, phase * RAD_TO_DEG, NULL, SDL_FLIP_NONE);//rotate and show wepon
        SDL_RenderPresent(renderer);
    }
    SDL_FreeSurface(Gun_image);
    SDL_FreeSurface(background_01);
    SDL_DestroyTexture(background_01_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
