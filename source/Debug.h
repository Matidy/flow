#include <vector>
#include <cmath>
#include <SDL_scancode.h>

#include "Globals.h"
#include "Libs/PhysicsLib.h"
#include "Data/flEnergy.h"
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

	flEnergy* m_debugWorld;
	int32_t  m_tileUnderMouseIndex = -1;
	bool m_debugEnabled;

	
	static constexpr uint32_t m_vectorsSize = 12;
	PhysicsLib::Vector2D<float> m_vectors[m_vectorsSize];
	uint32_t m_vectorsFilled = 0u;

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
