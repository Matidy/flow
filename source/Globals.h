#include <cstdint>

#ifndef GLOBALS
#define GLOBALS

struct Globals
{
	//WorldGrid
	static constexpr uint32_t WORLD_X_SIZE = 1 << 12; //4096
	static constexpr uint32_t WORLD_Y_SIZE = 1 << 12; //4096
	static constexpr uint32_t TOTAL_WORLD_SIZE = WORLD_X_SIZE * WORLD_Y_SIZE; //16,777,216

	//Window dimension constant
	static constexpr uint32_t WINDOW_WIDTH = 1024u;
	static constexpr uint32_t WINDOW_HEIGHT = (WINDOW_WIDTH / 16) * 9;
};
#endif
