#include <cstdint>

#ifndef FL_VEC2
#define FL_VEC2
struct flVec2
{
	//standard 2D vector
	int32_t x, y;

	flVec2()
		:
		x(0),
		y(0)
	{}

	flVec2
	(
		int32_t _x, 
		int32_t _y
	)
		: 
		x(_x),
		y (_y)
	{}
};
#endif