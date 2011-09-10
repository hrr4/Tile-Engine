#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <SDL_endian.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <string>
#include <vector>

/* IDEAS still needing implemented:
	-Particle Effects - For lights n stuff
	-'Painting' different environments - Example: Painting with a grass brush on green tiles will make random
	grass sprites n stuff.
	%Alpha Transparency - For stuff above other layers n such
*/
 

// Constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;
 
const int ROOM_WIDTH = 10;
const int ROOM_HEIGHT = 7;
 
const int TILE_WIDTH = 16;
const int TILE_HEIGHT = 16;
 
const int CLIP_MAX = 600;

// SDL Shit
 
SDL_Surface* screen = NULL;
SDL_Surface* tileMap = NULL;
 
SDL_Event Event;
 
SDL_Rect clip[CLIP_MAX];

GLuint texture2[1];

int tileArray[ROOM_HEIGHT][ROOM_WIDTH] = {
	{0,0,0,0,0,0,0,0,0,0},
	{0,0,1,1,1,1,1,1,0,0},
	{0,1,2,2,2,2,2,2,1,0},
	{0,1,2,2,2,2,2,2,1,0},
	{0,1,2,2,2,2,2,2,1,0},
	{0,0,1,1,1,1,1,1,0,0},
	{0,0,0,0,0,0,0,0,0,0}
};
 
// u = unblock, b = block, w = water.
// You can mix these together
 
std::string typeArray[ROOM_HEIGHT][ROOM_WIDTH] = {
	{"b", "b", "b", "b", "b", "b", "b", "b", "b", "b"},
	{"b", "b", "u", "u", "u", "u", "u", "u", "b", "b"},
	{"b", "b", "uw", "uw", "uw", "uw", "uw", "uw", "b", "b"},
	{"b", "b", "u", "u", "u", "u", "u", "u", "b", "b"},
	{"b", "b", "u", "u", "u", "u", "u", "u", "b", "b"},
	{"b", "b", "u", "u", "u", "u", "u", "u", "b", "b"},
	{"b", "b", "b", "b", "b", "b", "b", "b", "b", "b"}
};

// 2 will be the default layer

short signed int layerArray[ROOM_HEIGHT][ROOM_WIDTH] = {
	{1,1,1,1,1,1,1,1,1,1},
	{0,0,1,1,1,1,1,1,0,0},
	{0,1,2,2,2,2,2,2,1,0},
	{0,1,2,2,2,2,2,2,1,0},
	{0,1,2,2,2,2,2,2,1,0},
	{0,0,1,1,1,1,1,1,0,0},
	{0,0,0,0,0,0,0,0,0,0}
};

// need this to associate types w/ clips = Tiles!

struct Tile {
	SDL_Rect* clip;
	std::string type;
	int layer;
};

void applySurface(int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip = NULL) {
	SDL_Rect offset;
 
	offset.x = x;
	offset.y = y;
 
	SDL_BlitSurface(source, clip, destination, &offset);
}

Uint32 getPixel32(SDL_Surface* _source, int _x, int _y) {
	Uint32* pixels = (Uint32 *)_source->pixels;

	return pixels[(_y*_source->w) + _x];
}

void putPixel32(SDL_Surface* _source, int x, int y, Uint32 _pixel) {
	Uint32* pixels = (Uint32 *)_source->pixels;

	pixels[(y*_source->w) + x] = _pixel;
}

// Plane class
// This is the plane we'll draw the tileset on
// So eventually it'll need to be the size of the level
// Not the size of the screen.

class Plane {
public:
	Plane() : pSurface(NULL), x(0), y(0) {};
	SDL_Surface* Load(std::string _filename);
	void generateClips(SDL_Surface* _Source, int _tileWidth, int _tileHeight, SDL_Rect* _clip, int _clipMax);
	std::vector<Tile*> generateTiles(int* _tileArray, std::string* _typeArray, short int* _layerArray, SDL_Rect* _clip);
	SDL_Surface* assembleMap(SDL_Surface* _Source, std::vector<Tile*> _tilesVec);
	void Draw(SDL_Surface* _blitSource, SDL_Surface* _blitDestination, SDL_Rect* clip);

private:
	int x, y;
	SDL_Surface* pSurface;
	GLuint texture[1];
	GLenum texture_format;
	std::vector<Tile*> tilesVec;

	int nextPowerOfTwo(int _num);
};

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
	SDL_Surface* loadedImage = IMG_Load(_filename.c_str());
	SDL_Surface* optimizedImage = NULL;

	if (loadedImage != NULL) {
		optimizedImage = SDL_CreateRGBSurface(NULL, nextPowerOfTwo(loadedImage->w), 
			nextPowerOfTwo(loadedImage->h), 32, loadedImage->format->Rmask,
			loadedImage->format->Gmask, loadedImage->format->Bmask, loadedImage->format->Amask);

		SDL_BlitSurface(loadedImage, NULL, optimizedImage, NULL);
		SDL_FreeSurface(loadedImage);
	}
 
	if (optimizedImage != NULL) {
		if (SDL_MUSTLOCK(optimizedImage)) {
			SDL_LockSurface(optimizedImage);
		}
		Uint32 colorkey = SDL_MapRGB(optimizedImage->format, 255, 0, 255);
		Uint32 pixel = 0;

		for (int i = 0; i < optimizedImage->h; ++i) {
			for (int j = 0; j < optimizedImage->w; ++j) {
				pixel = getPixel32(optimizedImage, j, i);

				if (pixel == colorkey) {
					pixel = SDL_MapRGBA(optimizedImage->format, 0, 0, 0, 0);
					putPixel32(optimizedImage, j, i, pixel);
				}
			}
		}

		if (SDL_MUSTLOCK(optimizedImage)) {
			SDL_UnlockSurface(optimizedImage);
		}

		SDL_ConvertSurface(optimizedImage, optimizedImage->format, SDL_HWSURFACE | SDL_RLEACCEL);
	}

	nOfColors = optimizedImage->format->BytesPerPixel;

	if (nOfColors == 4) {
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			texture_format = GL_RGBA;
		} else {
			texture_format = GL_BGRA;
		}
	} else if (nOfColors == 3) {
		if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
			texture_format = GL_RGB;
		} else {
			texture_format = GL_BGR;
		}
	}

	return optimizedImage;
}

