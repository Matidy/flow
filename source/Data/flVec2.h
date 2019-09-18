#include <cstdint>

#ifndef FL_VEC2
#define FL_VEC2
template<typename T>
struct flVec2
{
	//standard 2D vector
	T x, y;

	flVec2()
		: x(0),
		  y(0)
	{}

	flVec2 (T _x, T _y)
		: x(_x),
		  y (_y)
	{}

	flVec2<T> operator+(T _operand) const
	{
		flVec2<T> result;
		result.x = x + _operand;
		result.y = y + _operand;

		return result;
	}
	flVec2<T> operator+(flVec2<T> _operand) const
	{
		flVec2<T> result;
		result.x = x + _operand.x;
		result.y = y + _operand.y;

		return result;
	}
	flVec2<T> operator-(T _operand) const
	{
		flVec2<T> result;
		result.x = x - _operand;
		result.y = y - _operand;

		return result;
	}
	flVec2<T> operator-(flVec2<T> _operand) const
	{
		flVec2<T> result;
		result.x = x - _operand.x;
		result.y = y - _operand.y;

		return result;
	}
	flVec2<T> operator*(T _operand) const
	{
		flVec2<T> result;
		result.x = x * _operand;
		result.y = y * _operand;

		return result;
	}
	flVec2<T> operator*(flVec2<T> _operand) const
	{
		flVec2<T> result;
		result.x = x * _operand.x;
		result.y = y * _operand.y;

		return result;
	}
	flVec2<T> operator/(T _operand) const
	{
		flVec2<T> result;
		result.x = x / _operand;
		result.y = y / _operand;

		return result;
	}
	flVec2<T> operator/(flVec2<T> _operand) const
	{
		flVec2<T> result;
		result.x = x / _operand.x;
		result.y = y / _operand.y;

		return result;
	}
};
#endif