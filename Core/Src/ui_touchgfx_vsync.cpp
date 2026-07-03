#include <touchgfx/hal/OSWrappers.hpp>

extern "C" void UI_TouchGFX_SignalVSync(void)
{
    touchgfx::OSWrappers::signalVSync();
}
