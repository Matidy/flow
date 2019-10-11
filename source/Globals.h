#include <cstdint>

#ifndef GLOBALS
#define GLOBALS

namespace Globals
{
	//flSpace/flEnergy
	constexpr uint32_t NULL_INDEX = 0xffffffff;
	constexpr uint32_t WALL_INDEX = 0xfffffffe;

	//WorldGrid
	constexpr uint32_t WORLD_X_SIZE = 1 << 7; //128
	constexpr uint32_t WORLD_Y_SIZE = 1 << 7; //128
	constexpr uint32_t TOTAL_WORLD_SIZE = WORLD_X_SIZE * WORLD_Y_SIZE; //16,384
	constexpr uint32_t TILE_DRAW_DIMENSIONS = 1; //setting this to one for simplicity, and using floats to subdivide tiles to partially draw tiles

	//Window dimension constant
	constexpr uint32_t WINDOW_WIDTH_ASPECT  = 16u;
	constexpr uint32_t WINDOW_HEIGHT_ASPECT = 9u;
	constexpr uint32_t WINDOW_SIZE_SCALAR   = 64u; //1024x576
	constexpr float	   WINDOW_ASPECT_RATIO  = static_cast<float>(WINDOW_HEIGHT_ASPECT) / static_cast<float>(WINDOW_WIDTH_ASPECT);
	constexpr uint32_t WINDOW_WIDTH  = WINDOW_WIDTH_ASPECT  * WINDOW_SIZE_SCALAR;
	constexpr uint32_t WINDOW_HEIGHT = WINDOW_HEIGHT_ASPECT * WINDOW_SIZE_SCALAR;
};
#endif
