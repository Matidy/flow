#include <vector>
#include <cmath>
#include <SDL_scancode.h>

#include "Globals.h"
#include "Data/flPoint.h"
#include "Data/flVec2.h"
#include "RenderDelegate.h"
#include "InputDelegate.h"

struct RenderCoreIF;
struct InputCoreIF;

class Debug :
	public RenderDelegate,
	public InputDelegate
{
public:
	//from RenderDelegate
	virtual void DelegateDraw(SDL_Renderer * const _gRenderer) const override final;

	//from InputDelegate
	virtual void DefineChordInput(uint32_t _timeStep) override final;
	virtual void MouseMovementInput(flVec2<int> _mousePos) override final;
	virtual void MouseDownInput(eMouseButtonType const _buttonType, flVec2<int32_t> _mousePos) override final;
	virtual void MouseUpInput(eMouseButtonType const _buttonType, flVec2<int32_t> _mousePos) override final;
	virtual void DefineHeldInput(uint32_t _timeStep) override final;
	virtual void KeyPressedInput(SDL_Scancode const& _key) override final;
	virtual void KeyReleasedInput(SDL_Scancode const& _key) override final;

	//world size in points/tiles
	static constexpr uint32_t m_debugWorldDim = 17u;
	static constexpr uint32_t m_debugWorldSize = m_debugWorldDim * m_debugWorldDim;
	static constexpr uint32_t m_tilesDimPixels = Globals::WINDOW_HEIGHT/m_debugWorldDim;

	Debug(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCoreIF);
	~Debug();
	void UpdateStep(uint32_t const _timeStep);

	bool const& ToggleDebug();
	bool const DebugEnabled() const;

private:
	//// data ///////////////////////////////////////////////////////////////////////////////
	enum PropagationMode
	{
		SimpleDirectional,
		SemiCircleDirectional,

		CountPropagationMode
	};
	uint32_t m_propagationMode;

	enum class DrawMode
	{
		Propagation,
		Tessalation
	};
	DrawMode m_drawMode;

	static constexpr uint32_t m_nullIndex = ~0;

	flPoint* m_debugWorld;
	int32_t  m_tileUnderMouseIndex = -1;
	bool m_debugEnabled;

	template <typename T>
	struct Vector2D
	{
		flVec2<T> m_startingPoint;
		flVec2<T> m_endPoint;
		flVec2<T> m_direction;
		T m_scalar;

		Vector2D()
			: m_startingPoint(flVec2<T>(0, 0)),
			  m_endPoint(flVec2<T>(0, 0)),
			  m_direction(flVec2<T>(1, 1)),
			  m_scalar(0)
		{}

		Vector2D(T _x1, T _y1, T _x2, T _y2)
			: m_startingPoint(flVec2<T>(_x1, _y1)),
			  m_endPoint(flVec2<T>(_x2, _y2))
		{
			m_direction = m_endPoint - m_startingPoint;
			Normalise();
		}

		//@consider - normalisation doesn't really exist as a concept in grid space. More sensible option here might be to simplify the direction vector as much as possible
		void Normalise()
		{
			T hypoDist = sqrt(m_direction.x*m_direction.x + m_direction.y*m_direction.y);

			m_direction = m_direction/hypoDist;
			m_scalar = hypoDist;
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
	static constexpr uint32_t m_vectorsSize = 2;
	Vector2D<float> m_vectors[m_vectorsSize];

	SDL_Point* m_tessalationPoints;

	float m_propagationRate;
	std::vector<uint32_t> m_pointsToPropagate;

	//pixel dimensions of world on screen
	uint32_t m_debugWorldPixelDim = m_tilesDimPixels*m_debugWorldDim;
	uint32_t m_verticlePixelBuffer = (Globals::WINDOW_HEIGHT - m_debugWorldPixelDim) / 2;
	uint32_t m_horizontalPixelBuffer = (Globals::WINDOW_WIDTH - m_debugWorldPixelDim) / 2;

	bool m_showMousePos = false;
	bool m_setTilesActive = false;
	bool m_setTilesUnactive = false;

	//// functions ///////////////////////////////////////////////////////////////////////////
	void CyclePropModeLeft();
	void CyclePropModeRight();
	void PropagateAdjacent();
	void UnpropagateAdjacent();
	void ResetWorldGrid();

	uint32_t GetIndexUp(uint32_t _currentPointIndex);
	uint32_t GetIndexDown(uint32_t _currentPointIndex);
	uint32_t GetIndexLeft(uint32_t _currentPointIndex);
	uint32_t GetIndexRight(uint32_t _currentPointIndex);

	void FindTileUnderMouse(flVec2<int>& _mousePos);
	void MaintainMousePosLabel(flVec2<int>& _mousePos);
};
