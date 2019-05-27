#include <stdio.h>
#include <stdint.h>

struct SDL_Window;
struct SDL_Surface;
struct SDL_Renderer;
struct SDL_Point;

class Window 
{
  public:
	//SDL data structures
	SDL_Window* m_gWindow;
	SDL_Surface* m_gScreenSurface;
	SDL_Surface* m_gImage;
	SDL_Renderer* m_gRenderer;

	//Window dimension constant
	static const uint32_t WINDOW_WIDTH = 1024u;
	static const uint32_t WINDOW_HEIGHT = (WINDOW_WIDTH/16)*9;

	Window(void);
	//intialise SDL
	bool Init();
	//close and free memory
	void Close();

	void Draw();

  private:
};