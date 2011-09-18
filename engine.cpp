#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <SDL_endian.h>
#include <fstream>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <string>
#include <vector>
#include <cctype>

/* IDEAS still needing implemented:
	-Particle Effects - For lights n stuff
	-'Painting' different environments - Example: Painting with a grass brush on green tiles will make random
	grass sprites n stuff.
	%Alpha Transparency - For stuff above other layers n 
	- Read in approx 10 letters @ a time.
	- Have a vector of vectors for block maps and vector of vectors for layer maps
	- Keep them together in the file - aka layer1 block1 layer2 block2 etc.
	
	!!! Going to have to get rid of surfaces i think....
*/
 

// Constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;
 
const int TILE_WIDTH = 16;
const int TILE_HEIGHT = 16;

const int ROOM_WIDTH = SCREEN_WIDTH/TILE_WIDTH;
const int ROOM_HEIGHT = SCREEN_HEIGHT/TILE_HEIGHT;

const int CLIP_MAX = ROOM_WIDTH*ROOM_HEIGHT;

// SDL Shit
 
SDL_Surface* screen = NULL;
SDL_Surface* tileMap = NULL;
 
SDL_Event Event;
 
SDL_Rect clip[CLIP_MAX];

// need this to associate types w/ clips = Tiles!

struct Tile {
	SDL_Rect* clip;
	std::string type;
	int layer;
};

// Utilities

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
	void Load(std::string _filename);
	void Plane::generateClips(GLenum _target, int _tileWidth, int _tileHeight, SDL_Rect* _clip, int _clipMax);
	std::vector<Tile*> generateTiles(SDL_Rect* _clip);
	void generateVertices();
	void assembleMap(SDL_Surface* _Source, std::vector<Tile*> _tilesVec);
	void Draw(SDL_Surface* _blitSource, SDL_Surface* _blitDestination, SDL_Rect* clip);
	void Read(std::string _file);

private:
	int x, y;
	SDL_Surface* pSurface;
	GLuint tileTex[CLIP_MAX];
	GLuint tileset[1];
	GLenum texture_format;
	std::vector<Tile*> tilesVec;
	// vector of layer vectors
	std::vector<std::vector<int>*> layerVector2;
	// vector of block vectors
	std::vector<std::vector<char>*> blockVector2;
	
	std::vector<int> vertVec;
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

void Plane::Load(std::string _filename) {
	GLint nOfColors;
	SDL_Surface* loadedImage = IMG_Load(_filename.c_str()), *optimizedImage = NULL;
	Uint32 colorkey = 0, pixel = 0;

	if (loadedImage != NULL) {
		optimizedImage = SDL_CreateRGBSurface(NULL, nextPowerOfTwo(loadedImage->w), nextPowerOfTwo(loadedImage->h), 32, loadedImage->format->Rmask, loadedImage->format->Gmask, loadedImage->format->Bmask, loadedImage->format->Amask);

		SDL_BlitSurface(loadedImage, NULL, optimizedImage, NULL);
		SDL_FreeSurface(loadedImage);
	}
 
	if (optimizedImage != NULL) {
		if (SDL_MUSTLOCK(optimizedImage)) {
			SDL_LockSurface(optimizedImage);
		}
		colorkey = SDL_MapRGB(optimizedImage->format, 255, 0, 255);

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

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glGenTextures(1, &tileset[0]);
	glBindTexture(GL_TEXTURE_2D, tileset[0]);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, optimizedImage->w, optimizedImage->h, 0, texture_format, GL_UNSIGNED_BYTE, optimizedImage->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	SDL_FreeSurface(optimizedImage);
}

void Plane::generateVertices() {
	vertVec.reserve((ROOM_WIDTH*2)*(ROOM_HEIGHT*2));
	int xPos = 0, yPos = 0;
	for (int i = 0; i < vertVec.size(); ++i) {
		if (xPos < ROOM_WIDTH) {
			// Only need to build 2 vertices per iteration here, v0 = top left, v1 = bottom left
			vertVec.push_back(xPos+yPos); vertVec.push_back(xPos+yPos); // v0
			vertVec.push_back(xPos+yPos); vertVec.push_back((xPos+yPos)*TILE_HEIGHT); // v1
			xPos += TILE_WIDTH;
		} else {
			xPos = 0;
			yPos += TILE_HEIGHT;
		}
	}
}

void Plane::generateClips(GLenum _target, int _tileWidth, int _tileHeight, SDL_Rect* _clip, int _clipMax) {
	GLint texWidth[1], texHeight[1]; 

	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, texWidth);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, texHeight);

	int Incr = 0, row = 0, xPos = 0, testRow = 0;
	int arrayYTest = 0;
	int arrayTest = 0;
	int derp = 0;
	int j = 0;

	for (GLint i = 0; i < ROOM_HEIGHT*ROOM_WIDTH; ++i) {
		derp = xPos+Incr;
		if (j < layerVector2.size()) {
			arrayTest = layerVector2[j]->at(derp);
		} else {
			j++;
		}
		clip[derp].x = (arrayTest * _tileWidth);
		clip[derp].y = (arrayYTest * _tileHeight);
		clip[derp].w = _tileWidth;
		clip[derp].h = _tileHeight;
		// this probably wont work right..but i'll get to it later! :D
		// Why can't i just do this in the xPos test below?
		if ((arrayTest * _tileWidth) >= *texWidth) {
			//arrayYTest = layerVector2[++testRow][xPos];
		}
		if (xPos >= ROOM_WIDTH) {
			row++;
			Incr+=xPos;
			xPos = 1;
			//arrayYTest = tileArray3[++testRow][xPos];
		} else {
			xPos++;
		}
	}
}

