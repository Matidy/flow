#include <vector>
#include <SDL_scancode.h>

#include "Data/flPoint.h"
#include "InputDelegate.h"

struct RenderCoreIF;
struct InputCoreIF;

class Debug : public InputDelegate
{
public:
	//from InputDelegate
	virtual void DefineHeldInput() override final;
	virtual void KeyPressedInput(SDL_Scancode const& _key) override final;
	virtual void KeyReleasedInput(SDL_Scancode const& _key) override final;

	static constexpr uint32_t m_debugWorldDim = 17u;
	static constexpr uint32_t m_debugWorldSize = m_debugWorldDim * m_debugWorldDim;

	Debug(RenderCoreIF& _renderCoreIF, InputCoreIF& _inputCore);
	void UpdateStep(uint32_t const _timeStep);

	bool const& ToggleDebug();
	bool const DebugEnabled() const;

	bool const DisplayTessalation() const;

	flPoint const * const GetDebugWorldDataRead() const;

private:
	enum PropagationMode
	{
		SimpleDirectional,
		SemiCircleDirectional,

		CountPropagationMode
	};
	uint32_t m_propagationMode;

	static constexpr uint32_t m_nullIndex = ~0;

	flPoint m_debugWorld[m_debugWorldDim*m_debugWorldDim];
	bool m_debugEnabled;
	//used to tell Window whether it should be displaying tessalation screen or not
	bool m_displayTessalation;

	float m_propagationRate;
	std::vector<uint32_t> m_pointsToPropagate;

	RenderCoreIF& m_renderCoreIF;

	void ToggleTessalationDisplay();

	void CyclePropModeLeft();
	void CyclePropModeRight();
	void PropagateAdjacent();
	void UnpropagateAdjacent();
	void ResetWorldGrid();

	uint32_t GetIndexUp(uint32_t _currentPointIndex);
	uint32_t GetIndexDown(uint32_t _currentPointIndex);
	uint32_t GetIndexLeft(uint32_t _currentPointIndex);
	uint32_t GetIndexRight(uint32_t _currentPointIndex);
};
