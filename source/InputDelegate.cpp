#include "Interfaces/InputCoreIF.h"

#include "InputDelegate.h"


InputDelegate::InputDelegate(InputCoreIF& _inputCore)
		: m_inputCoreIF(_inputCore)
{}