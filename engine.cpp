#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <gl/GL.h>
#include <gl/GLU.h>
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

// Plane class
// This is the plane we'll draw the tileset on
// So eventually it'll need to be the size of the level
// Not the size of the screen.

class Plane {
public:
	Plane() : pSurface(NULL), x(0), y(0) {};
	SDL_Surface* Load(std::string _filename);
	void generateClips(GLenum _target, int _tileWidth, int _tileHeight, SDL_Rect* _clip, int _clipMax);
	std::vector<Tile*> generateTiles(int* _tileArray, std::string* _typeArray, short int* _layerArray, SDL_Rect* _clip);
	SDL_Surface* assembleMap(SDL_Surface* tileSurface, std::vector<Tile*> _tilesVec, SDL_Rect* _clip);
	void Draw(SDL_Surface* _blitSource, SDL_Surface* _blitDestination, SDL_Rect* clip);

private:
	int x, y;
	SDL_Surface* pSurface;
	GLuint texture[1];
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
	GLenum texture_format;
	SDL_Surface* loadedImage = IMG_Load(_filename.c_str());
	SDL_Surface* optimizedImage = NULL;

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

	//glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	/*glGenTextures(1, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	glTexImage2D(GL_TEXTURE_2D, 0, nOfColors, optimizedImage->w, optimizedImage->h, 0, texture_format, 
		GL_UNSIGNED_BYTE, optimizedImage->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/

	/*gluBuild2DMipmaps(GL_TEXTURE_2D, 3, optimizedImage->w, optimizedImage->h, texture_format, GL_UNSIGNED_BYTE,
		optimizedImage->pixels);*/

	return optimizedImage;
}

void Plane::generateClips(GLenum _target, int _tileWidth, int _tileHeight, SDL_Rect* _clip, int _clipMax) {
	GLint texWidth[1], texHeight[1]; 
	int Incr = 0, row = 0, test = 1;

	glGetTexLevelParameteriv(_target, 0, GL_TEXTURE_WIDTH, texWidth);
	glGetTexLevelParameteriv(_target, 0, GL_TEXTURE_HEIGHT, texHeight);

	for (GLint i = 0; i <= ROOM_HEIGHT*ROOM_WIDTH; ++i) {
		clip[test+Incr].x = ((test+Incr) * _tileWidth);
		clip[test+Incr].y = (row * _tileHeight);
		clip[test+Incr].w = _tileWidth;
		clip[test+Incr].h = _tileHeight;
		if (test >= ROOM_WIDTH) {
			row++;
			Incr+=test;
			test = 0;
		}	
		test++;
	}
}

std::vector<Tile*> Plane::generateTiles(int* _tileArray, std::string* _typeArray, short int* _layerArray, 
	SDL_Rect* _clip) {

		std::vector<Tile*> tempVec;

		Tile* tempTile;

		for (int i = 0; i < /*CLIP_MAX*/ROOM_HEIGHT*ROOM_WIDTH; ++i) {
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

SDL_Surface* Plane::assembleMap(SDL_Surface* tileSurface, std::vector<Tile*> _tilesVec, SDL_Rect* _clip)  {
	SDL_Surface* tempSurface = SDL_CreateRGBSurface(NULL, ROOM_WIDTH, ROOM_HEIGHT, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

	// Gonna try to use an iterator to get through the tiles, instead of copying info to dummy tile.
	// need to fix this, it is making a surface out of each clip, which is wrong.
	// this will just draw the last clip
	for (std::vector<Tile*>::const_iterator iter = _tilesVec.begin(); iter != _tilesVec.end(); ++iter) {
		for (int i = 0; i < ROOM_HEIGHT*ROOM_WIDTH; ++i) {
			//applySurface(x, y, tileSurface, );

		}
	}

	return tempSurface;
}

void Plane::Draw(SDL_Surface* _blitSource, SDL_Surface* _blitDestination, SDL_Rect* clip) {

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glGenTextures(1, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, _blitSource->w, _blitSource->h, 0, GL_BGRA, 
		GL_UNSIGNED_BYTE, _blitSource->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, _blitSource->w, _blitSource->h, GL_BGRA, GL_UNSIGNED_BYTE,
		_blitSource->pixels);
	
	applySurface(0, 0, _blitSource, _blitDestination, clip);

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

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

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
 

 
int main(int argc, char *argv[]) {
	Plane tileset = Plane();
	bool quit = false, isFullscreen = false;

 
	if ((SDL_Init(SDL_INIT_EVERYTHING)==-1)) 
		return true;

	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_OPENGL | SDL_RESIZABLE);

	if (init_GL() == false)
		return false;
 
	tileMap = tileset.Load("tileset16.png");


	tileset.generateClips(GL_TEXTURE_2D, TILE_WIDTH, TILE_HEIGHT, clip, CLIP_MAX);

	std::vector<Tile*> tilesVec = tileset.generateTiles(*tileArray, *typeArray, *layerArray, clip);

	//tileMap = tileset.assembleMap(tilesVec, clip);

	SDL_Surface* tempSurface = SDL_CreateRGBSurface(NULL, tileMap->w, tileMap->h, 32, tileMap->format->Rmask, tileMap->format->Gmask, tileMap->format->Bmask, tileMap->format->Amask);

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
					applySurface(xOffset, yOffset, tileMap, tempSurface, tilesVec[i+incr+j]->clip);
					xOffset += TILE_WIDTH;
				}
				xOffset = 0;
				yOffset += TILE_HEIGHT;
				incr += (ROOM_WIDTH-1);
			}

			glClear(GL_COLOR_BUFFER_BIT);

			//tileset.Draw(tileMap, screen, clip);
			tileset.Draw(tempSurface, screen, clip);

			xOffset = yOffset = 0;
			SDL_GL_SwapBuffers();
		}
	}

	tilesVec.erase(tilesVec.begin(), tilesVec.end());

	SDL_FreeSurface(tileMap);
	SDL_FreeSurface(screen);

	SDL_Quit();

	exit(0);
}