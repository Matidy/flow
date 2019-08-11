#include "Interfaces/InputCoreIF.h"

#include "InputDelegate.h"


InputDelegate::InputDelegate(InputCoreIF& _inputCore)
		: m_inputCoreIF(_inputCore)
{}

InputDelegate::~InputDelegate()
{
	//@check this is actually being called, given virtual destructor rule of thumb
	m_inputCoreIF.InputDelegateDestructing(this);
}