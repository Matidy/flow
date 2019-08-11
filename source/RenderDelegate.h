#pragma once

struct SDL_Renderer; //@this is enough for function param? no need to actually include definition for this?
struct RenderCoreIF;

class RenderDelegate
{
public:
	RenderDelegate(RenderCoreIF& _renderCore, bool _startEnabled = true);
	~RenderDelegate();

	//params - _enabled - true to enable, false to disable
	bool const& ToggleRender(bool const _enabled) { return m_enabled = _enabled; }
	bool const& Enabled() const { return m_enabled; }

	virtual void DelegateDraw(SDL_Renderer * const _gRenderer) const {}
protected:
	bool m_enabled;
	RenderCoreIF& m_renderCoreIF;
};