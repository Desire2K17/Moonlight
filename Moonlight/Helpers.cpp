#include <cmath>
#include <cwctype>
#include <fstream>
#include <tuple>

#include "imgui/imgui.h"

#include "Config.h"
#include "ConfigStructs.h"
#include "GameData.h"
#include "Helpers.h"
#include "Memory.h"
#include "SDK/GlobalVars.h"

float Helpers::approachValueSmooth(float target, float value, float fraction) noexcept
{
    float delta = target - value;
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    delta *= fraction;
    return value + delta;
}
