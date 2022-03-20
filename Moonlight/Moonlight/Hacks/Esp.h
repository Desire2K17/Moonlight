#pragma once

enum EspId {
    ALLIES_ALL = 0,
    ALLIES_VISIBLE,
    ALLIES_OCCLUDED,

    ENEMIES_ALL,
    ENEMIES_VISIBLE,
    ENEMIES_OCCLUDED
};

namespace Esp
{
    void render() noexcept;
}
