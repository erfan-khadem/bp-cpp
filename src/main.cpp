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
    balls_rect.x =  100;
    balls_rect.y = 200;
    balls_rect2.h = 40;
    balls_rect2.w = 40;
    balls_rect2.x =  40;
    balls_rect2.y = 40;
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
    int counter_for_loc = 0;  
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

    Gun_image = IMG_Load("../art/images/Gun_01.png");
    ball_01_image = IMG_Load("../art/images/ball_01.png");
    ball_02_image = IMG_Load("../art/images/ball_02.png");

    if (!(Gun_image || ball_01_image || ball_02_image))
    {
        cerr << "image not loaded" << endl;
    }
    SDL_Texture *Gun_Texture = SDL_CreateTextureFromSurface(renderer, Gun_image);
    SDL_Texture *ball_01_texture = SDL_CreateTextureFromSurface(renderer, ball_01_image);
    SDL_Texture *ball_02_texture = SDL_CreateTextureFromSurface(renderer, ball_02_image);
    bool should_quit = false;
    
    
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
            counter_for_loc++;
        }
        //cout<<locations[counter2_for_loc-1].first<<" "<<locations[counter2_for_loc-1].second<<endl;
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
        const double phase = vector_phase(diff) + (PI / 2.0);
        SDL_RenderCopy(renderer , ball_01_texture , NULL , &balls_rect);// show ball 1 in its position
        SDL_RenderCopy(renderer , ball_02_texture , NULL , &balls_rect2);// show ball 2 in its position
        SDL_RenderCopyEx(renderer, Gun_Texture, NULL, &Gun_rect, phase * RAD_TO_DEG, NULL, SDL_FLIP_NONE);//rotate_and_show wepon
        SDL_RenderPresent(renderer);
    }
    SDL_FreeSurface(Gun_image);
    SDL_FreeSurface(background_01);
    SDL_DestroyTexture(Gun_Texture);
    SDL_DestroyTexture(background_01_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
