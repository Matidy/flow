#include <iostream>
#include <tuple>

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


constexpr Uint32 FRAME_RATE_CAP = 120u; //comes out at 125 due to rounding
constexpr Uint32 MINIMUM_FRAME_DURATION = 1000/FRAME_RATE_CAP;

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
				//structured bindings (C++17 feature)
				auto CreateIndexedFloat = []() -> std::tuple<int, float>
				{
					return { 3, 5.555f };
				};
				auto[index, indexedFloat] = CreateIndexedFloat();
				std::cout << index << ", " << indexedFloat << std::endl;

				//float precision
				int32_t minus = -1;
				uint32_t not_minus = static_cast<uint32_t> (minus);

				float a1 =  1.f / 128.f;  //0.00781250000
				float a2 =  2.f / 128.f;  //0.0156250000
				float a3 =  3.f / 128.f;  //0.0234375000
				float a4 =  4.f / 128.f;  //0.0312500000
				float a8 =  8.f / 128.f;  //0.0625000000
				float a16 = 16.f / 128.f; //0.125000000
				float a79 = 79.f / 128.f; //0.617187500

				float b1 =  1.f / 32.f;  //0.0312500000
				float b2 =  2.f / 32.f;  //0.0625000000
				float b3 =  3.f / 32.f;  //0.0937500000
				float b4 =  4.f / 32.f;  //0.125000000
				float b8 =  8.f / 32.f;  //0.250000000
				float b16 = 16.f / 32.f; //0.500000000
				float b31 = 31.f / 32.f; //0.968750000

				float c1 =  1.f / 12.f;  //0.0833333358
				float c2 =  2.f / 12.f;  //0.166666672
				float c3 =  3.f / 12.f;  //0.250000000
				float c4 =  4.f / 12.f;  //0.333333343
				float c8 =  8.f / 12.f;  //0.666666687
				float c9 =  9.f / 12.f;  //0.750000000

				//virtual destructors
				class BaseA
				{
				public:
					BaseA() {}
					virtual ~BaseA() { std::cout << "Destructing BaseA" << std::endl; }
				};
				class BaseB
				{
				public:
					BaseB() {}
					virtual ~BaseB() { std::cout << "Destructing BaseB" << std::endl; }
				};
				class Derived :
					public BaseA,
					public BaseB
				{
				public:
					Derived() {}
					~Derived() override { std::cout << "Destructing Derived" << std::endl; }
				};

				Derived* deri = new Derived;
				BaseA* base = deri;

				delete base;
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
				timeLastFrame = currentTime;
				currentTime = SDL_GetTicks();
				deltaTime = currentTime - timeLastFrame;
				// Frame Rate Cap //
				if (deltaTime < MINIMUM_FRAME_DURATION)
				{
					SDL_Delay(MINIMUM_FRAME_DURATION - deltaTime);
					deltaTime = MINIMUM_FRAME_DURATION;
				}

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
#ifdef _DEBUG
					if (inputCore.InputBatchEnabled())
					{
						inputCore.BatchProcessInputs(e);
					}
					else
					{
#endif
						inputCore.UpdateKeyboardState(e);
#ifdef _DEBUG
					}
#endif

					inputCore.CheckMouseInput(e);
					inputCore.CheckPressedKeyboardInput(e);
				}
				if (quit == true)
				{
					break;
				}

				inputCore.CheckHeldKeyboardInput(deltaTime);

				////////////////////////////////////////////////////////////////////
				//		World Sim		//
				////////////////////////////
				gameData.UpdateStep(deltaTime);

				////////////////////////////////////////////////////////////////////
				//		Render		 //
				/////////////////////////
				renderCore.DrawUpdate(deltaTime);

				SDL_RenderPresent(renderCore.m_gRenderer);

				SDL_SetRenderDrawColor(renderCore.m_gRenderer, 92, 92, 92, 255);
				SDL_RenderClear(renderCore.m_gRenderer); //reset frame to blank black screen to avoid back buffer data persisting across frames.
			}
		}
	}
	
	//game closed - shutdown
	SDL_Delay(100);
	Main::ShutdownLibraries();
	return 0;
}

