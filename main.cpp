#include <SDL_timer.h>
#include <SDL_events.h>
#include <SDL_render.h>

#include "source/Window.h"
#include "source/WorldGrid.h"
#include "source/Input.h"

#if _DEBUG
#include "source/Debug.h"
#endif

const Uint32 FRAME_RATE = 60u; //Currently frame rate = 2*FRAME_RATE
const Uint32 MINIMUM_FRAME_DURATION = 1000/FRAME_RATE;

int main(int argc, char* args[]) 
{
#if _DEBUG 
	Debug debug;
#endif
	Window window;
	if(!window.Init()) 
	{
		printf("Failed to initialise SDL and/or game window.\n");
	}
	else 
	{
		WorldGrid worldGrid;
		if (!worldGrid.Init())
		{
			printf("Failed to init WorldGrid. \n");
		}
		else
		{
			Input input;
			if (!input.Init(&worldGrid
#if _DEBUG					
				, &debug
#endif
				))
			{
				printf("Failed to init Input. \n");
			}
			else
			{
				bool quit = false;
				Uint32 startTime = SDL_GetTicks();
				Uint32 currentTime = startTime;
				Uint32 timeLastFrame;
				Uint32 timeBetweenFrames;

				// init world state //
				worldGrid.GenerateWorld();
				//////////////////////////////////////////////////////
					//					Main Loop				//	
				//////////////////////////////////////////////////////
				while (!quit)
				{
					// Frame Rate Cap //
					timeLastFrame = currentTime;
					currentTime = SDL_GetTicks();
					timeBetweenFrames = currentTime - timeLastFrame;
					if (timeBetweenFrames <= MINIMUM_FRAME_DURATION)
						SDL_Delay(MINIMUM_FRAME_DURATION - timeBetweenFrames);

					//////////////////////////////
					//		Input		//
					/////////////////////////
					SDL_Event e;
					while (SDL_PollEvent(&e) != 0)
					{
						if (e.type == SDL_QUIT)
						{
							quit = true;
							break;
						}
						else
						{
							input.UpdateKeyboardState(e);
						}
					}
					if (quit == true)
					{
						break;
					}

					input.HandleInput();
						
					////////////////////////////////
					//		World Sim		//
					////////////////////////////
					worldGrid.UpdateStep(timeBetweenFrames);

					//////////////////////////////
					//		Render		 //
					/////////////////////////
					window.Draw();
					SDL_RenderPresent(window.m_gRenderer);
				}
			}
		}
	}
	
	window.Close();
	SDL_Delay(100);
	return 0;
}