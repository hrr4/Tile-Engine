#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <string>
#include <vector>

/* IDEAS still needing implemented:
	-Particle Effects - For lights n stuff
	-'Painting' different environments - Example: Painting with a grass brush on green tiles will make random
	grass sprites n stuff.
	-Alpha Transparency - For stuff above other layers n such
	
	-On how to clip OpenGL Surfaces - Calculate tile width / pic width, tile height / pic height,
		since glTexCoord's use percents. 
		
	- A way to get this to draw, is to assemble the tiles on a newly created RGB SDL Surface and blit that
*/
 

// Constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;
 
const int ROOM_WIDTH = 10;
const int ROOM_HEIGHT = 7;
 
const int TILE_WIDTH = 16;
const int TILE_HEIGHT = 16;
 
const int CLIP_MAX = 500;

// SDL Shit
 
SDL_Surface* screen = NULL;
 
SDL_Event Event;
 
SDL_Rect clip[CLIP_MAX];

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
	"b", "b", "uw", "uw", "uw", "uw", "uw", "uw", "b", "b",
	"b", "b", "u", "u", "u", "u", "u", "u", "b", "b",
	"b", "b", "u", "u", "u", "u", "u", "u", "b", "b",
	"b", "b", "u", "u", "u", "u", "u", "u", "b", "b",
	"b", "b", "b", "b", "b", "b", "b", "b", "b", "b"
};

// 2 will be the default layer

short signed int layerArray[CLIP_MAX] = {
	1,1,1,1,1,1,1,1,1,1,
	0,0,1,1,1,1,1,1,0,0,
	0,1,2,2,2,2,2,2,1,0,
	0,1,2,2,2,2,2,2,1,0,
	0,1,2,2,2,2,2,2,1,0,
	0,0,1,1,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,0,0
};

// need this to associate types w/ clips = Tiles!

struct Tile {
	SDL_Rect* clip;
	std::string type;
	int layer;
};

// Plane class
// This is the plane we'll draw the tileset on
// So eventually it'll need to be the size of the level
// Not the size of the screen.

class Plane {
public:
	Plane();
	SDL_Surface* Load(std::string _filename);
	void generateClips(GLenum _target, int _tileWidth, int _tileHeight, SDL_Rect* _clip, int _clipMax);
	std::vector<Tile*> generateTiles(std::vector<Tile*> _tilesVec, int* _tileArray, std::string* _typeArray, short int* _layerArray, SDL_Rect* _clip);
	void Draw();
	
private:
	int x, y;
	SDL_Surface* pSurface;
	GLuint texture[1];
	std::vector<Tile*> tilesVec;

	int nextPowerOfTwo(int _num);
};

Plane::Plane() {
	pSurface = NULL;
	x = y = 0;
}

int Plane::nextPowerOfTwo(int _num) {
	_num--;
	_num = (_num >> 1) | _num;
	_num = (_num >> 2) | _num;
	_num = (_num >> 4) | _num;
	_num = (_num >> 8) | _num;
	_num = (_num >> 16) | _num;
	_num++;
	return _num;
}

SDL_Surface* Plane::Load(std::string _filename) {
	GLint nOfColors;
	GLenum texture_format;
	SDL_Surface* loadedImage = IMG_Load(_filename.c_str());
	SDL_Surface* optimizedImage = NULL;
	SDL_Surface* powerSurface = NULL;

	if (loadedImage != NULL) {
		optimizedImage = SDL_CreateRGBSurface(NULL, nextPowerOfTwo(loadedImage->w), 
			nextPowerOfTwo(loadedImage->h), 32, loadedImage->format->Rmask,
			loadedImage->format->Gmask, loadedImage->format->Bmask, loadedImage->format->Amask);
		// Only blitting to test out functionality for now. Will have to blit the rebuilt
		// tilemap when i get it goin.
		SDL_BlitSurface(loadedImage, NULL, optimizedImage, NULL);
		SDL_FreeSurface(loadedImage);
	}
 
	if (optimizedImage != NULL) {
			/*Uint32 colorkey = SDL_MapRGBA(optimizedImage->format, 255, 0, 255, 255);
			SDL_SetColorKey(optimizedImage, SDL_SRCCOLORKEY | SDL_RLEACCEL, colorkey);*/
			//SDL_ConvertSurface(optimizedImage, optimizedImage->format, SDL_HWSURFACE | SDL_RLEACCEL);
	}

	nOfColors = optimizedImage->format->BytesPerPixel;
	
	if (nOfColors == 4) {
		if (optimizedImage->format->Rmask == 0x000000ff)
			texture_format = GL_RGBA;
		else
			texture_format = GL_BGRA;
	} else if (nOfColors == 3) {
		if (optimizedImage->format->Rmask == 0x000000ff)
			texture_format = GL_RGB;
		else
			texture_format = GL_BGR;
	}

	glGenTextures(1, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	glTexImage2D(GL_TEXTURE_2D, 0, nOfColors, optimizedImage->w, optimizedImage->h, 0, texture_format, 
		GL_UNSIGNED_BYTE, optimizedImage->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//glDeleteTextures(1, &texture[0]);

	return optimizedImage;
}

void Plane::generateClips(GLenum _target, int _tileWidth, int _tileHeight, SDL_Rect* _clip, int _clipMax) {
	GLint texWidth[1], texHeight[1]; 
	int incr = 0;
	
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, texWidth);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, texHeight);

	for (int j = 0; j < texHeight[0]; ++j) {
		for (int k = 0; k < texWidth[0]; ++k) {
			clip[(k+incr)].x = ((k+incr) * _tileWidth);
			clip[(j+incr)].y = ((j+incr) * _tileHeight);
			clip[(k+j+incr)].w = _tileWidth;
			clip[(k+j+incr)].h = _tileHeight;
			if (k >= texWidth[0])
				incr = k;
		}
	}
}

