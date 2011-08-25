#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string>
#include <vector>
 
 
// Constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;
 
const int ROOM_WIDTH = 10;
const int ROOM_HEIGHT = 7;
 
const int TILE_WIDTH = 16;
const int TILE_HEIGHT = 16;
 
const int CLIP_MAX = 128;
 
int tileArray[CLIP_MAX] = {
        0,0,0,0,0,0,0,0,0,0,
        0,0,1,1,1,1,1,1,0,0,
        0,1,2,2,2,2,2,2,1,0,
        0,1,2,2,2,2,2,2,1,0,
        0,1,2,2,2,2,2,2,1,0,
        0,0,1,1,1,1,1,1,0,0,
        0,0,0,0,0,0,0,0,0,0
};
 
// u = unblock, b = block, w = water.
// You can mix these together
 
std::string typeArray[CLIP_MAX] = {
        "b", "b", "b", "b", "b", "b", "b", "b", "b", "b",
        "b", "b", "u", "u", "u", "u", "u", "u", "b", "b",
        "b", "b", "uw", "uw",  "uw", "uw", "uw", "uw", "b", "b",
        "b", "b", "u", "u", "u", "u", "u", "u", "b", "b",
        "b", "b", "u", "u", "u", "u", "u", "u", "b", "b",
        "b", "b", "u", "u", "u", "u", "u", "u", "b", "b",
        "b", "b", "b", "b", "b", "b", "b", "b", "b", "b"
};

// need this to associate types w/ clips = Tiles!

struct Tile {
	SDL_Rect* clip;
	std::string type;
};


// SDL Shit
 
SDL_Surface* screen = NULL;
SDL_Surface* tileset = NULL;
 
SDL_Event Event;
 
SDL_Rect clip[CLIP_MAX];
 
SDL_Surface* loadImage(std::string fileName) {
        SDL_Surface* loadedImage = IMG_Load(fileName.c_str());
        SDL_Surface* optimizedImage = NULL;
 
        if (loadedImage != NULL) {
                optimizedImage = SDL_DisplayFormat(loadedImage);
                SDL_FreeSurface(loadedImage);
        }
 
        if (optimizedImage != NULL) {
                Uint32 colorkey = SDL_MapRGB(optimizedImage->format, 255, 0, 255);
                SDL_SetColorKey(optimizedImage, SDL_SRCCOLORKEY | SDL_RLEACCEL, colorkey);
        }
 
        return optimizedImage;
}
 
void applySurface(int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip = NULL) {
        SDL_Rect offset;
 
        offset.x = x;
        offset.y = y;
 
        SDL_BlitSurface(source, clip, destination, &offset);
}
 
void generateClips(SDL_Surface* _source, int _tileWidth, int _tileHeight, SDL_Rect* clip, int _clipMax) {
        int incr = 0;
        for (int i = 0; i < /*_source->h*/ 1; ++i) {
                for (int j = 0; j < /*_source->w*/3; ++j) {
                        clip[(j+incr)].x = ((j+incr) * _tileWidth);
                        clip[(i+incr)].y = ((i+incr) * _tileHeight);
                        clip[(i+j+incr)].w = _tileWidth;
                        clip[(i+j+incr)].h = _tileHeight;
                        if (j >= _source->w)
                                incr = j;
                }
        }
}

// Debating on whether or not to generate Clips + Types into Tiles in one func or not.
// Thinking about keeping them seperate, can build a wrapper function if need be.... I guess

void generateTiles(std::vector _tilesVec, std::string* _typeArray, SDL_Rect* clip) {
        for (int i = 0; i < CLIP_MAX; ++i) {
        	tempTile = new Tile;
        	// Can i do this? Can't test. Would like to just copy the whole clip struct
        	// Might be easier if possible.
        	tempTile.clip = &clip;
        	// / end ^
        	// else ill have to do it member by member.
        	tempTile.clip.x = clip[i].x;
        	tempTile.clip.y = clip[i].y;
                tempTile.clip.w = clip[i].w;
                tempTile.clip.h = clip[i].h;
                
                tempTile.type = _typeArray[i];
                
                
                _tilesVec.push_back(tempTile);
        }
}
 
int main(int argc, char *argv[]) {
        bool quit = false;
 
        if ((SDL_Init(SDL_INIT_EVERYTHING)==-1)) {
                return 1;
        }
        
        // I should get rid of this clipping vector, and just read through the normal array.
        // wasting space n shit.
        //std::vector<int> clipVec(tileArray, tileArray + sizeof(tileArray) / sizeof(int));
        
        std::vector<Tiles> tilesVec();
 
        screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE);
 
        tileset = loadImage("tileset16.png");
        
        generateClips(tileset, TILE_WIDTH, TILE_HEIGHT, clip, CLIP_MAX);
        
        generateTiles(tilesVec, &typeArray, clip);
 
        int xOffset = 0;
        int yOffset = 0;
        int incr = 0;
        int row_incr = 0;
 
        while (quit == false) {
                while (SDL_PollEvent(&Event)) {
                        if (Event.type == SDL_QUIT) {
                                quit = true;
                        }
 
                        for (int i = 0; i < ROOM_HEIGHT; ++i) {
                                if (row_incr >= ROOM_HEIGHT) {
                                        break;
                                } else {
                                        row_incr++;
                                }
                                for (int j = 0; j < ROOM_WIDTH; ++j) {
                                        applySurface(xOffset, yOffset, tileset, screen, &clip[tileArray[j]]);
                                        xOffset += TILE_WIDTH;
                                }
 
                                xOffset = 0;
                                yOffset += TILE_HEIGHT;
                                incr += (ROOM_WIDTH-1);
                        }
                        xOffset = yOffset = 0;
                        if (SDL_Flip(screen) == -1)
                                return 1;
                }
        }
 
        SDL_FreeSurface(tileset);
        SDL_FreeSurface(screen);
 
        SDL_Quit();
 
        exit(0);
}