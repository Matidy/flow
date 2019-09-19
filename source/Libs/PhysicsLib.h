#pragma once
#include <cmath>
#include "../Data/flVec2.h"


////////////////////////////////////////////////////////////////////
namespace PhysicsLib
{
	static constexpr float PIf = 3.1415927f;
	static constexpr float TAUf = PIf*2.f;
	//C = TAU radians

	template <typename T>
	struct Vector2D
	{
		flVec2<T> m_startingPoint;
		flVec2<T> m_direction;
		T m_scalar;

		Vector2D()
			: m_startingPoint(flVec2<T>(0, 0)),
			  m_direction(flVec2<T>(1, 1)),
			  m_scalar(0)
		{}

		Vector2D(T _x1, T _y1, T _x2, T _y2)
			: m_startingPoint(flVec2<T>(_x1, _y1))
		{
			flVec2<T> endPoint(_x2, _y2);
			m_direction = endPoint - m_startingPoint;
			Normalise();
		}

		/* brief - constructor for polar coordinates
		 * params - _orientDeg - rotation starting from +x = 0 and rotating anti-clockwise
		 */
		Vector2D(flVec2<T> _startingPoint, T _orientDeg, T _extension)
			:
			m_startingPoint(_startingPoint),
			m_scalar(_extension)
		{
			//@bug - not correct
			T asRadians = _orientDeg * (360.f/TAUf);
			m_direction.x = cos(asRadians);
			m_direction.y = sin(asRadians);
			//no need to normalise as above equations to get x and y are for normalised hypotenuse
		}

		//@consider - normalisation doesn't really exist as a concept in grid space. More sensible option here might be to simplify the direction vector as much as possible
		void Normalise()
		{
			T hypoDist = sqrt(m_direction.x*m_direction.x + m_direction.y*m_direction.y);

			m_direction = m_direction/hypoDist;
			m_scalar = hypoDist;
		}

		flVec2<T> Translate(flVec2<T> _translation)
		{
			m_startingPoint = m_startingPoint + _translation;
			return m_startingPoint;
		}

		flVec2<T> CalcEndpoint() const
		{
			return m_startingPoint + m_direction*m_scalar;
		}

		flVec2<T> GetIntersectionPoint(Vector2D<T> const _otherVector) const
		{
			flVec2<T> intersectionPoint;

			//need to check for verticle vectors here as m_direction.x==0 in this case (avoid divide by 0 exception)

			T mA = m_direction.y/m_direction.x;
			T mB = _otherVector.m_direction.y/_otherVector.m_direction.x;
			T cA = m_startingPoint.y + (-m_startingPoint.x / m_direction.x)*m_direction.y;
			T cB = _otherVector.m_startingPoint.y + (-_otherVector.m_startingPoint.x / _otherVector.m_direction.x)*_otherVector.m_direction.y;

			//need to return null vec2 here if mA == mB (vectors are parallel so no intersection)

			intersectionPoint.x = (cB - cA) / (mA - mB);
			intersectionPoint.y = mA * intersectionPoint.x + cA;
			return intersectionPoint;
		}
	};
};

