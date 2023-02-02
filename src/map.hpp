#pragma once

#include <fstream>
#include <filesystem>
#include <regex>

#include "SDL.h"
#include "SDL_image.h"

#include "utils/common.h"

#define MAPS_PATH "../art/maps/"
#define NEWMAP_NAME "generator/new_locations.txt"
#define NEWMAP_COLOR SDL_Color{0x35, 0x84, 0xe4, 0xff}
#define NEWMAP_SEC_COLOR SDL_Color{0xf6, 0xd3, 0x2d, 0xff}

static void strip_format(string &s) {
    auto ptr = s.rfind('.');
    s.erase(ptr);
}

static void normalize_name(string &s) {
    for(auto &c:s) {
        if(c == '-' || c == '_'){
            c = ' ';
        }
    }
}

using std::ifstream;
using std::ofstream;
using std::regex;

class Map {
public:
    int num_points;

    SDL_Renderer *rend;

    string path_name;
    string background_name;

    string locs_path;
    string path_path;
    string bkg_path;

    vector<pii> locations;

    SDL_Texture *path_txt;
    SDL_Texture *background_txt;
    SDL_Texture *blended_txt;

    Map(const string &_path_name, const string &_bkg_name, SDL_Renderer* const _renderer)
    : rend(_renderer), path_name(_path_name), background_name(_bkg_name){

        normalize_name(path_name);
        normalize_name(background_name);

        path_path = MAPS_PATH + _path_name + ".png";
        bkg_path = MAPS_PATH + _bkg_name + ".png";

        path_txt = IMG_LoadTexture(rend, path_path.c_str());
        background_txt = IMG_LoadTexture(rend, bkg_path.c_str());
        blended_txt = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET, SCREEN_W, SCREEN_H);

        assert(path_txt != nullptr);
        assert(background_txt != nullptr);
        assert(blended_txt != nullptr);

        assert(SDL_SetRenderTarget(rend, blended_txt) == 0);
        SDL_RenderCopy(rend, background_txt, NULL, NULL);
        SDL_SetTextureBlendMode(path_txt, SDL_BLENDMODE_ADD);
        SDL_RenderCopy(rend, path_txt, NULL, NULL);
        SDL_SetRenderTarget(rend, NULL);

        locs_path = MAPS_PATH + _path_name + ".txt";

        ifstream map_file(locs_path);
        assert(map_file.is_open());
        {
            int number_of_coordinates = 0;
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
            map_file.close();
        }
        num_points = locations.size();
    }
    ~Map() {
        SDL_DestroyTexture(path_txt);
        SDL_DestroyTexture(background_txt);
        SDL_DestroyTexture(blended_txt);
    }

    void draw_background() {
        SDL_RenderCopy(rend, blended_txt, NULL, NULL);
    }
};

vector<Map*> get_all_maps(SDL_Renderer *rend) {
    vector<Map*> result;
    regex str_expr ("^map-(.*)\\.txt$");
    for(const auto &entry: std::filesystem::directory_iterator(MAPS_PATH)) {
        if(!entry.is_regular_file()) {
            continue;
        }
        string name = entry.path().filename();
        if (regex_match(name.begin(), name.end(), str_expr)) {
            strip_format(name);
            string bkg_name = "background" + name.substr(3);
            result.push_back(new Map(name, bkg_name, rend));
        }
    }
    return result;
}

class NewMap{
public:
    vector<pii> added_points;
    ofstream output_file;

    SDL_Renderer *rend;

    NewMap(SDL_Renderer *_rend) {
        output_file.open(MAPS_PATH NEWMAP_NAME);
        assert(output_file.good());

        rend = _rend;
    }
    ~NewMap(){
        output_file.close();
        char buf[100];
        char buf2[100];
        string result;
        UID(map_gen, 100, 999'999);
        int rnd = map_gen(rng);
        snprintf(buf, 90, "../art/maps/%%s-%06d.%%s", rnd);
        snprintf(buf2, 90, buf, "map", "txt");
        result = "python3 ../art/maps/generator/interpolation.py ";
        result += MAPS_PATH NEWMAP_NAME " ";
        result += buf2;
        system(result.c_str());

        result = "python3 ../art/maps/generator/mask.py ";
        result += buf2;
        result += ' ';
        snprintf(buf2, 90, buf, "map", "png");
        result += buf2;
        int res = system(result.c_str());

        if(res != 0) {
            return;
        }

        result = "cp ../art/maps/background-01.png ";
        snprintf(buf2, 90, buf, "background", "png");
        result += buf2;
        system(result.c_str());

        result = "cp " MAPS_PATH NEWMAP_NAME " ";
        snprintf(buf2, 90, buf, "raw_map", "txt");
        result += buf2;
        system(result.c_str());
    }

    void add_point(const pii location) {
        added_points.push_back(location);
        output_file << location.first << ' ' << location.second << endl;
    }

    void draw_status(const pii mouse_pos) {
        if(added_points.empty()){
            return;
        }
        RENDER_COLOR(rend, NEWMAP_COLOR);
        for(int i = 1; i < (int)added_points.size(); i++){
            SDL_RenderDrawLine(rend,
                added_points[i-1].first, added_points[i-1].second,
                added_points[i].first, added_points[i].second);
        }
        auto [x,y] = added_points.back();
        RENDER_COLOR(rend, NEWMAP_SEC_COLOR);
        SDL_RenderDrawLine(rend,
            x, y,
            mouse_pos.first, mouse_pos.second
        );
    }
};