std::vector<Tile*> Plane::generateTiles(int* _tileArray, std::string* _typeArray, short int* _layerArray, 
	SDL_Rect* _clip) {
		
		std::vector<Tile*> tempVec;

		for (int i = 0; i < CLIP_MAX; ++i) {
			Tile* tempTile = new Tile;
			
			tempTile->clip = &_clip[_tileArray[i]];
			tempTile->type = _typeArray[i];
			tempTile->layer = _layerArray[i];

			_tilesVec.push_back(tempTile);

		}
		
		tempTile = NULL;
		
		return tempVec;
}

// Assemble Map - This will put the tiles/clips onto an RGB Surface, and queue it for blit.

void Plane::assembleMap(std::vector<Tile*> _tilesVec) {
	
}

void Plane::Draw() {
	//Offset
	glTranslatef(x, y, 0);

	glScaled(.5, .5, 1);
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	// Build
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2i(0, 0);
		glTexCoord2f(1, 0); glVertex2i(1000, 0);
		glTexCoord2f(1, 1); glVertex2i(1000, 1000);
		glTexCoord2f(0, 1); glVertex2i(0, 1000);
	glEnd();
	glFlush();

	//Reset
	glLoadIdentity();
}


bool init_GL() {
	glClearColor(0, 0, 0, 0);
	glEnable(GL_TEXTURE_2D);

	// Projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);

	// Model View
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Errors
	if (glGetError() != GL_NO_ERROR)
		return false;
	return true;
}

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

 
int main(int argc, char *argv[]) {
	Plane tileset = Plane();
	bool quit = false, isFullscreen = false;
 
	if ((SDL_Init(SDL_INIT_EVERYTHING)==-1)) 
		return true;
		
	std::vector<Tile*> tilesVec;
 
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_OPENGL | SDL_RESIZABLE);

	if (init_GL() == false)
		return false;
 
	tileset.Load("tileset16.png");
	
	tileset.generateClips(GL_TEXTURE_2D, TILE_WIDTH, TILE_HEIGHT, clip, CLIP_MAX);
	
	tilesVec = tileset.generateTiles(tileArray, typeArray, layerArray, clip);
		
	//generateClips(sTileset, TILE_WIDTH, TILE_HEIGHT, clip, CLIP_MAX);
		
	//tilesVec = generateTiles(tilesVec, tileArray, typeArray, layerArray, clip);
 
	int xOffset = 0;
	int yOffset = 0;
	int incr = 0;
	int row_incr = 0;
 
	while (quit == false) {
		while (SDL_PollEvent(&Event)) {
			switch(Event.type) {
				case SDL_QUIT:
					quit = true;
					break;
				case SDL_KEYDOWN:
					switch (Event.key.keysym.sym) {
						case SDLK_SPACE:
							if (!isFullscreen) {
								screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_OPENGL | SDL_FULLSCREEN);						
								isFullscreen = true;
							} else {
								screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_OPENGL | SDL_RESIZABLE);
							}
							break;

						case SDLK_ESCAPE:
							quit = true;
							break;
					}
			}
			for (int i = 0; i < ROOM_HEIGHT; ++i) {
				if (row_incr >= ROOM_HEIGHT) {
					break;
				} else {
					row_incr++;
				}
				for (int j = 0; j < ROOM_WIDTH; ++j) {
					//applySurface(xOffset, yOffset, tileset, screen, tilesVec[i+incr+j]->clip);
					//
					xOffset += TILE_WIDTH;
				}

				xOffset = 0;
				yOffset += TILE_HEIGHT;
				incr += (ROOM_WIDTH-1);
			}
			glClear(GL_COLOR_BUFFER_BIT);

			tileset.Draw();

			xOffset = yOffset = 0;
			SDL_GL_SwapBuffers();
		}
	}

	tilesVec.erase(tilesVec.begin(), tilesVec.end());

	SDL_FreeSurface(screen);

	SDL_Quit();

	exit(0);
}