// Assemble Map - This will put the tiles/clips onto an RGB Surface, and queue it for blit.

void Plane::assembleMap(SDL_Surface* _Source, std::vector<Tile*> _tilesVec)  {
	/*SDL_Surface* tempSurface = SDL_CreateRGBSurface(NULL, ROOM_WIDTH*TILE_WIDTH, ROOM_HEIGHT*TILE_HEIGHT, 32, _Source->format->Rmask, _Source->format->Gmask, _Source->format->Bmask, _Source->format->Amask);*/

	int Incr = 0, row = 0, xPos = 0;
	
	for (GLint i = 0; i < ROOM_HEIGHT*ROOM_WIDTH; ++i) {
		//applySurface(xPos * _tilesVec[i]->clip->w, row * _tilesVec[i]->clip->h, _Source, tempSurface, _tilesVec[i]->clip);
		// A texture per quad
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glGenTextures(1, &tileTex[i]);
		glBindTexture(GL_TEXTURE_2D, tileTex[i]);

		glTexImage2D(GL_TEXTURE_2D, 0, 4, TILE_WIDTH, TILE_HEIGHT, 0, texture_format, GL_UNSIGNED_BYTE, tileset);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		if (xPos >= (ROOM_WIDTH-1)) {
			row++;
			Incr+=xPos;
			xPos = 0;
		} else {
			xPos++;
		}
	}
}

std::vector<Tile*> Plane::generateTiles(SDL_Rect* _clip) {
	std::vector<Tile*> tempVec;

	Tile* tempTile;

	int j = 0;

	for (int i = 0; i < ROOM_HEIGHT*ROOM_WIDTH; ++i) {
		tempTile = new Tile;

		tempTile->clip = &_clip[i];
		if (j < layerVector2.size()) {
			tempTile->type = blockVector2[j]->at(i);
			tempTile->layer = layerVector2[j]->at(i);
		} else {
			j++;
		}

		tempVec.push_back(tempTile);

	}
	tempTile = NULL;
	delete tempTile;

	return tempVec;
}



void Plane::Draw(SDL_Surface* _blitSource, SDL_Surface* _blitDestination, SDL_Rect* clip) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	applySurface(0, 0, _blitSource, _blitDestination, NULL);

	//Offset
	glTranslatef(x, y, 0);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// Build
	/*glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2i(0, 0);
		glTexCoord2f(1, 0); glVertex2i(ROOM_WIDTH*TILE_WIDTH, 0);
		glTexCoord2f(1, 1); glVertex2i(ROOM_WIDTH*TILE_WIDTH, ROOM_HEIGHT*TILE_HEIGHT);
		glTexCoord2f(0, 1); glVertex2i(0, ROOM_HEIGHT*TILE_HEIGHT);
	glEnd();*/
	//glBegin(GL_TRIANGLE_STRIP);
	glDrawElements(GL_TRIANGLE_STRIP, (ROOM_WIDTH*2)*(ROOM_HEIGHT*2), GL_UNSIGNED_BYTE, &vertVec);

	//Reset
	glLoadIdentity();
}

bool init_GL() {
	glClearColor(0, 0, 0, 0);
	GLfloat light_position[] = {.2, 1, 1, 0};
	GLfloat light_ambient[] = {0, 0, 0, -1};

	glShadeModel(GL_SMOOTH);

	//glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	//glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);

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
 
void Plane::Read(std::string _file) {
	std::vector<char>* testblock;
	std::vector<int>* testlayer;
	std::ifstream derpstream;
	std::string derpinput;
	char test;
	derpstream.open(_file);

	// % = new layer array
	// & = new block array
	
	if (derpstream.is_open()) {
		derpstream.seekg(0, std::ios::beg);
		while (std::getline(derpstream, derpinput)) {
			for (int i = 0; i < derpinput.length(); ++i) {
				if (isalnum(derpinput[i])) {
					if (isalpha(derpinput[i])) {
						testblock->push_back(derpinput[i]);
					} else {
						// it's a number
						testlayer->push_back((derpinput[i] - '0'));
					}
				} else {
					test = derpinput[i];
					if (test == '%') {
						// new layer vector
						// Meh just gonna try this, would be nice to pass the address as we wont need the actual name
						testlayer = new std::vector<int>;
						layerVector2.push_back(testlayer);
					}  else if (test == '&') {
						// new block vector
						testblock = new std::vector<char>;
						blockVector2.push_back(testblock);
					}
				}
			}
		}
	}
	derpstream.close();
}

int main(int argc, char *argv[]) {
	Plane tileset = Plane();
	bool quit = false, isFullscreen = false;
 
	std::vector<std::string> testType;

	if ((SDL_Init(SDL_INIT_EVERYTHING)==-1)) 
		return true;

	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_OPENGL | SDL_RESIZABLE | SDL_DOUBLEBUF);

	if (init_GL() == false)
		return false;
 
	tileset.Read("test2.level");

	/*tileMap = */tileset.Load("tileset16.png");

	tileset.generateClips(GL_TEXTURE_2D, TILE_WIDTH, TILE_HEIGHT, clip, CLIP_MAX);

	std::vector<Tile*> tilesVec = tileset.generateTiles(clip);
	
	/*tileMap = */tileset.assembleMap(tileMap, tilesVec);
 
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