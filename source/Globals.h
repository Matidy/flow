#include <cstdint>

#ifndef GLOBALS
#define GLOBALS

struct Globals
{
	//WorldGrid
	static constexpr uint32_t WORLD_X_SIZE = 1 << 7; //512
	static constexpr uint32_t WORLD_Y_SIZE = 1 << 7; //512
	static constexpr uint32_t TOTAL_WORLD_SIZE = WORLD_X_SIZE * WORLD_Y_SIZE; //262,144
	static constexpr uint32_t TILE_DRAW_DIMENSIONS = 1; //setting this to one for simplicity, and using floats to subdivide tiles to partially draw tiles

	//Window dimension constant
	static constexpr uint32_t WINDOW_WIDTH = 1024u;
	static constexpr uint32_t WINDOW_HEIGHT = (WINDOW_WIDTH / 16) * 9;
};
#endif
