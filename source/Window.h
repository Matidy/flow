#include <stdio.h>
#include <stdint.h>

struct SDL_Window;
struct SDL_Surface;
struct SDL_Renderer;
struct SDL_Point;

class Debug;
class WorldGrid;

class Window 
{
  public:
	//SDL data structures
	SDL_Window* m_gWindow;
	SDL_Surface* m_gScreenSurface;
	SDL_Surface* m_gImage;
	SDL_Renderer* m_gRenderer;

	//Window dimension constant
	static constexpr uint32_t WINDOW_WIDTH = 1024u;
	static constexpr uint32_t WINDOW_HEIGHT = (WINDOW_WIDTH/16)*9;

	Window(void);
	//intialise SDL
	bool Init();
	//close and free memory
	void Close();

	void Draw(WorldGrid const& _worldGrid
#if _DEBUG
		, Debug const& _debug
#endif
	);

  private:
	  //could replace this with a screen class for each specific render context? Not sure what the data spec
	  //for that would be right now though
	  enum class WindowDrawState
	  {
			WDS_Game
#if _DEBUG
			, WDS_DebugPropagation
			, WDS_DebugTessalation
#endif
	  };
	  WindowDrawState m_drawState;

	  //could use a switch draw context function to clear screen and update m_drawState here

	void DrawGame(WorldGrid const& _worldGrid);
	

#if _DEBUG
	// better way to do this might be to have window store an array of function pointers to
	// debug draw functions defined in other classes (so that the debug function are local to the
	// data they're drawing from)
	void DrawDebugPropagation(Debug const& _debug);
	void DrawDebugTessalation(Debug const& _debug);
#endif
};