void Plane::generateClips(SDL_Surface* _Source, int _tileWidth, int _tileHeight, SDL_Rect* _clip, int _clipMax) {
	int Incr = 0, row = 0, xPos = 0;
	int maxWidth = _Source->w, testRow = 0;

	for (GLint i = 0; i <= ROOM_HEIGHT*ROOM_WIDTH; ++i) {
		int arrayTest = tileArray[row][xPos];
		int arrayYTest = 0;
		clip[xPos+Incr].x = (arrayTest * _tileWidth);
		clip[xPos+Incr].y = (arrayYTest * _tileHeight);
		clip[xPos+Incr].w = _tileWidth;
		clip[xPos+Incr].h = _tileHeight;
		// this probably wont work right..but i'll get to it later! :D
		if ((arrayTest * _tileWidth) >= maxWidth) {
			arrayYTest = tileArray[++testRow][xPos];
		}
		if (xPos >= ROOM_WIDTH) {
			row++;
			Incr+=xPos;
			xPos = 1;
		} else {
			xPos++;
		}
	}
}

std::vector<Tile*> Plane::generateTiles(int* _tileArray, std::string* _typeArray, short int* _layerArray, SDL_Rect* _clip) {
	std::vector<Tile*> tempVec;

	Tile* tempTile;

	for (int i = 0; i < ROOM_HEIGHT*ROOM_WIDTH; ++i) {
		tempTile = new Tile;

		tempTile->clip = &_clip[i];
		tempTile->type = _typeArray[i];
		tempTile->layer = _layerArray[i];

		tempVec.push_back(tempTile);

	}
	tempTile = NULL;
	delete tempTile;

	return tempVec;
}

// Assemble Map - This will put the tiles/clips onto an RGB Surface, and queue it for blit.

SDL_Surface* Plane::assembleMap(SDL_Surface* _Source, std::vector<Tile*> _tilesVec)  {
	SDL_Surface* tempSurface = SDL_CreateRGBSurface(NULL, _Source->w, _Source->h, 32, _Source->format->Rmask, _Source->format->Gmask, _Source->format->Bmask, _Source->format->Amask);

	int Incr = 0, row = 0, xPos = 0;
	
	for (GLint i = 0; i < ROOM_HEIGHT*ROOM_WIDTH; ++i) {
		applySurface(xPos * _tilesVec[i]->clip->w, row * _tilesVec[i]->clip->h, _Source, tempSurface, _tilesVec[i]->clip);
		if (xPos >= (ROOM_WIDTH-1)) {
			row++;
			Incr+=xPos;
			xPos = 0;
		}	else {
			xPos++;
		}
	}

	SDL_SaveBMP(tempSurface, "tempsurface.bmp");
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glGenTextures(1, &texture2[0]);
	glBindTexture(GL_TEXTURE_2D, texture2[0]);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, tempSurface->w, tempSurface->h, 0, texture_format, GL_UNSIGNED_BYTE, tempSurface->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return tempSurface;
}

void Plane::Draw(SDL_Surface* _blitSource, SDL_Surface* _blitDestination, SDL_Rect* clip) {
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);

	applySurface(0, 0, _blitSource, _blitDestination, NULL);

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

	//Reset
	glLoadIdentity();
}

bool init_GL() {
	glClearColor(0, 0, 0, 0);
	glEnable(GL_TEXTURE_2D);

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);

	// Model View
	glMatrixMode(GL_MODELVIEW);

	// Errors
	if (glGetError() != GL_NO_ERROR)
		return false;
	return true;
}
 

int main(int argc, char *argv[]) {
	Plane tileset = Plane();
	bool quit = false, isFullscreen = false;

 
	if ((SDL_Init(SDL_INIT_EVERYTHING)==-1)) 
		return true;

	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_OPENGL | SDL_RESIZABLE | SDL_DOUBLEBUF);

	if (init_GL() == false)
		return false;
 
	tileMap = tileset.Load("tileset16.png");

	tileset.generateClips(tileMap, TILE_WIDTH, TILE_HEIGHT, clip, CLIP_MAX);

	std::vector<Tile*> tilesVec = tileset.generateTiles(*tileArray, *typeArray, *layerArray, clip);
	
	tileMap = tileset.assembleMap(tileMap, tilesVec);
 
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
								screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_OPENGL | SDL_FULLSCREEN | SDL_DOUBLEBUF);						
								isFullscreen = true;
							} else {
								screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_OPENGL | SDL_RESIZABLE | SDL_DOUBLEBUF);
							}
							break;

						case SDLK_ESCAPE:
							quit = true;
							break;
					}
			}

			tileset.Draw(tileMap, screen, clip);

			SDL_GL_SwapBuffers();
		}
	}

	tilesVec.erase(tilesVec.begin(), tilesVec.end());

	SDL_FreeSurface(tileMap);
	SDL_FreeSurface(screen);

	glDisable(GL_TEXTURE_2D);
	SDL_Quit();

	exit(0);
}