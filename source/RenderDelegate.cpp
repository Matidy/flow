#include "Interfaces/RenderCoreIF.h"

#include "RenderDelegate.h"

RenderDelegate::RenderDelegate(RenderCoreIF& _renderCore, bool _startEnabled)
		: m_enabled(_startEnabled),
		  m_renderCoreIF(_renderCore)
{
	m_renderCoreIF.AddRenderDelegate(this);
}

RenderDelegate::~RenderDelegate()
{
	m_renderCoreIF.RemoveRenderDelegate(this);
}