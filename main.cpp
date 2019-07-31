#include <SDL.h>
/* following includes are redundant as SDL.h include all SDL headers. Kept for now as like explictness in which
   parts of SDL are currently being used.
*/
#include <SDL_timer.h>
#include <SDL_events.h>
#include <SDL_render.h>
/* https://stackoverflow.com/questions/21392755/difference-between-surface-and-texture-sdl-general
   Difference between SDL_Surface and SDLTexture.
*/

#include <SDL_ttf.h>
/* https://stackoverflow.com/questions/27331819/whats-the-difference-between-a-character-a-code-point-a-glyph-and-a-grapheme
   Useful definitions of Character, code point, code unit, grapheme and glyph for working with ttf/Unicode.
*/

#include "source/RenderCore.h"
#include "source/InputCore.h"
#include "source/GameData.h"


constexpr Uint32 FRAME_RATE = 60u; //Currently frame rate = 2*FRAME_RATE for some reason.
constexpr Uint32 MINIMUM_FRAME_DURATION = 1000/FRAME_RATE;

namespace Main
{
	bool InitLibraries()
	{
		bool success = true;
		if (SDL_Init(	SDL_INIT_VIDEO
					 && SDL_INIT_EVENTS ) < 0) {
			printf("SDL could not initialise! SDL_Error: %s\n", SDL_GetError());

			success = false;
		}
		if (TTF_Init() < 0)
		{
			printf("SDL_ttf could not initialise! TTF_Error: %s\n", TTF_GetError());

			success = false;
		}

		return success;
	}

	void ShutdownLibraries()
	{
		SDL_Quit();
		TTF_Quit();
	}
}

int main(int argc, char* args[]) 
{
	if (!Main::InitLibraries())
	{
		printf("Failed in initialise Library dependencies.");
	}
	else
	{
		RenderCore renderCore;
		InputCore inputCore;
		if (!renderCore.Init())
		{
			printf("Failed to initialise SDL and/or game window.\n");
		}
		else
		{
			GameData gameData(renderCore, inputCore);

			//test code section
			{
				/*
				char const * stringA = (char *)"Test String";
				char stringB[18] = "You what my child";
				char *stringC = (char*)"Ey up";

				stringB = stringC;

				int32_t d = 1;
				*/
			}

			bool quit = false;
			Uint32 startTime = SDL_GetTicks();
			Uint32 currentTime = startTime;
				
			//////////////////////////////////////////////////////
				//					Main Loop				//	
			//////////////////////////////////////////////////////
			Uint32 timeLastFrame;
			Uint32 deltaTime;
			while (!quit)
			{
				// Frame Rate Cap //
				timeLastFrame = currentTime;
				currentTime = SDL_GetTicks();
				deltaTime = currentTime - timeLastFrame;
				if (deltaTime <= MINIMUM_FRAME_DURATION)
					SDL_Delay(MINIMUM_FRAME_DURATION - deltaTime);

				////////////////////////////////////////////////////////////////////
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
#if _DEBUG
						if (inputCore.InputBatchEnabled())
						{
							inputCore.BatchProcessInputs(e);
						}
						else
						{
#endif
							inputCore.UpdateKeyboardState(e);
#if _DEBUG
						}
#endif
					}
				}
				if (quit == true)
				{
					break;
				}

				inputCore.CheckHeldKeyboardInput();

				////////////////////////////////////////////////////////////////////
				//		World Sim		//
				////////////////////////////
				gameData.UpdateStep(deltaTime);

				////////////////////////////////////////////////////////////////////
				//		Render		 //
				/////////////////////////
				renderCore.DrawUpdate(gameData, deltaTime);

				SDL_RenderPresent(renderCore.m_gRenderer);

				SDL_SetRenderDrawColor(renderCore.m_gRenderer, 0, 0, 0, 255);
				SDL_RenderClear(renderCore.m_gRenderer); //reset frame to blank black screen to avoid back buffer data persisting across frames.
			}
		}
	}
	
	//game closed - shutdown
	SDL_Delay(100);
	Main::ShutdownLibraries();
	return 0;
}

