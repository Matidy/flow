#include "Window.h"

#include <SDL.h>

#include "ValRGBA.h"


Window::Window(void) 
{
	m_gWindow = NULL;
	m_gScreenSurface = NULL;
	m_gImage = NULL;
	m_gRenderer = NULL;
}

bool Window::Init() 
{
	bool success = false;

	//Initialize SDL
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	else {
		//Create window
		m_gWindow = SDL_CreateWindow("1-col", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
		if(m_gWindow == NULL) {
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}
		else {
			//Create renderer for window
			m_gRenderer = SDL_CreateRenderer(m_gWindow, -1, SDL_RENDERER_ACCELERATED);
			if(m_gRenderer == NULL) {
				printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
				success = false;
			}
			else {
				m_gScreenSurface = SDL_GetWindowSurface(m_gWindow);
				success = true;
			}
		}
	}

	return success;
}

void Window::Close() 
{
	//Destroy window
	SDL_DestroyWindow(m_gWindow);
	m_gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}

void Window::Draw()
{
	SDL_Point points[3000];
	Uint32 i = 0;
	for (Uint32 x = 0; x <= Window::WINDOW_WIDTH;)
	{
		for (Uint32 y = 0; y <= Window::WINDOW_HEIGHT;)
		{
			Uint32 offset = 0;
			if (y/16 & 1) //checking for odd row through Least Sig Bit
			{
				offset = 8;
			}

			points[i].x = x+offset;
			points[i].y = y;

			i++;
			y += 16;
		}

		x += 16;
	}

	SDL_SetRenderDrawColor(m_gRenderer, 255, 255, 255, 255);
	//SDL_RenderDrawLines(m_gRenderer, points, i);
	SDL_RenderDrawPoints(m_gRenderer, points, i);

	/*SDL_RenderFillRect(m_gRenderer,
	SDL_RenderDrawRect(m_gRenderer,
	SDL_RenderDrawLines
	SDL_RenderDrawLine
	SDL_RenderDrawPoints
	SDL_RenderDrawPoint*/
}

//Window::DrawDebug